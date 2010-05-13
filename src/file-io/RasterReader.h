/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_RASTERREADER_H
#define GPLATES_FILEIO_RASTERREADER_H

#include <map>
#include <QString>

#include "FileInfo.h"
#include "ReadErrorAccumulation.h"

#include "property-values/InMemoryRaster.h"


namespace GPlatesFileIO
{
	class RasterReader
	{
	public:

		/**
		 * Libraries that we use to read in rasters.
		 */
		enum FormatHandler
		{
			Q_IMAGE_READER,
			GDAL,
			UNKNOWN
		};

		/**
		 * Holds information about a supported format.
		 */
		struct FormatInfo
		{
			FormatInfo(
					const QString &description_,
					FormatHandler handler_) :
				description(description_),
				handler(handler_)
			{
			}

			QString description;
			FormatHandler handler;
		};

		/**
		 * Reads a raster image file and loads a raster in the corresponding InMemoryRaster
		 * instance.
		 *
		 * Returns true iff the raster was successfully read.
		 * 
		 * Once the model is able to store raster data, we can replace this interface with 
		 * one which takes a reference to the model instead of a reference to a raster, 
		 * so that we are consistent with the other file-input classes.  
		 */
		static
		bool
		read_file(
				const FileInfo &file_info,
				GPlatesPropertyValues::InMemoryRaster &raster,
				ReadErrorAccumulation &read_errors);

		/**
		 * Fill the time_dependent_raster_map with <time, filename> pairs. 
		 * This function will look for files of the form <root_name>-<time>.<ext>
		 * The first such file encountered will be used to establish the root_name.
		 * Any files matching the same pattern will be added to the time_dependent_raster_map.
		 *
		 * This currently only supports reading a directory of JPEG images.
		 */
		static
		void
		populate_time_dependent_raster_map(
				QMap<int, QString> &raster_map,
				QString directory_path,
				ReadErrorAccumulation &read_errors);	

		/**
		 * Given a time_dependent_raster_map and a reconstruction_time, this function will return 
		 * the filename whose time value is closest to the reconstruction time. 
		 */
		static
		QString
		get_nearest_raster_filename(
				const QMap<int, QString> &raster_map,
				double time);

		/**
		 * Retrieves information about formats supported when reading rasters.
		 *
		 * The returned map is a mapping from file extension to information about the format.
		 * Note that "jpg" and "jpeg" appear as two separate elements in the map.
		 */
		static
		const std::map<QString, FormatInfo> &
		get_supported_formats();

		/**
		 * Gets a string that can be used as the filter string in a QFileDialog.
		 *
		 * The first filter is an all-inclusive filter that matches all supported
		 * raster formats. The other filters are for the other formats, sorted in
		 * alphabetic order by description.
		 */
		static
		const QString &
		get_file_dialog_filters();
	};
}

#endif  // GPLATES_FILEIO_RASTERREADER_H
