/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <QDir>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "TranscribeUtils.h"

#include "ScribeInternalAccess.h"


namespace GPlatesScribe
{
	namespace
	{
		/**
		 * Regular expression for a Windows drive letter (eg, "C:/").
		 */
		const QRegExp WINDOWS_DRIVE_LETTER_REGEXP("^[a-zA-Z]\\:/");

		/**
		 * Regular expression for a Windows share name (eg, "//sharename/").
		 */
		const QRegExp WINDOWS_SHARE_NAME_REGEXP("^//[^/]+/");


		/**
		 * Returns the directory path of the specified file path.
		 *
		 * Absolute paths starting with '/' have an empty string as first element of path.
		 * Absolute paths starting with 'C:/', for example, have 'C:' as first element of path.
		 * Absolute paths starting with '//sharename/', for example, have '//sharename' as first element of path.
		 */
		QStringList
		get_dir_path(
				QString file_path,
				boost::optional<QString &> file_name = boost::none)
		{
			QStringList dir_path;

			if (WINDOWS_DRIVE_LETTER_REGEXP.indexIn(file_path) >= 0)
			{
				// Split "C:/dir/file.txt" into "C:" and "dir/file.txt" for example.

				const QString drive_letter = file_path.left(WINDOWS_DRIVE_LETTER_REGEXP.matchedLength() - 1);

				// Make the drive letters are uppercase so they compare properly later on.
				// They should already be uppercase if QFileInfo::absoluteFilePath() was used to create them.
				// But we make sure anyway.
				dir_path.append(drive_letter.toUpper());

				file_path = file_path.mid(WINDOWS_DRIVE_LETTER_REGEXP.matchedLength());
			}
			else if (WINDOWS_SHARE_NAME_REGEXP.indexIn(file_path) >= 0)
			{
				// Split "//sharename/dir/file.txt" into "//sharename" and "dir/file.txt" for example.

				const QString share_name = file_path.left(WINDOWS_SHARE_NAME_REGEXP.matchedLength() - 1);

				dir_path.append(share_name);

				file_path = file_path.mid(WINDOWS_SHARE_NAME_REGEXP.matchedLength());
			}

			dir_path = dir_path + file_path.split('/');

			// Return filename (without path) if requested.
			if (file_name)
			{
				file_name.get() = dir_path.last();
			}

			dir_path.pop_back(); // Remove filename to get directory containing file.

			return dir_path;
		}
	}
}


void
GPlatesScribe::TranscribeUtils::FilePath::set_file_path(
		const QString &file_path)
{
	d_split_paths = file_path.split('/');
}


QString
GPlatesScribe::TranscribeUtils::FilePath::get_file_path(
		bool convert) const
{
	QString file_path = d_split_paths.join("/");

	if (convert)
	{
		file_path = TranscribeUtils::convert_file_path(file_path);
	}

	return file_path;
}


GPlatesScribe::TranscribeResult
GPlatesScribe::TranscribeUtils::FilePath::transcribe(
		Scribe &scribe,
		bool transcribed_construct_data)
{
	//
	// Due to tracking issues, where an untracked sequence (eg, QStringList)
	// still has tracked objects (but we don't want to track them), we will explicitly
	// use the sequence protocol (in ObjectTag) to transcribe our QStringList data member
	// and turn tracking off for each item in it.
	//
	// Update: Actually this is no longer the case (untracked sequences now have untracked elements).
	// But to keep the transcription compatible with previous versions (ie, avoid adding an extra
	// object tag such as 'split_paths' for our QStringList) we'll continue to transcribe each
	// QStringList data member explicitly.
	//

	if (scribe.is_saving())
	{
		unsigned int split_paths_size = d_split_paths.size();
		scribe.transcribe(TRANSCRIBE_SOURCE, split_paths_size, ObjectTag().sequence_size());

		for (unsigned int p = 0; p < split_paths_size; ++p)
		{
			scribe.transcribe(TRANSCRIBE_SOURCE, d_split_paths[p], ObjectTag()[p]);
		}
	}
	else // loading...
	{
		d_split_paths.clear();

		unsigned int split_paths_size;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, split_paths_size, ObjectTag().sequence_size()))
		{
			return scribe.get_transcribe_result();
		}

		for (unsigned int p = 0; p < split_paths_size; ++p)
		{
			QString split_path;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, split_path, ObjectTag()[p]))
			{
				return scribe.get_transcribe_result();
			}

			d_split_paths.append(split_path);
		}
	}


	//
	// If requested then record saved/loaded file path (and optionally adjust loaded file path).
	//
	// The client requests this by pushing a transcribe context.
	boost::optional<TranscribeContext<FilePath> &> transcribe_context = scribe.get_transcribe_context<FilePath>();
	if (transcribe_context)
	{
		if (scribe.is_loading())
		{
			if (transcribe_context->d_load_relative_file_paths)
			{
				//
				// Convert transcribed file paths to be relative to the loaded project file rather than
				// relative to the project file location when it was saved. If file path cannot be
				// converted to be relative then the original path is used.
				//
				// Note: Resource files (eg, ":/age.cpt") and empty file paths are not converted.
				//
				const QString file_path_relative_to_project =
						TranscribeUtils::convert_file_path_relative_to_project(
								// Note that we don't add/remove Windows drive letter or share name because
								// we want to compare the project filename on the system the project was saved on
								// with the data filenames on the same save system.
								get_file_path(false/*convert*/)/*file_path_when_saved*/,
								transcribe_context->d_load_relative_file_paths->first/*project_file_path_when_saved*/,
								transcribe_context->d_load_relative_file_paths->second/*project_file_path_when_loaded*/);
				set_file_path(file_path_relative_to_project);
			}

			if (transcribe_context->d_load_file_path_remapping)
			{
				//
				// Remap the file path (from an missing file to an existing file) if the filename
				// is in the remapping map.
				//
				// NOTE: This needs to be done after converting to relative file paths (if requested)
				// since the remapping is from file paths relative to the loaded project file location.
				//
				QMap<QString, QString>::const_iterator remapped_file_path_iter =
						transcribe_context->d_load_file_path_remapping->find(
								// Need to use the converted file path (ie, file path on local system)
								// since that's what's stored in the remapping map...
								get_file_path(true/*convert*/));
				if (remapped_file_path_iter != transcribe_context->d_load_file_path_remapping->end())
				{
					set_file_path(remapped_file_path_iter.value());
				}
			}
		}

		// Record the transcribed file path.
		transcribe_context->d_file_paths.append(get_file_path(false/*convert*/));
	}

	return TRANSCRIBE_SUCCESS;
}


