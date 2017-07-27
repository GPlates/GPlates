/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sstream>
#include <vector>
#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>

#include "InternalSession.h"

#include "DeprecatedSessionRestore.h"
#include "TranscribeSession.h"

#include "global/GPlatesAssert.h"

#include "scribe/Scribe.h"
#include "scribe/ScribeExceptions.h"
#include "scribe/ScribeBinaryArchiveReader.h"
#include "scribe/ScribeBinaryArchiveWriter.h"
#include "scribe/ScribeTextArchiveReader.h"
#include "scribe/ScribeTextArchiveWriter.h"
#include "scribe/TranscribeUtils.h"


const QString GPlatesPresentation::InternalSession::CURRENT_FORMAT_SESSION_METADATA_KEY("session_metadata");
const QString GPlatesPresentation::InternalSession::CURRENT_FORMAT_SESSION_DATA_KEY("session_data");
const QString GPlatesPresentation::InternalSession::GPLATES_1_5_FORMAT_SESSION_STATE_KEY("serialized_session_state");


GPlatesPresentation::InternalSession::SessionFormat
GPlatesPresentation::InternalSession::get_session_format(
		const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state)
{
	//
	// Test starting with most recent format and ending with least recent.
	//
	// This is because current format also saves the format 'GPLATES_1_5_FORMAT', so when loading
	// such an archive we want the current format (since it contains the most information).
	//

	if (session_state.contains(CURRENT_FORMAT_SESSION_METADATA_KEY) &&
		session_state.contains(CURRENT_FORMAT_SESSION_DATA_KEY))
	{
		return CURRENT_FORMAT;
	}

	if (session_state.contains(GPLATES_1_5_FORMAT_SESSION_STATE_KEY))
	{
		return GPLATES_1_5_FORMAT;
	}

	// The "loaded_files" key exists for all GPLATES_1_4_OR_BEFORE_FORMAT versions.
	if (session_state.contains("loaded_files"))
	{
		return GPLATES_1_4_OR_BEFORE_FORMAT;
	}

	return UNKNOWN_FORMAT;
}


bool
GPlatesPresentation::InternalSession::has_valid_session_keys(
		const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state)
{
	return get_session_format(session_state) != UNKNOWN_FORMAT;
}


