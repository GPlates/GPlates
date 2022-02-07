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
#include <QFile>

#include "InternalSession.h"

#include "Application.h"
#include "DeprecatedSessionRestore.h"
#include "TranscribeSession.h"

#include "global/GPlatesAssert.h"

#include "scribe/Scribe.h"
#include "scribe/ScribeExceptions.h"
#include "scribe/ScribeTextArchiveReader.h"
#include "scribe/ScribeTextArchiveWriter.h"
#include "scribe/TranscribeUtils.h"


GPlatesPresentation::InternalSession::non_null_ptr_to_const_type
GPlatesPresentation::InternalSession::create_restore_session(
		const GPlatesAppLogic::UserPreferences::KeyValueMap &session_state,
		bool is_deprecated_session)
{
	// Deprecated sessions have a version number (from 0 to 3 inclusive).
	// Non-deprecated sessions do not need a version number (versioning is handled implicitly
	// by the Scribe system).
	boost::optional<int> deprecated_version;
	if (is_deprecated_session)
	{
		// The 'version' entry was added at version 1, previous versions should default to zero...
		//
		// Note: The prefs KeyValueMap is a QMap of QStrings->QVariants.
		// The .value() call we use here will use a default-constructed value if no such entry exists.
		deprecated_version = session_state.value("version").toInt();

		const QDateTime time = session_state.value("time").toDateTime();

		const QStringList loaded_files = session_state.value("loaded_files").toStringList();

		return non_null_ptr_to_const_type(
				new InternalSession(session_state, time, loaded_files, deprecated_version));
	}

	//
	// Setup internal session for reading session metadata.
	//

	const QString serialized_session_state = session_state.value("serialized_session_state").toString();

	// Convert the serialized QString to a string.
	const QByteArray archive_byte_array = serialized_session_state.toLatin1();

	// Need to specify the array size in case array has embedded zeros,
	// otherwise the first zero found will terminate the string assignment.
	const std::string archive_string(archive_byte_array.constData(), archive_byte_array.size());

	// Store the archive string in the archive stream so the Scribe can read it.
	std::istringstream archive_stream(archive_string);

	GPlatesScribe::ArchiveReader::non_null_ptr_type archive_reader =
			GPlatesScribe::TextArchiveReader::create(archive_stream);

	// Read the session metadata transcription from the archive.
	// Note: We don't close the archive reader because we are not reading the session 'data'
	// transcription and closing might give a not-at-end-of-archive error.
	const GPlatesScribe::Transcription::non_null_ptr_type transcription_metadata =
			archive_reader->read_transcription();

	//
	// Session metadata.
	//

	// The scribe to load the session metadata from the session metadata transcription.
	GPlatesScribe::Scribe scribe_metadata(transcription_metadata);

	// Load session date/time.
	GPlatesScribe::LoadRef<QDateTime> time = scribe_metadata.load<QDateTime>(TRANSCRIBE_SOURCE, "time");
	GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
			time.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	// Load session loaded filenames.
	// Use 'TranscribeUtils::FilePath' to generate smaller archives/transcriptions.
	GPlatesScribe::LoadRef< std::vector<GPlatesScribe::TranscribeUtils::FilePath> > transcribe_loaded_files =
			scribe_metadata.load< std::vector<GPlatesScribe::TranscribeUtils::FilePath> >(
					TRANSCRIBE_SOURCE, "loaded_files");
	GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
			transcribe_loaded_files.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	// Extract load filenames as QStrings.
	QStringList loaded_files;
	for (unsigned int n = 0; n < transcribe_loaded_files->size(); ++n)
	{
		loaded_files.append(transcribe_loaded_files->at(n));
	}

	// Make sure the metadata transcription is complete to ensure the metadata was restored correctly.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_to_const_type(new InternalSession(session_state, time, loaded_files));
}


GPlatesPresentation::InternalSession::non_null_ptr_to_const_type
GPlatesPresentation::InternalSession::save_session()
{
	//
	// Setup string buffer for writing session metadata/data.
	//

	// Serialize the current state of GPlates into a string stream.
	std::ostringstream archive_stream;

	GPlatesScribe::ArchiveWriter::non_null_ptr_type archive_writer =
			GPlatesScribe::TextArchiveWriter::create(archive_stream);

	//
	// Session metadata.
	//

	// The scribe to save the session metadata.
	GPlatesScribe::Scribe scribe_metadata;

	// The session metadata.
	const QDateTime time = QDateTime::currentDateTime();
	const QStringList loaded_files = TranscribeSession::get_save_session_files();

	// Save the session metadata.

	scribe_metadata.save(TRANSCRIBE_SOURCE, time, "time");

	// Use 'TranscribeUtils::FilePath' to generate smaller archives/transcriptions.
	std::vector<GPlatesScribe::TranscribeUtils::FilePath> transcribe_loaded_files(
			loaded_files.begin(), loaded_files.end());
	scribe_metadata.transcribe(TRANSCRIBE_SOURCE, transcribe_loaded_files, "loaded_files");

	// Make sure the metadata transcription is complete otherwise the metadata will be incorrectly
	// restored when the archive is loaded.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	// Write the session metadata transcription to the archive.
	archive_writer->write_transcription(*scribe_metadata.get_transcription());

	//
	// Session data.
	//

	// The scribe to save the session data.
	GPlatesScribe::Scribe scribe_data;

	// Transcribe the session state.
	TranscribeSession::transcribe(scribe_data, loaded_files);

	// Write the session data transcription to the archive.
	archive_writer->write_transcription(*scribe_data.get_transcription());

	archive_writer->close();

	//
	// Save string buffer to user preferences.
	//

	// Get the serialized string from the archive.
	const std::string archive_string = archive_stream.str();

	// Convert the serialized string to a QString.
	//
	// Need to specify the string size in case string has embedded zeros,
	// otherwise the first zero found will terminate the string assignment.
	const QString serialized_session_state =
			QString::fromLatin1(archive_string.data(), archive_string.length());

	GPlatesAppLogic::UserPreferences::KeyValueMap session_state;

	session_state.insert("serialized_session_state", QVariant(serialized_session_state));

	return non_null_ptr_to_const_type(new InternalSession(session_state, time, loaded_files));
}


