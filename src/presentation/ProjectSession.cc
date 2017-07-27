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

#include <vector>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "ProjectSession.h"

#include "Application.h"
#include "TranscribeSession.h"

#include "app-logic/ApplicationState.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningFileForWritingException.h"

#include "global/GPlatesAssert.h"

#include "scribe/Scribe.h"
#include "scribe/ScribeBinaryArchiveReader.h"
#include "scribe/ScribeBinaryArchiveWriter.h"
#include "scribe/ScribeExceptions.h"
#include "scribe/TranscribeUtils.h"


GPlatesPresentation::ProjectSession::non_null_ptr_type
GPlatesPresentation::ProjectSession::create_restore_session(
		QString project_filename)
{
	// Make sure project filename is an absolute path.
	project_filename = QFileInfo(project_filename).absoluteFilePath();

	//
	// Setup project file for reading session metadata.
	//

	// Open the project file for reading.
	QFile project_file(project_filename);
	if (!project_file.open(QIODevice::ReadOnly))
	{
		throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				project_filename);
	}

	QDataStream archive_stream(&project_file);

	GPlatesScribe::ArchiveReader::non_null_ptr_type archive_reader =
			GPlatesScribe::BinaryArchiveReader::create(archive_stream);

	// Read the session metadata transcription from the archive.
	// Note: We don't close the archive reader because we are not reading the session 'data'
	// transcription and closing might give a not-at-end-of-archive error.
	const GPlatesScribe::Transcription::non_null_ptr_type transcription_metadata =
			archive_reader->read_transcription();

	// We can close the project file now that we've read the session metadata transcription from it.
	project_file.close();

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
	boost::optional<QStringList> all_file_paths_when_saved =
			GPlatesScribe::TranscribeUtils::load_file_paths(
					scribe_metadata,
					TRANSCRIBE_SOURCE,
					"all_file_paths",
					// Note that we don't add/remove Windows drive letters or share names because
					// we want to compare the project filename on the system the project was saved on
					// with the data filenames on the same save system...
					false/*convert*/);
	if (!all_file_paths_when_saved)
	{
		// We already have 'loaded_files' but they were converted (ie, not exactly the same as
		// when the project was saved) - so we'll load them again without conversion.
		all_file_paths_when_saved = GPlatesScribe::TranscribeUtils::load_file_paths(
				scribe_metadata,
				TRANSCRIBE_SOURCE,
				"loaded_files",
				false/*convert*/);
		GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
				all_file_paths_when_saved,
				GPLATES_ASSERTION_SOURCE);
	}

	// Load the filename of the project when it was saved.
	// This is used to detect if the project file has moved location so we can see which
	// data files have also moved to remain in the same relative location to the project file.
	boost::optional<QString> project_filename_when_saved =
			GPlatesScribe::TranscribeUtils::load_file_path(
					scribe_metadata,
					TRANSCRIBE_SOURCE,
					"original_project_filename",
					// Note that we don't add/remove Windows drive letter or share name because
					// we want to compare the project filename on the system the project was saved on
					// with the data filenames on the same save system...
					false/*convert*/);
	GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
			project_filename_when_saved,
			GPLATES_ASSERTION_SOURCE);

	// Make sure the metadata transcription is complete to ensure the metadata was restored correctly.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(
			new ProjectSession(
					project_filename,
					project_filename_when_saved.get(),
					time,
					loaded_files.get(),
					all_file_paths_when_saved.get()));
}


