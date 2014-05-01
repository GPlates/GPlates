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

#ifndef GPLATES_FILE_IO_GDALRASTERWRITER_H
#define GPLATES_FILE_IO_GDALRASTERWRITER_H

#include <map>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <QString>

#include "RasterWriter.h"

#include "property-values/RasterType.h"
#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

// Forward declaration.
class GDALDataset;
class GDALDriver;


namespace GPlatesFileIO
{
	/**
	 * Writes colour and numerical rasters using GDAL with support for georeferencing and spatial reference systems.
	 */
	class GDALRasterWriter :
			public RasterWriterImpl
	{
	public:

		/**
		 * Adds information about the formats supported by this writer.
		 */
		static
		void
		get_supported_formats(
				RasterWriter::supported_formats_type &supported_formats);


		GDALRasterWriter(
				const QString &filename,
				const RasterWriter::FormatInfo &format_info,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int num_raster_bands,
				GPlatesPropertyValues::RasterType::Type raster_band_type);

		~GDALRasterWriter();

		virtual
		bool
		can_write() const;

		virtual
		void
		set_georeferencing(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing);

		virtual
		void
		set_spatial_reference_system(
				const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type& srs);

		virtual
		bool
		write_region_data(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &region_data,
				unsigned int band_number,
				unsigned int x_offset,
				unsigned int y_offset);

		virtual
		bool
		write_file();

	private:

		/**
		 * Visits a numerical raw raster and writes its data to our (in-memory) raster.
		 */
		class WriteNumericalRegionDataVisitorImpl
		{
		public:

			WriteNumericalRegionDataVisitorImpl(
					GDALDataset *in_memory_dataset,
					unsigned int band_number,
					GPlatesPropertyValues::RasterType::Type raster_band_type,
					boost::optional<double> &band_no_data_value,
					unsigned int x_offset,
					unsigned int y_offset) :
				d_in_memory_dataset(in_memory_dataset),
				d_band_number(band_number),
				d_raster_band_type(raster_band_type),
				d_band_no_data_value(band_no_data_value),
				d_x_offset(x_offset),
				d_y_offset(y_offset),
				d_wrote_region(false)
			{  }

			//! Returns true if region data was successfully written during visitation.
			bool
			wrote_region() const
			{
				return d_wrote_region;
			}

		private:

			template <class RawRasterType>
			void
			do_visit(
					RawRasterType &region_data,
					typename boost::disable_if_c<RawRasterType::has_data && RawRasterType::has_no_data_value>::type *dummy = NULL)
			{
				// Default case: do nothing.
				d_wrote_region = false;
			}


			// All numerical non-proxied data formats support data and a no-data value.
			template <class RawRasterType>
			void
			do_visit(
					RawRasterType &region_data,
					typename boost::enable_if_c<RawRasterType::has_data && RawRasterType::has_no_data_value>::type *dummy = NULL)
			{
				typedef typename RawRasterType::element_type region_element_type;

				// If the raster is floating-point then the region data can be integer or floating-point.
				// However, if the raster is integer then the region data must also be integer.
				// This is because floating-point region data always has NaN as a no-data value and if we
				// convert it to integer then we must select an integer no-data value but we don't know
				// which integer to pick (the caller knows their data range better and is better suited to
				// choose a no-data value - so we force them to use integer region raw rasters since those
				// have a no-data value attached).

				const GPlatesPropertyValues::RasterType::Type region_raster_band_type =
						GPlatesPropertyValues::RasterType::get_type_as_enum<region_element_type>();

				if (GPlatesPropertyValues::RasterType::is_floating_point(region_raster_band_type) &&
					GPlatesPropertyValues::RasterType::is_integer(d_raster_band_type))
				{
					qWarning() << "Cannot write floating-point region data to an integer GDAL raster.";
					d_wrote_region = false;

					return;
				}

				if (GPlatesPropertyValues::RasterType::is_integer(region_raster_band_type) &&
					GPlatesPropertyValues::RasterType::is_floating_point(d_raster_band_type))
				{
					// Convert the integer data to floating-point (this also converts any no-data pixels).
					// We use 'double' instead of 'float' in order to exactly capture 32-bit integers
					// (which can be represented exactly in 'double' but not in 'float').
					// The GDAL dataset will convert back to 'float' if our target raster is 'float'.
					GPlatesPropertyValues::DoubleRawRaster::non_null_ptr_type double_region_data =
							GPlatesPropertyValues::RawRasterUtils::convert_integer_raster_to_float_raster<
									RawRasterType, GPlatesPropertyValues::DoubleRawRaster>(region_data);

					d_wrote_region = write_numerical_region_data(*double_region_data);

					return;
				}

				d_wrote_region = write_numerical_region_data(region_data);
			}

			friend class GPlatesPropertyValues::TemplatedRawRasterVisitor<WriteNumericalRegionDataVisitorImpl>;


			template <class RawRasterType>
			bool
			write_numerical_region_data(
					RawRasterType &region_data);


			GDALDataset *d_in_memory_dataset;
			unsigned int d_band_number;
			GPlatesPropertyValues::RasterType::Type d_raster_band_type;
			boost::optional<double> &d_band_no_data_value;
			unsigned int d_x_offset;
			unsigned int d_y_offset;

			bool d_wrote_region;
		};

		typedef GPlatesPropertyValues::TemplatedRawRasterVisitor<WriteNumericalRegionDataVisitorImpl>
				WriteNumericalRegionDataVisitor;


		/**
		 * Information not contained in GPlatesFileIO::RasterWriter::FormatInfo.
		 */
		struct InternalFormatInfo
		{
			InternalFormatInfo(
					const std::string &driver_name_,
					const std::vector<std::string> &creation_options_) :
				driver_name(driver_name_),
				creation_options(creation_options_)
			{  }

			//! GDAL driver name.
			std::string driver_name;

			//! Options passed to 'GDALDriver::CreateCopy()'. 
			std::vector<std::string> creation_options;
		};

		// Map format descriptions to internal format information.
		typedef std::map<QString, InternalFormatInfo> format_desc_to_internal_format_info_map_type;

		//! Track internal format information by the format description.
		static format_desc_to_internal_format_info_map_type s_format_desc_to_internal_format_info_map;


		/**
		 * Adds the supported format information and records format-description to internal format info mapping.
		 */
		static
		void
		add_supported_format(
				GPlatesFileIO::RasterWriter::supported_formats_type &supported_formats,
				const QString &filename_extension,
				const QString &format_description,
				const QString &format_mime_type,
				const char *driver_name,
				const std::vector<std::string> &creation_options = std::vector<std::string>());


		/**
		 * Finds the internal format info from format description.
		 *
		 * @throws PreconditionViolationError if not found.
		 */
		static
		const InternalFormatInfo &
		get_internal_format_info(
				const GPlatesFileIO::RasterWriter::FormatInfo &format_info);



		bool
		write_colour_region_data(
				GPlatesPropertyValues::RawRaster &region_data,
				unsigned int x_offset,
				unsigned int y_offset);

		bool
		write_numerical_region_data(
				GPlatesPropertyValues::RawRaster &region_data,
				unsigned int band_number,
				unsigned int x_offset,
				unsigned int y_offset);


		QString d_filename;
		unsigned int d_num_raster_bands;
		GPlatesPropertyValues::RasterType::Type d_raster_band_type;

		//! The optional no-data value for each raster band.
		std::vector< boost::optional<double> > d_raster_band_no_data_values;

		//! Extra information concerning the raster format being written.
		InternalFormatInfo d_internal_format_info;

		/**
		 * Handle to the in-memory buffer. NULL indicates @a can_write will fail.
		 *
		 * Memory released by @a GDALClose call in our destructor.
		 */
		GDALDataset *d_in_memory_dataset;

		/**
		 * Used to copy the in-memory dataset to the file.
		 *
		 * Memory managed by GDAL.
		 */
		GDALDriver *d_file_driver;
	};
}

#endif // GPLATES_FILE_IO_GDALRASTERWRITER_H
