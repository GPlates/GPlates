/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_FILEINFO_H
#define GPLATES_FILEIO_FILEINFO_H

#include <QFileInfo>
#include <QString>

#include "scribe/Transcribe.h"


namespace GPlatesFileIO
{
	class FileInfo;


	/**
	 * Returns true if the specified file exists.
	 */
	bool
	file_exists(
			const FileInfo &file_info);


	/**
	 * This function attempts to open the file for writing.
	 *
	 * If the file already exists it is left unchanged.
	 * If the file does not exist then this function will *not* leave behind an empty file upon returning.
	 */
	bool
	is_writable(
			const QString &filename);

	/**
	 * An overload of @a is_writable.
	 */
	bool
	is_writable(
			const QFileInfo &file_info);

	/**
	 * An overload of @a is_writable.
	 */
	bool
	is_writable(
			const FileInfo &file_info);


	/**
	 * Holds information associated with a loaded file.
	 *
	 * FileInfo is used to keep track of information about a file.
	 * It keeps track of basic file information using a QFileInfo object.
	 */
	class FileInfo
	{
	public:
		/**
		 * Construct a FileInfo for the given @a file_name.
		 * The @a file_name does not have to exist.
		 *
		 * Note that we disable caching inside QFileInfo (even though it can affect performance).
		 * This is done because it's possible that the cached state (inside QFileInfo) might not
		 * reflect the actual state. One option we could have taken would be to refresh (see
		 * QFileInfo::refresh()) the file information every time we make a change to the file.
		 * However this becomes quite troublesome to do throughout GPlates (and the copying of
		 * FileInfo objects makes it even harder). So the option we've taken is to disable caching.
		 * An example where caching can be problematic is creating a file that didn't exist when
		 * a FileInfo was constructed. If the FileInfo is asked if the file exists it will correctly
		 * report that it doesn't (and cache the result, if caching enabled). If the file (on disc)
		 * is subsequently created then the original FileInfo (if caching enabled) would incorrectly
		 * report the file as non-existent (because it looks up its cache rather than querying the OS).
		 */
		explicit
		FileInfo(
				const QString &file_name) :
			d_file_info(file_name)
		{
			d_file_info.setCaching(false);
		}


		/**
		 * Construct an empty FileInfo.
		 *
		 * This may seem a little weird, but is necessary to enable the user to create a
		 * new empty FeatureCollection within GPlates and have it show up in the list of
		 * active reconstructable files (so that it can be displayed on the globe, and
		 * saved via the ManageFeatureCollections dialog).
		 */
		FileInfo():
			d_file_info()
		{  }


		/**
		 * Return a string that can be used by the GUI to identify this file.
		 *
		 * If the filename hasn't been set in the constructor then the returned
		 * string will be empty.
		 */
		const QString
		get_display_name(
				bool use_absolute_path_name) const;


		/**
		 * Returns the file name up to (but not including) the last '.' character.
		 *
		 * As a special case, if the file name has a double-barrelled extension and
		 * the part after the last '.' is "gz" (e.g. ".tar.gz", ".gpml.gz"), the final
		 * two extensions are removed, e.g. for "foo.gpml.gz", "foo" is returned.
		 */
		const QString
		get_file_name_without_extension() const;


		/**
		 * Return a @a QFileInfo object.
		 */
		const QFileInfo &
		get_qfileinfo() const
		{
			return d_file_info;
		}

	private:
		QFileInfo d_file_info;

		//! Transcribe to/from serialization archives.
		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);

		// Only the scribe system should be able to transcribe.
		friend class GPlatesScribe::Access;
	};


	inline
	bool
	file_exists(
			const FileInfo &file_info)
	{
		return file_info.get_qfileinfo().exists();
	}

	inline
	bool
	is_writable(
			const QFileInfo &file_info)
	{
		return is_writable(file_info.filePath());
	}

	inline
	bool
	is_writable(
			const FileInfo &file_info)
	{
		return is_writable(file_info.get_qfileinfo());
	}
}

#endif // GPLATES_FILEIO_FILEINFO_H
