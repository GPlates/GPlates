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

#include <utility>
#include <map>
#include <boost/scoped_ptr.hpp>
#include <QRect>
#include <QString>

#include "RasterBandReaderHandle.h"
#include "ReadErrorAccumulation.h"
#include "ReadErrorOccurrence.h"

#include "property-values/RasterType.h"
#include "property-values/RawRaster.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesFileIO
{
	// Forward declarations.
	class RasterBandReader;
	class RasterReaderImpl;

	class RasterReader :
			public GPlatesUtils::ReferenceCount<RasterReader>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RasterReader> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterReader> non_null_ptr_to_const_type;

		/**
		 * Libraries that we use to read in rasters.
		 */
		enum FormatHandler
		{
			IMAGEMAGICK,
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
					const QString &mime_type_,
					FormatHandler handler_) :
				description(description_),
				mime_type(mime_type_),
				handler(handler_)
			{  }

			QString description;
			QString mime_type;
			FormatHandler handler;
		};

		/**
		 * Returns a RasterReader to read data from @a filename.
		 *
		 * Errors encountered are added to @a read_errors if it is not NULL.
		 * @a read_errors is *not* stored in the RasterReader for reporting of
		 * errors in subsequent method calls.
		 */
		static
		non_null_ptr_type
		create(
				const QString &filename,
				ReadErrorAccumulation *read_errors = NULL);

		/**
		 * Returns the filename of the file that the RasterReader was created with.
		 */
		const QString &
		get_filename() const;

		/**
		 * Returns whether the file, as given in the constructor, is capable of
		 * yielding any raster data at all.
		 */
		bool
		can_read();

		/**
		 * Returns the number of bands in the raster.
		 *
		 * For single-band rasters, the number of bands is always 1.
		 *
		 * Returns 0 in case of error.
		 */
		unsigned int
		get_number_of_bands(
				ReadErrorAccumulation *read_errors = NULL);

		/**
		 * Returns the size (width by height) of the raster.
		 *
		 * Returns (0, 0) in case of error, or if the bands in the raster have
		 * different sizes.
		 */
		std::pair<unsigned int, unsigned int>
		get_size(
				ReadErrorAccumulation *read_errors = NULL);

		/**
		 * Returns the data type of the given @a band_number.
		 *
		 * @a band_number must be between 1 and @a get_number_of_bands inclusive.
		 *
		 * Returns UNKNOWN if the given @a band_number could not be read.
		 */
		GPlatesPropertyValues::RasterType::Type
		get_type(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors = NULL);

		/**
		 * Returns a proxied RawRaster, that can be used to get actual data
		 * from the given @a band_number at a later time.
		 *
		 * @a band_number must be between 1 and @a get_number_of_bands inclusive.
		 *
		 * Returns boost::none if the given @a band_number could not be read.
		 */
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_proxied_raw_raster(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors = NULL);

		/**
		 * Returns a non-proxied RawRaster, that contains data from the given
		 * @a region in the given @a band_number.
		 *
		 * If @a region is a null region (default), the entire band is returned
		 * without cropping.
		 *
		 * @a band_number must be between 1 and @a get_number_of_bands inclusive.
		 *
		 * Returns boost::none if the given @a band_number could not be read.
		 */
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_raw_raster(
				unsigned int band_number,
				const QRect &region = QRect(),
				ReadErrorAccumulation *read_errors = NULL);

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


		///////////////////////////////////////////////////////////////////////
		// The following static functions are deprecated.
		///////////////////////////////////////////////////////////////////////

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

	private:

		RasterReader(
				const QString &filename,
				ReadErrorAccumulation *read_errors);

		/**
		 * Returns a pointer to a copy of the data contained within the given
		 * @a region in the given @a band_number.
		 *
		 * Ownership of the memory passes to the caller of this function.
		 *
		 * @a band_number must be between 1 and @a get_number_of_bands inclusive.
		 *
		 * Returns NULL if the band could not be read.
		 */
		void *
		get_data(
				unsigned int band_number,
				const QRect &region = QRect(),
				ReadErrorAccumulation *read_errors = NULL);

		RasterBandReaderHandle
		create_raster_band_reader_handle(
				unsigned int band_number);

		boost::scoped_ptr<RasterReaderImpl> d_impl;
		QString d_filename;

		friend class RasterBandReader;
	};


	class RasterReaderImpl :
			public boost::noncopyable
	{
	public:

		virtual
		~RasterReaderImpl();

		virtual
		bool
		can_read() = 0;

		virtual
		unsigned int
		get_number_of_bands(
				ReadErrorAccumulation *read_errors) = 0;

		virtual
		std::pair<unsigned int, unsigned int>
		get_size(
				ReadErrorAccumulation *read_errors) = 0;

		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_proxied_raw_raster(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors) = 0;

		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_raw_raster(
				unsigned int band_number,
				const QRect &region,
				ReadErrorAccumulation *read_errors) = 0;

		virtual
		GPlatesPropertyValues::RasterType::Type
		get_type(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors) = 0;

		virtual
		void *
		get_data(
				unsigned int band_number,
				const QRect &region,
				ReadErrorAccumulation *read_errors) = 0;
	};
}

#endif  // GPLATES_FILEIO_RASTERREADER_H