GPlatesPresentation::ProjectSession::non_null_ptr_type
GPlatesPresentation::ProjectSession::save_session(
		QString project_filename)
{
	// Make sure project filename is an absolute path.
	project_filename = QFileInfo(project_filename).absoluteFilePath();

	//
	// Session data.
	//

	// The scribe to save the session data.
	GPlatesScribe::Scribe scribe_data;

	// Record all saved file paths (whilst transcribing session data).
	//
	// Note: This is not just feature collection files. Can be any file (eg, CPT file).
	GPlatesScribe::TranscribeContext<GPlatesScribe::TranscribeUtils::FilePath> transcribe_file_path_context;
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<GPlatesScribe::TranscribeUtils::FilePath>
			transcribe_file_path_context_guard(scribe_data, transcribe_file_path_context);

	// Transcribe the session state.
	// Returns the loaded feature collection filenames and all transcribed filenames
	// (including non-feature-collection filenames such as CPT filenames).
	const QStringList loaded_files = TranscribeSession::save(scribe_data);

	// All saved file paths (transcribed while saving session data).
	// These are unique (and sorted) transcribed file paths.
	const QStringList all_file_paths = transcribe_file_path_context.get_file_paths();

	// Make sure the saved transcription is complete.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_data.is_transcription_complete(),
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

	// Save project filename.
	// This is used to detect if the project file has moved location so we can see which
	// data files have also moved to remain in the same relative location to the project file.
	GPlatesScribe::TranscribeUtils::save_file_path(
			scribe_metadata,
			TRANSCRIBE_SOURCE,
			project_filename,
			"original_project_filename");

	// Make sure the metadata transcription is complete otherwise the metadata will be incorrectly
	// restored when the archive is loaded.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe_metadata.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	//
	// Write session metadata/data to buffer.
	//

	QBuffer archive;
	archive.open(QBuffer::WriteOnly);

	QDataStream archive_stream(&archive);

	GPlatesScribe::ArchiveWriter::non_null_ptr_type archive_writer =
			GPlatesScribe::BinaryArchiveWriter::create(archive_stream);

	// Write the session metadata transcription to the archive.
	archive_writer->write_transcription(*scribe_metadata.get_transcription());

	// Write the session data transcription to the archive.
	archive_writer->write_transcription(*scribe_data.get_transcription());

	archive_writer->close();
	archive.close();

	//
	// Save project buffer to file.
	//

	// Open the project file for writing.
	QFile project_file(project_filename);
	if (!project_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(
				GPLATES_EXCEPTION_SOURCE,
				project_filename);
	}

	// Write the complete buffer to the project file.
	if (project_file.write(archive.data()) < 0)
	{
		throw GPlatesFileIO::ErrorOpeningFileForWritingException(
				GPLATES_EXCEPTION_SOURCE,
				project_filename);
	}
	
	project_file.close();

	return non_null_ptr_type(
			new ProjectSession(
					project_filename,
					project_filename/*project_filename_when_saved*/,
					time,
					loaded_files,
					all_file_paths,
					// Record the current session state so we can later detect any changes in session state...
					scribe_data.get_transcription()));
}


GPlatesPresentation::ProjectSession::ProjectSession(
		const QString &project_filename_,
		const QString &project_filename_when_saved_,
		const QDateTime &time_,
		const QStringList &filenames_,
		const QStringList &all_file_paths_when_saved_,
		boost::optional<GPlatesScribe::Transcription::non_null_ptr_to_const_type> last_saved_or_restored_session_state) :
	Session(time_, filenames_),
	d_project_filename(project_filename_),
	d_project_filename_when_saved(project_filename_when_saved_),
	d_all_file_paths_when_saved(all_file_paths_when_saved_),
	d_last_saved_or_restored_session_state(last_saved_or_restored_session_state)
{  }


bool
GPlatesPresentation::ProjectSession::has_project_file_moved() const
{
	return d_project_filename !=
			// Add/remove Windows driver letter or share name if needed...
			GPlatesScribe::TranscribeUtils::convert_file_path(d_project_filename_when_saved);
}


void
GPlatesPresentation::ProjectSession::get_absolute_file_paths(
		QStringList &existing_absolute_file_paths,
		QStringList &missing_absolute_file_paths) const
{
	const int num_file_paths = d_all_file_paths_when_saved.size();
	for (int n = 0; n < num_file_paths; ++n)
	{
		const QString absolute_file_path =
				GPlatesScribe::TranscribeUtils::convert_file_path(d_all_file_paths_when_saved[n]);

		if (QFileInfo(absolute_file_path).exists())
		{
			existing_absolute_file_paths.append(absolute_file_path);
		}
		else // missing...
		{
			missing_absolute_file_paths.append(absolute_file_path);
		}
	}
}


void
GPlatesPresentation::ProjectSession::get_relative_file_paths(
		QStringList &existing_relative_file_paths,
		QStringList &missing_relative_file_paths) const
{
	const int num_file_paths = d_all_file_paths_when_saved.size();
	for (int n = 0; n < num_file_paths; ++n)
	{
		const QString file_path_relative_to_project =
				GPlatesScribe::TranscribeUtils::convert_file_path_relative_to_project(
						d_all_file_paths_when_saved[n],
						d_project_filename_when_saved,
						d_project_filename);
		if (QFileInfo(file_path_relative_to_project).exists())
		{
			existing_relative_file_paths.append(file_path_relative_to_project);
		}
		else // missing...
		{
			missing_relative_file_paths.append(file_path_relative_to_project);
		}
	}
}


void
GPlatesPresentation::ProjectSession::set_load_relative_file_paths(
		bool load_relative_file_paths)
{
	if (load_relative_file_paths)
	{
		d_load_files_relative_to_project = std::make_pair(
				d_project_filename_when_saved,
				d_project_filename/*project_file_path_when_loaded*/);
	}
	else
	{
		d_load_files_relative_to_project = boost::none;
	}
}


