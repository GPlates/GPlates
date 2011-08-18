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


namespace GPlatesFileIO
{
	class FileInfo;


	/**
	 * Returns true if the specified file exists.
	 */
	bool
	file_exists(
			const FileInfo &file_info);


	// NOTE: This is known to not work correctly on Windows Vista and above when
	// saving to the Desktop and certain other directories (in particular, those
	// that the user can write to, but does not explicitly own). The test on
	// Windows should probably be on the fileName() not the path(). However, if
	// you are writing to a file, just try and open the file for writing, and if
	// it fails, then throw an exception (instead of explicitly checking whether
	// the file can be written to).
#if 0
	//@{
	/**
	 * Returns 'true' if specified file is writable.
	 *
	 * @param file_info file to test for writability.
	 */
	bool
	is_writable(
			const QFileInfo &file_info);


	bool
	is_writable(
			const FileInfo &file_info);
	//@}
#endif


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
		 */
		explicit
		FileInfo(
				const QString &file_name):
			d_file_info(file_name)
		{  }


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
	};


	inline
	bool
	file_exists(
			const FileInfo &file_info)
	{
		return file_info.get_qfileinfo().exists();
	}
}

#endif // GPLATES_FILEIO_FILEINFO_H