GPlatesPresentation::InternalSession::non_null_ptr_type
GPlatesPresentation::InternalSession::create_restore_session(
		const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state)
{
	const SessionFormat session_format = get_session_format(session_state);

	if (session_format == GPLATES_1_4_OR_BEFORE_FORMAT)
	{
		// Note: The prefs KeyValueMap is a QMap of QStrings->QVariants.
		// The .value() call we use here will use a default-constructed value if no such entry exists.
		const QDateTime time = session_state.value("time").toDateTime();
		const QStringList loaded_files = session_state.value("loaded_files").toStringList();

		return non_null_ptr_type(
				new InternalSession(session_state, time, loaded_files, QStringList()/*all_file_paths*/));
	}

	//
	// Load the session metadata transcription.
	//

	boost::optional<GPlatesScribe::Transcription::non_null_ptr_type> transcription_metadata_opt;

	if (session_format == GPLATES_1_5_FORMAT)
	{
		//
		// GPlates 1.5 stores session state in a 'text' archive.
		//

		const QString gplates_1_5_session_archive =
				session_state.value(GPLATES_1_5_FORMAT_SESSION_STATE_KEY).toString();

		// Convert the serialized QString to a string.
		const QByteArray gplates_1_5_session_archive_byte_array = gplates_1_5_session_archive.toLatin1();

		// Need to specify the array size in case array has embedded zeros,
		// otherwise the first zero found will terminate the string assignment.
		const std::string gplates_1_5_session_archive_string(
				gplates_1_5_session_archive_byte_array.constData(),
				gplates_1_5_session_archive_byte_array.size());

		// Store the archive string in the archive stream so the Scribe can read it.
		std::istringstream gplates_1_5_session_archive_stream(gplates_1_5_session_archive_string);

		GPlatesScribe::ArchiveReader::non_null_ptr_type gplates_1_5_session_archive_reader =
				GPlatesScribe::TextArchiveReader::create(gplates_1_5_session_archive_stream);

		// Read the session metadata transcription from the archive.
		transcription_metadata_opt = gplates_1_5_session_archive_reader->read_transcription();

		// Note: We don't close the archive reader because we are not reading the GPlates 1.5
		// session 'data' transcription here (both session metadata and data are in same archive)
		// and closing might give a not-at-end-of-archive error.
	}
	else if (session_format == CURRENT_FORMAT)
	{
		//
		// Current format stores session state in a 'binary' archive.
		//

		QByteArray session_metadata_archive = session_state.value(CURRENT_FORMAT_SESSION_METADATA_KEY).toByteArray();
		QDataStream session_metadata_archive_stream(&session_metadata_archive, QIODevice::ReadOnly);

		GPlatesScribe::ArchiveReader::non_null_ptr_type session_metadata_archive_reader =
				GPlatesScribe::BinaryArchiveReader::create(session_metadata_archive_stream);

		// Read the session metadata transcription from the archive.
		transcription_metadata_opt = session_metadata_archive_reader->read_transcription();

		session_metadata_archive_reader->close();
	}
	else // session_format == UNKNOWN_FORMAT ...
	{
		GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
				false,
				GPLATES_ASSERTION_SOURCE);
	}

	const GPlatesScribe::Transcription::non_null_ptr_type transcription_metadata = transcription_metadata_opt.get();

	//
	// Load the session metadata.
	//

	// The scribe to load the session metadata from the session metadata transcription.
	GPlatesScribe::Scribe scribe_metadata(transcription_metadata);

	// Load session date/time.
	GPlatesScribe::LoadRef<QDateTime> time = scribe_metadata.load<QDateTime>(TRANSCRIBE_SOURCE, "time");
	GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
			time.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	// Load the feature collection filenames.
	// Use the TranscribeUtils::FilePath API to generate smaller archives/transcriptions.
	boost::optional<QStringList> loaded_files =
			GPlatesScribe::TranscribeUtils::load_file_paths(scribe_metadata, TRANSCRIBE_SOURCE, "loaded_files");
	GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
			loaded_files,
			GPLATES_ASSERTION_SOURCE);

	// Load all transcribed file paths.
	// Use the TranscribeUtils::FilePath API to generate smaller archives/transcriptions.
	//
	// Note: Older versions of GPlates (1.5) don't store this, in which case we just replace it
	// with 'loaded_files' since those were the only transcribed filenames in those older versions.
	boost::optional<QStringList> all_file_paths =
			GPlatesScribe::TranscribeUtils::load_file_paths(scribe_metadata, TRANSCRIBE_SOURCE, "all_file_paths");
	if (!all_file_paths)
	{
		all_file_paths = loaded_files.get();
	}

	// Make sure the metadata transcription is complete to ensure the metadata was restored correctly.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(new InternalSession(session_state, time, loaded_files.get(), all_file_paths.get()));
}


