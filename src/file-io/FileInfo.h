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
#include <QMap>
#include <QString>

#include "model/FeatureCollectionHandle.h"


namespace GPlatesFileIO
{
	class FileInfo;


	/**
	 * Creates a copy of @a other but with a different filename.
	 */
	FileInfo
	create_copy_with_new_filename(
			const QString &filename,
			const FileInfo &other);


	/**
	 * Returns true if the specified file exists.
	 */
	bool
	file_exists(
			const FileInfo &file_info);


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
		 * Return a @a QFileInfo object.
		 */
		const QFileInfo &
		get_qfileinfo() const
		{
			return d_file_info;
		}


		/**
		 * Get the model-to-shapefile mapping.
		 *
		 * This is currently stored here - however it could be stored
		 * in @a File (not sure which is better just yet since this
		 * @a FileInfo is stored inside a @a File anyway).
		 */
		const QMap< QString,QString >& 
		get_model_to_shapefile_map() const
		{
			return d_model_to_shapefile_map;
		}


		/**
		 * Set the model-to-shapefile mapping.
		 *
		 * This is currently stored here - however it could be stored
		 * in @a File (not sure which is better just yet since this
		 * @a FileInfo is stored inside a @a File anyway).
		 */
		void
		set_model_to_shapefile_map(
			QMap< QString, QString > model_to_shapefile_map) const
		{
			d_model_to_shapefile_map = model_to_shapefile_map;
		}

	private:
		QFileInfo d_file_info;

		/**
		 * A map of model property names to shapefile attributes names.                                                                     
		 */
		mutable QMap< QString,QString > d_model_to_shapefile_map;
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
			const FileInfo &file_info)
	{
		return is_writable(file_info.get_qfileinfo());
	}
}

#endif // GPLATES_FILEIO_FILEINFO_H