QStringList
GPlatesScribe::TranscribeContext<GPlatesScribe::TranscribeUtils::FilePath>::get_file_paths(
		bool convert,
		bool exclude_resource_and_empty_file_paths) const
{
	QStringList file_paths = d_file_paths;

	// Exclude resource and empty file paths if requested.
	if (exclude_resource_and_empty_file_paths)
	{
		for (int n = 0; n < file_paths.size(); )
		{
			if (TranscribeUtils::is_resource_file_path_or_empty_path(file_paths[n]))
			{
				file_paths.removeAt(n);
			}
			else
			{
				++n;
			}
		}
	}

	// Convert if requested.
	if (convert)
	{
		for (int n = 0; n < file_paths.size(); ++n)
		{
			file_paths[n] = TranscribeUtils::convert_file_path(file_paths[n]);
		}
	}

	// Make unique.
	file_paths = QList<QString>::fromSet(QSet<QString>::fromList(file_paths));

	// Sort.
	file_paths.sort();

	return file_paths;
}


void
GPlatesScribe::TranscribeUtils::save_file_path(
		Scribe &scribe,
		const GPlatesUtils::CallStack::Trace &transcribe_source,
		const QString &file_path_tag,
		const ObjectTag &object_tag)
{
	const FilePath transcribe_file_path(file_path_tag);

	scribe.save(transcribe_source, transcribe_file_path, object_tag);
}


boost::optional<QString>
GPlatesScribe::TranscribeUtils::load_file_path(
		Scribe &scribe,
		const GPlatesUtils::CallStack::Trace &transcribe_source,
		const ObjectTag &file_path_tag,
		bool convert)
{
	LoadRef<FilePath> transcribe_file_path = scribe.load<FilePath>(transcribe_source, file_path_tag);
	if (!transcribe_file_path.is_valid())
	{
		return boost::none;
	}

	return transcribe_file_path->get_file_path(convert);
}


void
GPlatesScribe::TranscribeUtils::save_file_paths(
		Scribe &scribe,
		const GPlatesUtils::CallStack::Trace &transcribe_source,
		const QStringList &file_paths,
		const ObjectTag &file_paths_tag)
{
	save_file_paths(scribe, transcribe_source, file_paths.constBegin(), file_paths.constEnd(), file_paths_tag);
}


boost::optional<QStringList>
GPlatesScribe::TranscribeUtils::load_file_paths(
		Scribe &scribe,
		const GPlatesUtils::CallStack::Trace &transcribe_source,
		const ObjectTag &file_paths_tag,
		bool convert)
{
	// Load number of file paths.
	unsigned int num_file_paths;
	if (!scribe.transcribe(transcribe_source, num_file_paths, file_paths_tag.sequence_size()))
	{
		return boost::none;
	}

	QStringList file_paths;

	for (unsigned int file_paths_index = 0; file_paths_index < num_file_paths; ++file_paths_index)
	{
		const boost::optional<QString> file_path =
				load_file_path(scribe, transcribe_source, file_paths_tag[file_paths_index], convert);
		if (!file_path)
		{
			return boost::none;
		}

		file_paths.append(file_path.get());
	}

	return file_paths;
}