GPlatesPresentation::InternalSession::non_null_ptr_type
GPlatesPresentation::InternalSession::save_session()
{
	//
	// Session data.
	//

	// The scribe to save the session data.
	//
	// We also separate out the GPlates 1.5 format session state (to support GPlates 1.5) into a
	// separate scribe so we can write it to a separate key in the session state (in user preferences).
	GPlatesScribe::Scribe scribe_data;
	GPlatesScribe::Scribe scribe_data_gplates_1_5;

	// Record all saved file paths (whilst transcribing session data).
	//
	// Note: This is not just feature collection files. Can be any file (eg, CPT file).
	GPlatesScribe::TranscribeContext<GPlatesScribe::TranscribeUtils::FilePath> transcribe_file_path_context;
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<GPlatesScribe::TranscribeUtils::FilePath>
			transcribe_file_path_context_guard(scribe_data, transcribe_file_path_context);

	// Transcribe the session state.
	// Returns the loaded feature collection filenames.
	const QStringList loaded_files = TranscribeSession::save(scribe_data, scribe_data_gplates_1_5);

	// All saved file paths (transcribed while saving session data).
	// These are unique (and sorted) transcribed file paths.
	const QStringList all_file_paths = transcribe_file_path_context.get_file_paths();

	// Make sure the saved transcription is complete.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_data.is_transcription_complete() &&
				scribe_data_gplates_1_5.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	//
	// Session metadata.
	//

	// The scribe to save the session metadata.
	GPlatesScribe::Scribe scribe_metadata;

	// Save the time to the session metadata.
	const QDateTime time = QDateTime::currentDateTime();
	scribe_metadata.save(TRANSCRIBE_SOURCE, time, "time");

	// Save the feature collection filenames.
	// Use the TranscribeUtils::FilePath API to generate smaller archives/transcriptions.
	GPlatesScribe::TranscribeUtils::save_file_paths(scribe_metadata, TRANSCRIBE_SOURCE, loaded_files, "loaded_files");

	// Save all transcribed file paths.
	// Use the TranscribeUtils::FilePath API to generate smaller archives/transcriptions.
	GPlatesScribe::TranscribeUtils::save_file_paths(scribe_metadata, TRANSCRIBE_SOURCE, all_file_paths, "all_file_paths");

	// Make sure the metadata transcription is complete otherwise the metadata will be incorrectly
	// restored when the archive is loaded.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);


	//
	// The session state key/value map that stores all transcriptions/archives.
	//
	GPlatesAppLogic::UserPreferences::KeyValueMap session_state;


	//
	// Write session metadata to a binary buffer (stored in 'CURRENT_FORMAT_SESSION_METADATA_KEY').
	//
	// We write to a binary buffer because it's smaller than a text buffer. On the Windows platform
	// the session state is saved to the Windows Registry which has a size limit of 1MB per entry
	// (which amounts to 512KB characters since Qt stores them as 16-bit unicode).
	//

	QByteArray session_metadata_binary_archive;
	QDataStream session_metadata_archive_stream(&session_metadata_binary_archive, QIODevice::WriteOnly);

	GPlatesScribe::ArchiveWriter::non_null_ptr_type session_metadata_archive_writer =
			GPlatesScribe::BinaryArchiveWriter::create(session_metadata_archive_stream);

	// Write the session metadata transcription to the archive.
	session_metadata_archive_writer->write_transcription(*scribe_metadata.get_transcription());

	session_metadata_archive_writer->close();

	// Save session metadata binary buffer to user preferences.
	session_state.insert(CURRENT_FORMAT_SESSION_METADATA_KEY, QVariant(session_metadata_binary_archive));

	//
	// Write session data to a binary buffer (stored in 'CURRENT_FORMAT_SESSION_DATA_KEY').
	//
	// We write to a binary buffer because it's smaller than a text buffer. On the Windows platform
	// the session state is saved to the Windows Registry which has a size limit of 1MB per entry
	// (which amounts to 512KB characters since Qt stores them as 16-bit unicode).
	//

	QByteArray session_data_archive;
	QDataStream session_data_archive_stream(&session_data_archive, QIODevice::WriteOnly);

	GPlatesScribe::ArchiveWriter::non_null_ptr_type session_data_archive_writer =
			GPlatesScribe::BinaryArchiveWriter::create(session_data_archive_stream);

	// Write the session data transcription to the archive.
	session_data_archive_writer->write_transcription(*scribe_data.get_transcription());

	session_data_archive_writer->close();

	// Compress the binary archive.
	//
	// Compressing our binary buffer reduces its size to less than half. On the Windows platform
	// the session state is saved to the Windows Registry which has a size limit of 1MB per entry
	// (which amounts to 512KB characters since Qt stores them as 16-bit unicode).
	session_data_archive = qCompress(session_data_archive);

	// Save session data binary buffer to user preferences.
	session_state.insert(CURRENT_FORMAT_SESSION_DATA_KEY, QVariant(session_data_archive));


	//
	// Write GPlates 1.5 session metadata/data to a text/string buffer (stored in 'GPLATES_1_5_FORMAT_SESSION_STATE_KEY').
	//
	// We have to save to a *text* archive because GPlates 1.5 expects a text archive when it loads.
	//
	// Also note that, after GPlates 1.5 was released, we made a small change to the text archive reader/writer
	// to support 'inf', '-inf' and 'nan' for floating-point numbers. This would cause a problem
	// for GPlates 1.5 if any of these numbers are transcribed. Luckily it turns out that they never
	// are transcribed for the state that is saved for GPlates 1.5 (it only happens for things like
	// a GeoTimeInstant that is 'distant-past' or 'distant-future', but GeoTimeInstant is not transcribed
	// for GPlates 1.5 state). Also note that, for this reason, we cannot combine both the current format
	// state and the GPlates 1.5 format state in the same transcription (because when the transcription
	// is loaded by GPlates 1.5 it would likely encounter 'inf', '-inf' or 'nan' which it does not expect).
	// But that's not a problem because the current format state is saved in a separate transcription.
	//

	// Serialize the current state of GPlates into a string stream.
	std::ostringstream gplates_1_5_session_archive_stream;

	GPlatesScribe::ArchiveWriter::non_null_ptr_type gplates_1_5_session_archive_writer =
			GPlatesScribe::TextArchiveWriter::create(gplates_1_5_session_archive_stream);

	// Write the session metadata transcription to the archive.
	gplates_1_5_session_archive_writer->write_transcription(*scribe_metadata.get_transcription());

	// Write the session data transcription to the archive.
	gplates_1_5_session_archive_writer->write_transcription(*scribe_data_gplates_1_5.get_transcription());

	gplates_1_5_session_archive_writer->close();

	// Get the serialized string from the archive.
	const std::string gplates_1_5_session_archive_string = gplates_1_5_session_archive_stream.str();

	// Convert the serialized string to a QString.
	//
	// Need to specify the string size in case string has embedded zeros,
	// otherwise the first zero found will terminate the string assignment.
	//
	const QString gplates_1_5_session_session_state = QString::fromLatin1(
			gplates_1_5_session_archive_string.data(),
			gplates_1_5_session_archive_string.length());

	session_state.insert(GPLATES_1_5_FORMAT_SESSION_STATE_KEY, QVariant(gplates_1_5_session_session_state));


	return non_null_ptr_type(new InternalSession(session_state, time, loaded_files, all_file_paths));
}


