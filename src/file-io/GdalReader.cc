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

#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

// Use this header instead of <cmath> if you want to use 'std::isnan'.
#include <cmath_ext.h> // FIXME: use functions in maths/Real.h

#include <QtOpenGL/qgl.h>
#include <QColor>
#include <QDir>
#include <QRgb>
#include <QDebug>

#include "ErrorOpeningFileForReadingException.h"
#include "Gdal.h"
#include "GdalReader.h"
#include "GdalReaderUtils.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"

#include "utils/TypeTraits.h"

namespace
{
#if 0
	void
	display_gdal_projection_info(
			GDALDataset *dataset)
	{
		if (dataset == NULL)
		{
			return;
		}

		std::cerr << "Driver: " <<
			dataset->GetDriver()->GetDescription() << " " << 
			dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME) 
			<< std::endl;

		if ( dataset->GetProjectionRef() != NULL)
		{
			std::cerr << dataset->GetProjectionRef() ;
		}

		if ( dataset->GetGeoTransform(adfGeoTransform) == CE_None)
		{
			double adfGeoTransform[6]; 
			std::cerr << "Origin: " <<
				adfGeoTransform[0] << " " << adfGeoTransform[3] << std::endl;
			std::cerr << "Pixel size: " <<
				adfGeoTransform[1] << " " << adfGeoTransform[5] << std::endl;
		}
	}

	void
	display_gdal_band_info(
			GDALRasterBand *band)
	{

		int xsize = band->GetXSize();
		int ysize = band->GetYSize();

		std::cerr << "X size: " << xsize << ", Y size: " << ysize << std::endl;

		int block_x, block_y;
		band->GetBlockSize(&block_x,&block_y);
		
		std::cerr << "Block size- x: " << block_x << " y: " << block_y << std::endl;
		std::cerr << "Type: " << 
			GDALGetDataTypeName(band->GetRasterDataType()) << std::endl;
		std::cerr << "Color Interpretation: " << 
			GDALGetColorInterpretationName(band->GetColorInterpretation()) << std::endl;

		std::cerr << "Has arbitrary overviews: " << band->HasArbitraryOverviews() << std::endl;

		int num_overviews = band->GetOverviewCount();
		std::cerr << "Overview count: " << num_overviews << std::endl;

		for (int count = 0; count < num_overviews ; count ++)
		{
			GDALRasterBand *temp_band = band->GetOverview(count);
			std::cerr << std::endl;
			std::cerr << "Band information about overview " << count << std::endl;
			display_gdal_band_info(temp_band);
		}

		int got_min, got_max; 
		double adfMinMax[2];

		adfMinMax[0] = band->GetMinimum(&got_min);
		adfMinMax[1] = band->GetMaximum(&got_max);

		if (! (got_min && got_max)){
			GDALComputeRasterMinMax(static_cast<GDALRasterBandH>(band), TRUE, adfMinMax);
		}

		std::cerr << "Min: " << adfMinMax[0] << ", Max: " << adfMinMax[1] << std::endl;

		if (band->GetColorTable() != NULL)
		{
			std::cerr << "Band has color table with " << 
				band->GetColorTable()->GetColorEntryCount() << " entries." << std::endl;
		}
	}