void
GPlatesPresentation::ProjectSession::set_remapped_file_paths(
		boost::optional< QMap<QString/*missing*/, QString/*existing*/> > file_path_remapping)
{
	d_file_path_remapping = file_path_remapping;
}


bool
GPlatesPresentation::ProjectSession::has_session_state_changed() const
{
	if (!d_last_saved_or_restored_session_state)
	{
		// Only get here if we were created as a restore session but haven't yet been restored.
		return false;
	}

	if (d_file_path_remapping &&
		!d_file_path_remapping->isEmpty())
	{
		// The file paths have been remapped but those changes haven't yet been saved.
		return true;
	}

	// Save the current session state to a transcription.
	GPlatesScribe::Scribe scribe_current_state;
	TranscribeSession::save(scribe_current_state);

	// Compare the current session state with the last saved/restored session state to see if they're different.
	//
	// NOTE: Two transcriptions only compare equal if they were transcribed in the same way
	// (objects transcribed in the same order, etc). This usually only happens when *saving*
	// the same session state using the same code path. As a result this can be used to save
	// session state at two different times and comparing them to see if any session state has changed.
	// For other comparisons it might pay to implement a separate 'are_equivalent()' method and
	// even provide composite objects tags to include/exclude in the comparison.
	return *scribe_current_state.get_transcription() !=
			*d_last_saved_or_restored_session_state.get();
}


void
GPlatesPresentation::ProjectSession::restore_session() const
{
	// Project sessions were introduced after old version 3 so we don't need to worry
	// about the deprecated method of restoring sessions in versions 0 to 3.

	//
	// Setup project file for reading session metadata/data.
	//

	// Open the project file for reading.
	QFile project_file(d_project_filename);
	if (!project_file.open(QIODevice::ReadOnly))
	{
		throw GPlatesFileIO::ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE,
				d_project_filename);
	}

	QDataStream archive_stream(&project_file);

	GPlatesScribe::ArchiveReader::non_null_ptr_type archive_reader =
			GPlatesScribe::BinaryArchiveReader::create(archive_stream);

	// Read the session metadata transcription from the archive.
	const GPlatesScribe::Transcription::non_null_ptr_type transcription_metadata =
			archive_reader->read_transcription();

	// Read the session data transcription from the archive (the second transcription in archive).
	const GPlatesScribe::Transcription::non_null_ptr_type transcription_data =
			archive_reader->read_transcription();

	// We close the archive reader because we have read both session 'metadata' and 'data' transcriptions.
	// And we want to check we've correctly reached the end of the archive.
	archive_reader->close();

	// We can close the project file now that we've read the session data transcription from it.
	project_file.close();

	//
	// If requested, then load data files relative to the project being loaded (instead of the
	// location the project file was saved to).
	//
	// Note that this is also needed for the metadata (not just the main session data) since the
	// metadata contains the file paths needed to load projects saved by GPlates 1.5.
	//
	// Also remap missing file paths (if any) to existing file paths.
	//

	GPlatesScribe::TranscribeContext<GPlatesScribe::TranscribeUtils::FilePath> transcribe_file_path_context;
	if (d_load_files_relative_to_project)
	{
		transcribe_file_path_context.set_load_relative_file_paths(d_load_files_relative_to_project.get());
	}
	transcribe_file_path_context.set_load_file_path_remapping(d_file_path_remapping);

	//
	// Session metadata.
	//
	// Note: This is actually only needed in case the project was saved by GPlates 1.5.
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
	// Note: 'load_files' is only needed for GPlates 1.5 projects (which only store the file paths
	// in the metadata). Current projects also store the file paths in the main session data.
	//
	// Note: We use the metadata "loaded_files" rather than 'Session::get_loaded_files()' on the
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

	//
	// Record the current session state so we can later compare to see if any changes to session state.
	//

	// Save the current session state to a transcription.
	//
	// NOTE: We can't rely on the 'transcription_data' transcription because it might have been
	// generated by a different version of GPlates (eg, a future version might add extra information or
	// a past version might be missing information). The transcription will get compared to what we
	// save with this current version of GPlates and so has to be compatible with that. Easiest way
	// to do this is to create a new transcription straight after the session has been loaded.
	GPlatesScribe::Scribe scribe_current_state;
	TranscribeSession::save(scribe_current_state);

	// Store the transcription for later comparison.
	d_last_saved_or_restored_session_state = scribe_current_state.get_transcription();
}
