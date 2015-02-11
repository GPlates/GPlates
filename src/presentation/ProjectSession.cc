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


GPlatesPresentation::ProjectSession::non_null_ptr_to_const_type
GPlatesPresentation::ProjectSession::create_restore_session(
		const QString &project_filename)
{
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
			GPLATES_ASSERTION_SOURCE,
			project_filename);

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

	return non_null_ptr_to_const_type(new ProjectSession(project_filename, time, loaded_files));
}


GPlatesPresentation::ProjectSession::non_null_ptr_to_const_type
GPlatesPresentation::ProjectSession::save_session(
		const QString &project_filename)
{
	//
	// Setup buffer for session metadata and data.
	//

	QBuffer archive;
	archive.open(QBuffer::WriteOnly);

	QDataStream archive_stream(&archive);

	GPlatesScribe::ArchiveWriter::non_null_ptr_type archive_writer =
			GPlatesScribe::BinaryArchiveWriter::create(archive_stream);

	//
	// Session metadata.
	//

	// The scribe to save the session metadata.
	GPlatesScribe::Scribe scribe_metadata;

	// The session metadata.
	const QDateTime time = QDateTime::currentDateTime();
	const QStringList loaded_files = TranscribeSession::get_save_session_files();
	// Make sure saved project filename is an absolute path.
	// We don't use this yet but might in future if handle case where a project file and
	// its data files are moved together (such that they're still relative to each other).
	const QString original_project_filename = QFileInfo(project_filename).absoluteFilePath();

	// Save the session metadata.

	scribe_metadata.save(TRANSCRIBE_SOURCE, time, "time");

	// Use 'TranscribeUtils::FilePath' to generate smaller archives/transcriptions.
	std::vector<GPlatesScribe::TranscribeUtils::FilePath> transcribe_loaded_files(
			loaded_files.begin(), loaded_files.end());
	scribe_metadata.transcribe(TRANSCRIBE_SOURCE, transcribe_loaded_files, "loaded_files");

	GPlatesScribe::TranscribeUtils::FilePath transcribe_original_project_filename(original_project_filename);
	scribe_metadata.transcribe(TRANSCRIBE_SOURCE, transcribe_original_project_filename, "original_project_filename");

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
	TranscribeSession::transcribe(scribe_data, loaded_files, project_filename);

	// Write the session data transcription to the archive.
	archive_writer->write_transcription(*scribe_data.get_transcription());

	//
	// Save project buffer to file.
	//

	archive_writer->close();
	archive.close();

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

	return non_null_ptr_to_const_type(
			new ProjectSession(
					project_filename,
					time,
					loaded_files));
}


GPlatesPresentation::ProjectSession::ProjectSession(
		const QString &project_filename_,
		const QDateTime &time_,
		const QStringList &filenames_) :
	Session(time_, filenames_),
	d_project_filename(project_filename_)
{  }


QStringList
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
			TranscribeSession::transcribe(scribe_data, loaded_files, d_project_filename);

	return files_not_loaded;
}
