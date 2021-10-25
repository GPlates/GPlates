/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_RASTERWRITER_H
#define GPLATES_FILE_IO_RASTERWRITER_H

#include <utility>
#include <map>
#include <utility> // For std::pair
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QRect>
#include <QString>

#include "property-values/Georeferencing.h"
#include "property-values/RasterType.h"
#include "property-values/RawRaster.h"
#include "property-values/SpatialReferenceSystem.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesFileIO
{
	// Forward declarations.
	class RasterWriterImpl;

	class RasterWriter :
			public GPlatesUtils::ReferenceCount<RasterWriter>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RasterWriter> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterWriter> non_null_ptr_to_const_type;

		/**
		 * Libraries that we use to write out rasters.
		 */
		enum FormatHandler
		{
			RGBA,
			GDAL,

			NUM_FORMAT_HANDLERS // This must be last.
		};

		/**
		 * Holds information about a supported format.
		 */
		struct FormatInfo
		{
			FormatInfo(
					const QString &description_,
					const QString &mime_type_,
					FormatHandler handler_,
					const std::vector<GPlatesPropertyValues::RasterType::Type> band_types_) :
				description(description_),
				mime_type(mime_type_),
				handler(handler_),
				band_types(band_types_)
			{  }

			QString description;
			QString mime_type;
			FormatHandler handler;
			//! Supported band types.
			std::vector<GPlatesPropertyValues::RasterType::Type> band_types;
		};

		//! Typedef for a map of filename extensions for format information.
		typedef std::map<QString, FormatInfo> supported_formats_type;


		/**
		 * Retrieves information about formats supported when writing rasters.
		 *
		 * The returned map is a mapping from file extension to information about the format.
		 * Note that "jpg" and "jpeg" appear as two separate elements in the map.
		 */
		static
		const supported_formats_type &
		get_supported_formats();

		/**
		 * Retrieves information about formats supported by @a format_handler when writing rasters.
		 */
		static
		const supported_formats_type &
		get_supported_formats(
				FormatHandler format_handler);


		/**
		 * Retrieves the file format information that would be used to write a raster to @a filename, or
		 * none if the filename extension is not supported.
		 */
		static
		boost::optional<const FormatInfo &>
		get_format(
				const QString &filename);


		/**
		 * Returns a RasterWriter to write data of the specified dimensions to @a filename.
		 *
		 * @a raster_band_type should match one of the band types supported by the file format.
		 * This can be determined with @a get_format.
		 *
		 * All raster bands will have the same data type @a raster_band_type.
		 * RGBA format handlers only accept a single band and it must be a colour (RGBA) band.
		 * GDAL format handlers can generally accept either a single colour (RGBA) band or
		 * multiple non-colour bands (in the case of a single colour band it actually gets stored as
		 * four bands R, G, B and A in the file but our reading/writing API considers it a single band).
		 *
		 * We're limiting all bands to have the same data type (even though GDAL does support mixed
		 * types) because it appears some common file formats do not support it...
		 *    http://lists.osgeo.org/pipermail/gdal-dev/2010-August/025657.html
		 *
		 * Note that the no-data value (applicable to integer/floating-point rasters only) is
		 * determined by the region data written (see @a write_region_data).
		 */
		static
		non_null_ptr_type
		create(
				const QString &filename,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int num_raster_bands,
				GPlatesPropertyValues::RasterType::Type raster_band_type);


		/**
		 * Returns whether any data can be written to the internal buffer.
		 *
		 * This does not take into account the file itself and whether it can be written to
		 * given its filename path (see @a write_file).
		 *
		 * This can fail, for example, if the image dimensions are too large (memory allocation error) or
		 * the raster band type or number of bands (specified in @a create) is not supported by the
		 * format handler type associated with the filename.
		 */
		bool
		can_write() const;


		/**
		 * Returns the filename of the file that the RasterWriter was created with.
		 */
		const QString &
		get_filename() const;


		/**
		 * Returns the size (width by height) of the raster.
		 */
		std::pair<unsigned int, unsigned int>
		get_size();


		/**
		 * Returns the number of bands as specified to @a create.
		 */
		unsigned int
		get_number_of_bands();


		/**
		 * Returns the raster type of each band.
		 */
		GPlatesPropertyValues::RasterType::Type
		get_raster_band_type();


		/**
		 * Sets the georeferencing of pixel/line raster data.
		 *
		 * The default is no georeferencing.
		 */
		void
		set_georeferencing(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing);


		/**
		 * Sets the raster's spatial reference system.
		 *
		 * The default is no spatial reference system.
		 */
		void
		set_spatial_reference_system(
				const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type& srs);


		/**
		 * Writes the non-proxied RawRaster data @a region_data to the specified offset
		 * (in the raster) of the specified band.
		 *
		 * If the 'raster_band_type' argument passed into @a create is RGBA, then the type of data in
		 * @a region_data must also be RGBA (ie, must be Rgba8RawRaster).
		 * If the 'raster_band_type' argument passed into @a create is an integer type, then the
		 * type of data in @a region_data can be any integer type (eg, UInt16RawRaster).
		 * If the 'raster_band_type' argument passed into @a create is a floating-point type, then
		 * the type of data in @a region_data can be any integer *or* floating-point type
		 * (eg, FloatRawRaster or UInt16RawRaster).
		 *
		 * Note that multiple calls per band can be made when the raster is written in sections.
		 *
		 * If any regions written to a raster have a no-data value then it is set on the raster.
		 * Note that only integer and floating-point rasters can have a no-data value (RGBA rasters
		 * do not have a no-data value). Floating-point rasters always have the NaN no-data value
		 * whereas integer rasters have an optional no-data value that can be any integer. So regions
		 * written to integer rasters can either have a no-data value or not, but those regions with
		 * no-data values must all have the *same* no-data value - essentially when the first region
		 * with a no-data value is encountered, that no-data value will be set on the raster and
		 * subsequent regions must match - and if none of the regions have a no-data value then the
		 * entire raster will not have one either.
		 *
		 * @a x_offset and @a y_offset are relative to the top-left corner of the raster.
		 *
		 * NOTE: @a band_number must be between 1 and @a get_number_of_bands inclusive.
		 *
		 * Returns false if:
		 *  - raw raster format is not supported (eg, writing non-colour data to a RGBA format handler), or
		 *  - band number is out-of-range, or
		 *  - region of data being written is outside the raster dimensions.
		 */
		bool
		write_region_data(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &region_data,
				unsigned int band_number,
				unsigned int x_offset = 0,
				unsigned int y_offset = 0);


		/**
		 * The final write to the filename passed into @a create.
		 *
		 * The data written with calls to @a write_region_data, and the georeferencing and
		 * spatial reference system (for GDAL format handlers), are written to file.
		 *
		 * Any regions of the raster that are not written to (by calls to @a write_region_data)
		 * will contain undefined pixel values.
		 *
		 * This should only be called once at the end.
		 * If it is not called then the file is not written.
		 * And after calling this method, subsequent calls to @a can_write will fail.
		 *
		 * Returns false if there was an error writing to the file.
		 */
		bool
		write_file();

	private:

		RasterWriter(
				const QString &filename,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int num_raster_bands,
				GPlatesPropertyValues::RasterType::Type raster_band_type);

		boost::scoped_ptr<RasterWriterImpl> d_impl;
		QString d_filename;
		unsigned int d_width;
		unsigned int d_height;
		unsigned int d_num_bands;
		GPlatesPropertyValues::RasterType::Type d_band_type;
	};


	class RasterWriterImpl :
			public boost::noncopyable
	{
	public:

		virtual
		~RasterWriterImpl();

		virtual
		bool
		can_write() const = 0;

		virtual
		void
		set_georeferencing(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing) = 0;

		virtual
		void
		set_spatial_reference_system(
				const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type& srs) = 0;

		virtual
		bool
		write_region_data(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &region_data,
				unsigned int band_number,
				unsigned int x_offset,
				unsigned int y_offset) = 0;

		virtual
		bool
		write_file() = 0;

	protected:

		RasterWriterImpl()
		{  }

	};
}

#endif // GPLATES_FILE_IO_RASTERWRITER_H