GPlatesPresentation::InternalSession::InternalSession(
		const GPlatesAppLogic::UserPreferences::KeyValueMap &session_key_value_map_,
		const QDateTime &time_,
		const QStringList &files_,
		boost::optional<int> deprecated_version) :
	Session(time_, files_),
	d_session_key_value_map(session_key_value_map_),
	d_deprecated_version(deprecated_version)
{  }


QStringList
GPlatesPresentation::InternalSession::restore_session() const
{
	// If the session was created by a version of GPlates before the general scribe system was
	// introduced then delegate to the old way of restoring sessions.
	if (d_deprecated_version)
	{
		// There was no "serialized_session_state" key before session version 4.0.
		const QString layers_state = d_session_key_value_map.value("layers_state").toString();

		return DeprecatedSessionRestore::restore_session(
				d_deprecated_version.get(),
				get_time(),
				get_loaded_files(),
				layers_state,
				GPlatesPresentation::Application::instance().get_application_state());
	}

	//
	// Setup internal session for reading session metadata/data.
	//

	const QString serialized_session_state =
			d_session_key_value_map.value("serialized_session_state").toString();

	// Convert the serialized QString to a string.
	const QByteArray archive_byte_array = serialized_session_state.toLatin1();

	// Need to specify the array size in case array has embedded zeros,
	// otherwise the first zero found will terminate the string assignment.
	const std::string archive_string(archive_byte_array.constData(), archive_byte_array.size());

	// Store the archive string in the archive stream so the Scribe can read it.
	std::istringstream archive_stream(archive_string);

	GPlatesScribe::ArchiveReader::non_null_ptr_type archive_reader =
			GPlatesScribe::TextArchiveReader::create(archive_stream);

	// Read the session metadata transcription from the archive.
	const GPlatesScribe::Transcription::non_null_ptr_type transcription_metadata =
			archive_reader->read_transcription();

	// Read the session data transcription from the archive (the second transcription in archive).
	const GPlatesScribe::Transcription::non_null_ptr_type transcription_data =
			archive_reader->read_transcription();

	// We close the archive reader because we have read both session 'metadata' and 'data' transcriptions.
	// And we want to check we've correctly reached the end of the archive.
	archive_reader->close();

	//
	// Session metadata.
	//

	// The scribe to load the session metadata from the session metadata transcription.
	GPlatesScribe::Scribe scribe_metadata(transcription_metadata);

	// Load session loaded filenames.
	// Use 'TranscribeUtils::FilePath' to generate smaller archives/transcriptions.
	GPlatesScribe::LoadRef< std::vector<GPlatesScribe::TranscribeUtils::FilePath> > transcribe_loaded_files =
			scribe_metadata.load< std::vector<GPlatesScribe::TranscribeUtils::FilePath> >(
					TRANSCRIBE_SOURCE, "loaded_files");
	GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
			transcribe_loaded_files.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	// Extract load filenames as QStrings.
	QStringList loaded_files;
	for (unsigned int n = 0; n < transcribe_loaded_files->size(); ++n)
	{
		loaded_files.append(transcribe_loaded_files->at(n));
	}

	// Make sure the metadata transcription is complete to ensure the metadata was restored correctly.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	//
	// Session data.
	//

	// The scribe to load the session data from the session data transcription.
	GPlatesScribe::Scribe scribe_data(transcription_data);

	// Transcribe the session state.
	//
	// NOTE: We use the metadata "loaded_files" rather than 'Session::get_loaded_files()' on the
	// off-chance that there were multiple identical filenames (which there shouldn't be) and
	// 'Session::get_loaded_files()' removed duplicates by converting to a QSet and back - this
	// would mess up our transcribed file indices and potentially cause layers to be connected
	// to the wrong files.
	QStringList files_not_loaded =
			TranscribeSession::transcribe(scribe_data, loaded_files);

	return files_not_loaded;
}