#endif

	using GPlatesFileIO::ErrorReadingGDALBand;


	template<class RawRasterType, bool IsFloatingPoint>
	struct ProcessNoDataValue
	{
		// IsFloatingPoint = false case here.
		static
		void
		process_no_data_value(
				typename RawRasterType::non_null_ptr_type result,
				typename RawRasterType::element_type *raster_buf,
				double no_data_value)
		{
			// For integer-valued rasters, we just cast the no_data_value to the
			// correct type and store the value.
			typedef typename RawRasterType::element_type raster_element_type;
			result->set_no_data_value(static_cast<raster_element_type>(no_data_value));
		}
	};


	template<class RawRasterType>
	struct ProcessNoDataValue<RawRasterType, /* bool IsFloatingPoint = */ true>
	{
		static
		void
		process_no_data_value(
				typename RawRasterType::non_null_ptr_type result,
				typename RawRasterType::element_type *raster_buf,
				double no_data_value)
		{
			// For floating-point rasters, we see if the no_data_value is NaN.
			// If it is NaN, we don't need to do anything - internally, we
			// expect NaN to be the no-data value anyway.
			// However, if it's not NaN, we'll have to convert all values that
			// match the no_data_value to NaN.
			if (GPlatesMaths::is_nan(no_data_value))
			{
				return;
			}

			typedef typename RawRasterType::element_type raster_element_type;
			raster_element_type casted_no_data_value = static_cast<raster_element_type>(no_data_value);
			raster_element_type casted_nan_value = static_cast<raster_element_type>(GPlatesMaths::nan());

			// Note: Boost lambda bind gave an error on MSVC2005 so changed to regular bind.
			std::replace_if(
					raster_buf,
					raster_buf + result->width() * result->height(),
					boost::bind(
						&GPlatesMaths::are_almost_exactly_equal<raster_element_type>,
						_1,
						casted_no_data_value),
					casted_nan_value);
		}
	};


	template<class RawRasterType>
	GPlatesPropertyValues::RawRaster::non_null_ptr_type
	read_raster_band(
			GDALRasterBand *band,
			bool flip,
			GDALDataType data_type)
	{
		int raster_width = band->GetXSize();
		int raster_height = band->GetYSize();

		// Create a new RawRaster and get a pointer to the buffer.
		typename RawRasterType::non_null_ptr_type result =
			RawRasterType::create(raster_width, raster_height);
		typedef typename RawRasterType::element_type raster_element_type;
		boost::shared_array<raster_element_type> raster_array = result->data();
		raster_element_type *raster_buf = raster_array.get();

		// Read it in line by line.
		for (int i = 0; i != raster_height; ++i)
		{
			// Work out which line we want to read in, depending on whether it's flipped.
			int line_index = flip ? raster_height - 1 - i : i;

			// Read the line into the buffer.
			CPLErr error = band->RasterIO(
					GF_Read,
					0 /* zero x-offset, read from left hand side */,
					line_index,
					raster_width,
					1 /* read one row */,
					raster_buf + i * raster_width,
					raster_width,
					1 /* one row of buffer */,
					data_type,
					0 /* no offsets in buffer */,
					0 /* no offsets in buffer */);

			if (error != CE_None)
			{
				throw ErrorReadingGDALBand();
			}
		}

		// Get and process the no-data value.
		int no_data_success = 0;
		double no_data_value = band->GetNoDataValue(&no_data_success);
		if (no_data_success)
		{
			ProcessNoDataValue
			<
				RawRasterType,
				GPlatesUtils::TypeTraits<raster_element_type>::is_floating_point
			>::process_no_data_value(result, raster_buf, no_data_value);
		}

		// Get and process statistics.
		GPlatesPropertyValues::RasterStatistics &statistics = result->statistics();
		double min, max, mean, std_dev;
		if (band->GetStatistics(
				false /* approx ok */,
				true /* force */,
				&min, &max, &mean, &std_dev) != CE_None)
		{
			throw ErrorReadingGDALBand();
		}
		statistics.minimum = min;
		statistics.maximum = max;
		statistics.mean = mean;
		statistics.standard_deviation = std_dev;

		return result;
	}


	GPlatesPropertyValues::RawRaster::non_null_ptr_type
	read_raster_band(
			GDALRasterBand *band,
			bool flip)
	{
		// Delegate to different template instantiation based on data type.
		GDALDataType data_type = band->GetRasterDataType();
		switch (data_type)
		{
			case GDT_Byte:
				return read_raster_band<GPlatesPropertyValues::UInt8RawRaster>(band, flip, data_type);

			case GDT_UInt16:
				return read_raster_band<GPlatesPropertyValues::UInt16RawRaster>(band, flip, data_type);

			case GDT_Int16:
				return read_raster_band<GPlatesPropertyValues::Int16RawRaster>(band, flip, data_type);

			case GDT_UInt32:
				return read_raster_band<GPlatesPropertyValues::UInt32RawRaster>(band, flip, data_type);

			case GDT_Int32:
				return read_raster_band<GPlatesPropertyValues::Int32RawRaster>(band, flip, data_type);

			case GDT_Float32:
				return read_raster_band<GPlatesPropertyValues::FloatRawRaster>(band, flip, data_type);

			case GDT_Float64:
				return read_raster_band<GPlatesPropertyValues::DoubleRawRaster>(band, flip, data_type);

			default:
				throw ErrorReadingGDALBand();
		}
	}

} // anonymous namespace


GPlatesFileIO::GdalReader::GdalReader()
{
	GDALAllRegister();

	d_dataset_ptr = NULL;
}


GPlatesFileIO::GdalReader::~GdalReader()
{
	try
	{
		if (d_dataset_ptr)
		{
			GDALClose(d_dataset_ptr);
		}
	}
	catch (...)
	{
	}
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::GdalReader::read_file(
		 const QString &filename,
		 GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	try
	{
		d_dataset_ptr = GdalReaderUtils::gdal_open(filename, read_errors);
		
		if (!d_dataset_ptr)
		{
			// Note: GdalReaderUtils::gdal_open() appends to the read_errors if it
			// returns NULL, so no need to do it ourselves here.
			return boost::none;
		}

		int n = d_dataset_ptr->GetRasterCount();
		if (n == 0)
		{
			throw ErrorReadingGDALBand();
		}

		// GDAL raster bands are numbered from 1. 
		GDALRasterBand *gdal_band_ptr = d_dataset_ptr->GetRasterBand(1);
		if (!gdal_band_ptr)
		{
			throw ErrorReadingGDALBand();
		}

		// GMT style GRDs are stored, and imported, upside-down.
		// See for example http://trac.osgeo.org/gdal/ticket/1926
		QString driver_type = d_dataset_ptr->GetDriver()->GetDescription();
		bool flip = (driver_type.compare("GMT") == 0);

		// Now read the band into a RawRaster.
		GPlatesPropertyValues::RawRaster::non_null_ptr_type result = read_raster_band(gdal_band_ptr, flip);
		return result;
	}
	catch (const ErrorReadingGDALBand &)
	{
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
				new GPlatesFileIO::LocalFileDataSource(filename, GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumber(0));

		read_errors.d_failures_to_begin.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
					e_source,
					e_location,
					GPlatesFileIO::ReadErrors::ErrorReadingGDALBand,
					GPlatesFileIO::ReadErrors::FileNotLoaded));

		return boost::none;
	}
}