GPlatesPresentation::InternalSession::InternalSession(
		const GPlatesAppLogic::UserPreferences::KeyValueMap &session_key_value_map_,
		const QDateTime &time_,
		const QStringList &filenames_,
		const QStringList &all_file_paths_) :
	Session(time_, filenames_),
	d_session_key_value_map(session_key_value_map_),
	d_all_file_paths(all_file_paths_)
{  }


void
GPlatesPresentation::InternalSession::get_file_paths(
		QStringList &existing_file_paths,
		QStringList &missing_file_paths) const
{
	const int num_file_paths = d_all_file_paths.size();
	for (int n = 0; n < num_file_paths; ++n)
	{
		const QString absolute_file_path =
				GPlatesScribe::TranscribeUtils::convert_file_path(d_all_file_paths[n]);

		if (QFileInfo(absolute_file_path).exists())
		{
			existing_file_paths.append(absolute_file_path);
		}
		else // missing...
		{
			missing_file_paths.append(absolute_file_path);
		}
	}
}


void
GPlatesPresentation::InternalSession::set_remapped_file_paths(
		boost::optional< QMap<QString/*missing*/, QString/*existing*/> > file_path_remapping)
{
	d_file_path_remapping = file_path_remapping;
}


void
GPlatesPresentation::InternalSession::restore_session() const
{
	const SessionFormat session_format = get_session_format(d_session_key_value_map);

	// If the session was created by a version of GPlates before the general scribe system was
	// introduced then delegate to the old way of restoring sessions.
	if (session_format == GPLATES_1_4_OR_BEFORE_FORMAT)
	{
		// 'GPLATES_1_4_OR_BEFORE_FORMAT' sessions have a version number (from 0 to 3 inclusive).
		// 'GPLATES_1_5_FORMAT' (and after) sessions do not need a version number (since versioning is
		// handled implicitly by the Scribe system).
		//
		// The 'version' entry was added at version 1, previous versions should default to zero...
		//
		// Note: The prefs KeyValueMap is a QMap of QStrings->QVariants.
		// The .value() call we use here will use a default-constructed value if no such entry exists.
		const int deprecated_version = d_session_key_value_map.value("version").toInt();

		const QString layers_state = d_session_key_value_map.value("layers_state").toString();

		DeprecatedSessionRestore::restore_session(
				deprecated_version,
				get_time(),
				get_loaded_files(),
				layers_state);

		return;
	}

	//
	// Load the session metadata and data transcriptions.
	//

	boost::optional<GPlatesScribe::Transcription::non_null_ptr_type> transcription_metadata_opt;
	boost::optional<GPlatesScribe::Transcription::non_null_ptr_type> transcription_data_opt;

	if (session_format == GPLATES_1_5_FORMAT)
	{
		//
		// GPlates 1.5 stores session state in a 'text' archive.
		//

		const QString gplates_1_5_session_archive =
				d_session_key_value_map.value(GPLATES_1_5_FORMAT_SESSION_STATE_KEY).toString();

		// Convert the serialized QString to a string.
		const QByteArray gplates_1_5_session_archive_byte_array = gplates_1_5_session_archive.toLatin1();

		// Need to specify the array size in case array has embedded zeros,
		// otherwise the first zero found will terminate the string assignment.
		const std::string gplates_1_5_session_archive_string(
				gplates_1_5_session_archive_byte_array.constData(),
				gplates_1_5_session_archive_byte_array.size());

		// Store the archive string in the archive stream so the Scribe can read it.
		std::istringstream gplates_1_5_session_archive_stream(gplates_1_5_session_archive_string);

		GPlatesScribe::ArchiveReader::non_null_ptr_type gplates_1_5_session_archive_reader =
				GPlatesScribe::TextArchiveReader::create(gplates_1_5_session_archive_stream);

		// Read the session metadata transcription from the archive.
		transcription_metadata_opt = gplates_1_5_session_archive_reader->read_transcription();

		// Read the session data transcription from the archive (the second transcription in archive).
		transcription_data_opt = gplates_1_5_session_archive_reader->read_transcription();

		// We close the archive reader because we have read both session 'metadata' and 'data' transcriptions.
		// And we want to check we've correctly reached the end of the archive.
		gplates_1_5_session_archive_reader->close();
	}
	else if (session_format == CURRENT_FORMAT)
	{
		//
		// Current format stores session state in a 'binary' archive.
		//

		//
		// Session 'metadata' transcription.
		//

		QByteArray session_metadata_archive =
				d_session_key_value_map.value(CURRENT_FORMAT_SESSION_METADATA_KEY).toByteArray();
		QDataStream session_metadata_archive_stream(&session_metadata_archive, QIODevice::ReadOnly);

		GPlatesScribe::ArchiveReader::non_null_ptr_type session_metadata_archive_reader =
				GPlatesScribe::BinaryArchiveReader::create(session_metadata_archive_stream);

		// Read the session metadata transcription from the archive.
		transcription_metadata_opt = session_metadata_archive_reader->read_transcription();

		session_metadata_archive_reader->close();

		//
		// Session 'data' transcription.
		//

		QByteArray session_data_archive =
				d_session_key_value_map.value(CURRENT_FORMAT_SESSION_DATA_KEY).toByteArray();

		// Uncompress the binary archive.
		session_data_archive = qUncompress(session_data_archive);

		QDataStream session_data_archive_stream(&session_data_archive, QIODevice::ReadOnly);

		GPlatesScribe::ArchiveReader::non_null_ptr_type session_data_archive_reader =
				GPlatesScribe::BinaryArchiveReader::create(session_data_archive_stream);

		// Read the session data transcription from the archive.
		transcription_data_opt = session_data_archive_reader->read_transcription();

		session_data_archive_reader->close();
	}
	else // session_format == UNKNOWN_FORMAT ...
	{
		GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
				false,
				GPLATES_ASSERTION_SOURCE);
	}

	const GPlatesScribe::Transcription::non_null_ptr_type transcription_metadata = transcription_metadata_opt.get();
	const GPlatesScribe::Transcription::non_null_ptr_type transcription_data = transcription_data_opt.get();

	//
	// Remap missing file paths (if any) to existing file paths.
	//

	GPlatesScribe::TranscribeContext<GPlatesScribe::TranscribeUtils::FilePath> transcribe_file_path_context;
	transcribe_file_path_context.set_load_file_path_remapping(d_file_path_remapping);

	//
	// Session metadata.
	//

	// The scribe to load the session metadata from the session metadata transcription.
	GPlatesScribe::Scribe scribe_metadata(transcription_metadata);

	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<GPlatesScribe::TranscribeUtils::FilePath>
			transcribe_file_path_context_guard_for_scribe_metadata(scribe_metadata, transcribe_file_path_context);

	// Load the feature collection filenames.
	// Use the TranscribeUtils::FilePath API to generate smaller archives/transcriptions.
	boost::optional<QStringList> loaded_files =
			GPlatesScribe::TranscribeUtils::load_file_paths(scribe_metadata, TRANSCRIBE_SOURCE, "loaded_files");
	GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
			loaded_files,
			GPLATES_ASSERTION_SOURCE);

	// Make sure the metadata transcription is complete to ensure the metadata was restored correctly.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	//
	// Session data.
	//

	// The scribe to load the session data from the session data transcription.
	GPlatesScribe::Scribe scribe_data(transcription_data);

	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<GPlatesScribe::TranscribeUtils::FilePath>
			transcribe_file_path_context_guard_for_scribe_data(scribe_data, transcribe_file_path_context);

	// Transcribe the session state.
	//
	// NOTE: We use the metadata "loaded_files" rather than 'Session::get_loaded_files()' on the
	// off-chance that there were multiple identical filenames (which there shouldn't be) and
	// 'Session::get_loaded_files()' removed duplicates by converting to a QSet and back - this
	// would mess up our transcribed file indices and potentially cause layers to be connected
	// to the wrong files.
	TranscribeSession::load(scribe_data, loaded_files.get());

	// Make sure scribe loaded from transcription correctly (eg, no dangling pointers due to
	// discarded pointed-to objects).
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_data.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);
}