QString
GPlatesScribe::TranscribeUtils::convert_file_path(
		const QString &file_path)
{
	// Resource files (eg, ":/age.cpt") and empty file paths are left unchanged.
	if (is_resource_file_path_or_empty_path(file_path))
	{
		return file_path;
	}

#if defined(Q_WS_WIN)

	// Add a Windows drive letter to absolute paths if necessary.
	if (file_path.startsWith('/') &&
		// But exclude sharenames ('//sharename/') since they are compatible with Windows...
		WINDOWS_SHARE_NAME_REGEXP.indexIn(file_path) < 0)
	{
		// Change "/dir/file.txt" into "C:/dir/file.txt" for example.
		return QDir::rootPath() + file_path.mid(1);
	}

#else // Mac or Linux...

	// Remove Windows drive letter if necessary.
	if (WINDOWS_DRIVE_LETTER_REGEXP.indexIn(file_path) >= 0)
	{
		// Change "C:/dir/file.txt" into "/dir/file.txt" for example.
		return '/' + file_path.mid(WINDOWS_DRIVE_LETTER_REGEXP.matchedLength());
	}
	else if (WINDOWS_SHARE_NAME_REGEXP.indexIn(file_path) >= 0)
	{
		// Change "//sharename/dir/file.txt" into "/dir/file.txt" for example.
		return '/' + file_path.mid(WINDOWS_SHARE_NAME_REGEXP.matchedLength());
	}

#endif

	return file_path;
}


QString
GPlatesScribe::TranscribeUtils::convert_file_path_relative_to_project(
		const QString &file_path_when_saved,
		const QString &project_file_path_when_saved,
		const QString &project_file_path_when_loaded)
{
	// Resource files (eg, ":/age.cpt") and empty file paths return unchanged.
	if (is_resource_file_path_or_empty_path(file_path_when_saved))
	{
		// Empty file paths and resource file paths don't need conversion (using 'convert_file_path()').
		return file_path_when_saved;
	}

	QStringList project_dir_path_when_saved_list = get_dir_path(project_file_path_when_saved);
	QStringList project_dir_path_when_loaded_list = get_dir_path(project_file_path_when_loaded);

	// If the directory of the project has not changed then the file path
	// relative to the saved and loaded projects will be the same.
	if (project_dir_path_when_loaded_list == project_dir_path_when_saved_list)
	{
		// Convert in case file path was saved on Windows and loading on Mac/Linux or vice versa.
		return convert_file_path(file_path_when_saved);
	}

	QString file_name;
	QStringList dir_path_when_saved_list = get_dir_path(file_path_when_saved, file_name);

	// The minimum size of the directory paths of the saved file and saved project file.
	const int dir_path_when_saved_size = dir_path_when_saved_list.size();
	const int project_dir_path_when_saved_size = project_dir_path_when_saved_list.size();
	const int min_saved_path_size = (dir_path_when_saved_size > project_dir_path_when_saved_size)
			? project_dir_path_when_saved_size
			: dir_path_when_saved_size;

	// Find the common path between the saved filename and saved project filename.
	int common_saved_path_size = 0;
	for ( ; common_saved_path_size < min_saved_path_size; ++common_saved_path_size)
	{
		if (dir_path_when_saved_list[common_saved_path_size] !=
			project_dir_path_when_saved_list[common_saved_path_size])
		{
			break;
		}
	}

	// If the drive letters or share names of paths saved on Windows are different then
	// we can't form a relative path, so just return the original save path.
	//
	// Note: For all paths the first element represents the drive letter or share name (on Windows)
	// or the empty string prior to root '/' (on Linux/Mac). So for paths saved on Linux/Mac the
	// empty string (prior to root '/') will always be common in both paths. However for Windows
	// we might get different drive letters or share names.
	if (common_saved_path_size == 0)
	{
		return convert_file_path(file_path_when_saved);
	}

	// Start with the path of the loaded project directory and build off that.
	QStringList file_path_when_loaded_list = project_dir_path_when_loaded_list;

	// Traverse backwards along the directory path until reach the directory path that both
	// the saved file and saved project file have in common.
	for (int remove_index = project_dir_path_when_saved_size - 1; remove_index >= common_saved_path_size; --remove_index)
	{
		// If we're trying to traverse beyond the root directory then we cannot form a relative path
		// so just return the original save path.
		//
		// Note: For all paths the first element represents the drive letter or share name (on Windows)
		// or the empty string prior to root '/' (on Linux/Mac). We don't want to remove that.
		if (file_path_when_loaded_list.size() <= 1)
		{
			return convert_file_path(file_path_when_saved);
		}

		file_path_when_loaded_list.pop_back();
	}

	// Traverse along the path of the saved file starting at the directory path that both
	// the saved file and saved project file have in common.
	for (int add_index = common_saved_path_size; add_index < dir_path_when_saved_size; ++add_index)
	{
		const QString add_path = dir_path_when_saved_list[add_index];
		file_path_when_loaded_list.push_back(add_path);
	}

	// Append the filename to the new path.
	file_path_when_loaded_list.append(file_name);

	// Rebuild the file path using directory separators.
	return file_path_when_loaded_list.join("/");
}


bool
GPlatesScribe::TranscribeUtils::is_resource_file_path_or_empty_path(
		const QString &file_path)
{
	return file_path.isEmpty() ||
		file_path.startsWith(":/") ||
		file_path.startsWith("qrc:///");
}
