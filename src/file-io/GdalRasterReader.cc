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

#include <cstring> // for strcmp
#include <boost/bind.hpp>

#include "GdalRasterReader.h"

#include "GdalUtils.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"

#include "property-values/RasterStatistics.h"


namespace
{
	template<class RawRasterType>
	struct AddNoDataValue
	{
		static
		void
		add_no_data_value(
				RawRasterType &raster,
				GDALRasterBand *band)
		{
			// Default case: do nothing.
		}
	};

	// Specialisation for rasters that have a no-data value that can be set.
	template<typename T, template <class> class DataPolicy, class StatisticsPolicy>
	struct AddNoDataValue<
		GPlatesPropertyValues::RawRasterImpl<T, DataPolicy, StatisticsPolicy,
			GPlatesPropertyValues::RawRasterNoDataValuePolicies::WithNoDataValue> >
	{
		typedef GPlatesPropertyValues::RawRasterImpl<T, DataPolicy, StatisticsPolicy,
			GPlatesPropertyValues::RawRasterNoDataValuePolicies::WithNoDataValue> RawRasterType;
		typedef typename RawRasterType::element_type raster_element_type;

		static
		void
		add_no_data_value(
				RawRasterType &raster,
				GDALRasterBand *band)
		{
			int no_data_success = 0;
			double no_data_value = band->GetNoDataValue(&no_data_success);

			if (no_data_success)
			{
				raster.set_no_data_value(static_cast<raster_element_type>(no_data_value));
			}
		}
	};

	// Specialisation for rasters that have data and always use NaN as no-data value.
	template<typename T, class StatisticsPolicy>
	struct AddNoDataValue<
		GPlatesPropertyValues::RawRasterImpl<T, GPlatesPropertyValues::RawRasterDataPolicies::WithData,
			StatisticsPolicy, GPlatesPropertyValues::RawRasterNoDataValuePolicies::NanNoDataValue> >
	{
		typedef GPlatesPropertyValues::RawRasterImpl<T, GPlatesPropertyValues::RawRasterDataPolicies::WithData,
			StatisticsPolicy, GPlatesPropertyValues::RawRasterNoDataValuePolicies::NanNoDataValue> RawRasterType;
		typedef typename RawRasterType::element_type raster_element_type;

		static
		void
		add_no_data_value(
				RawRasterType &raster,
				GDALRasterBand *band)
		{
			int no_data_success = 0;
			double no_data_value = band->GetNoDataValue(&no_data_success);

			if (no_data_success)
			{
				// If the no-data value of the raster on disk is NaN, then there is nothing
				// to do, because this RawRasterType expects NaN as the no-data value.
				if (GPlatesMaths::is_nan(no_data_value))
				{
					return;
				}

				// If it is not NaN, however, we will have to convert all values that match
				// no_data_value to NaN.
				raster_element_type casted_no_data_value = static_cast<raster_element_type>(no_data_value);
				raster_element_type casted_nan_value = static_cast<raster_element_type>(GPlatesMaths::nan());

				std::replace_if(
						raster.data(),
						raster.data() + raster.width() * raster.height(),
						boost::bind(
							&GPlatesMaths::are_almost_exactly_equal<raster_element_type>,
							_1,
							casted_no_data_value),
						casted_nan_value);
			}
		}
	};


	template<class RawRasterType>
	bool
	add_statistics(
			RawRasterType &raster,
			GDALRasterBand *band)
	{
		GPlatesPropertyValues::RasterStatistics &statistics = raster.statistics();

		double min, max, mean, std_dev;
		if (band->GetStatistics(
				false /* approx ok */,
				true /* force */,
				&min, &max, &mean, &std_dev) != CE_None)
		{
			// Failed to read statistics.
			return false;
		}

		statistics.minimum = min;
		statistics.maximum = max;
		statistics.mean = mean;
		statistics.standard_deviation = std_dev;

		return true;
	}

	template<class RawRasterType>
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
	create_proxied_raw_raster(
			GDALRasterBand *band,
			const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle)
	{
		// Create a proxied raster.
		int source_width = band->GetXSize();
		int source_height = band->GetYSize();
		typename RawRasterType::non_null_ptr_type result =
			RawRasterType::create(source_width, source_height, raster_band_reader_handle);

		// Attempt to add a no-data value.
		// OK if no-data value not added.
		AddNoDataValue<RawRasterType>::add_no_data_value(*result, band);

		// Attempt to add statistics.
		// Not OK if statistics not added, as all rasters read through GDAL
		// should be able to report back statistics.
		if (!add_statistics(*result, band))
		{
			return boost::none;
		}

		return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
	}

	template<typename RasterElementType>
	bool
	add_data(
			RasterElementType *result_buf,
			GDALRasterBand *band,
			bool flip,
			GDALDataType data_type,
			unsigned int region_x_offset,
			unsigned int region_y_offset,
			unsigned int region_width,
			unsigned int region_height)
	{
		int source_height = band->GetYSize();

		// Read it in line by line.
		for (unsigned int i = 0; i != region_height; ++i)
		{
			// Work out which line we want to read in, depending on whether it's flipped.
			int line_index = region_y_offset + i;
			if (flip)
			{
				line_index = source_height - 1 - line_index;
			}

			// Read the line into the buffer.
			CPLErr error = band->RasterIO(
					GF_Read,
					region_x_offset,
					line_index,
					region_width,
					1 /* read one row */,
					result_buf + i * region_width,
					region_width,
					1 /* one row of buffer */,
					data_type,
					0 /* no offsets in buffer */,
					0 /* no offsets in buffer */);

			if (error != CE_None)
			{
				return false;
			}
		}

		return true;
	}

	bool
	unpack_region(
			const QRect &region,
			int full_width,
			int full_height,
			unsigned int &region_x_offset,
			unsigned int &region_y_offset,
			unsigned int &region_width,
			unsigned int &region_height)
	{
		if (region.isValid())
		{
			// Check that region lies within the source raster.
			if (region.x() < 0 || region.y() < 0 ||
				region.width() == 0 || region.height() == 0 ||
					(region.x() + region.width()) > full_width ||
					(region.y() + region.height()) > full_height)
			{
				return false;
			}

			region_x_offset = region.x();
			region_y_offset = region.y();
			region_width = region.width();
			region_height = region.height();
		}
		else
		{
			// Invalid region means read in the whole source raster.
			region_x_offset = 0;
			region_y_offset = 0;
			region_width = full_width;
			region_height = full_height;
		}

		return true;
	}

	template<class RawRasterType>
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
	create_raw_raster(
			GDALRasterBand *band,
			bool flip,
			GDALDataType data_type,
			const QRect &region)
	{
		// Create a new RawRaster.
		int source_width = band->GetXSize();
		int source_height = band->GetYSize();
		unsigned int region_x_offset, region_y_offset, region_width, region_height;
		if (!unpack_region(region, source_width, source_height,
					region_x_offset, region_y_offset, region_width, region_height))
		{
			return boost::none;
		}

		typename RawRasterType::non_null_ptr_type result =
			RawRasterType::create(region_width, region_height);

		// Attempt to read in data from the specified region.
		// Not OK if adding data failed.
		if (!add_data(result->data(), band, flip, data_type,
					region_x_offset, region_y_offset, region_width, region_height))
		{
			return boost::none;
		}
		
		// Attempt to add a no-data value.
		// OK if no-data value not added.
		AddNoDataValue<RawRasterType>::add_no_data_value(*result, band);

		// Attempt to add statistics.
		// Not OK if statistics not added, as all rasters read through GDAL
		// should be able to report back statistics.
		if (!add_statistics(*result, band))
		{
			return boost::none;
		}

		return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
	}

	template<typename RasterElementType>
	void *
	read_data(
			GDALRasterBand *band,
			bool flip,
			GDALDataType data_type,
			const QRect &region)
	{
		typedef RasterElementType raster_element_type;

		// Allocate the buffer to read into.
		int source_width = band->GetXSize();
		int source_height = band->GetYSize();
		unsigned int region_x_offset, region_y_offset, region_width, region_height;
		if (!unpack_region(region, source_width, source_height,
					region_x_offset, region_y_offset, region_width, region_height))
		{
			return NULL;
		}

		raster_element_type *result_buf = new raster_element_type[region_width * region_height];

		if (!add_data(result_buf, band, flip, data_type,
					region_x_offset, region_y_offset, region_width, region_height))
		{
			delete[] result_buf;
			return NULL;
		}

		return result_buf;
	}
}


GPlatesFileIO::GDALRasterReader::GDALRasterReader(
		const QString &filename,
		const boost::function<RasterBandReaderHandle (unsigned int)> &proxy_handle_function,
		ReadErrorAccumulation *read_errors) :
	d_filename(filename),
	d_dataset(GdalUtils::gdal_open(filename, read_errors)),
	d_proxy_handle_function(proxy_handle_function),
	d_flip(d_dataset
			&& std::strcmp(d_dataset->GetDriver()->GetDescription(), "GMT") == 0)
{  }


GPlatesFileIO::GDALRasterReader::~GDALRasterReader()
{
	try
	{
		if (d_dataset)
		{
			// Closes the dataset as well as all bands that were opened.
			GDALClose(d_dataset);
		}
	}
	catch (...)
	{
	}
}


bool
GPlatesFileIO::GDALRasterReader::can_read()
{
	return d_dataset != NULL;
}


unsigned int
GPlatesFileIO::GDALRasterReader::get_number_of_bands(
		ReadErrorAccumulation *read_errors)
{
	if (d_dataset)
	{
		return d_dataset->GetRasterCount();
	}
	else
	{
		return 0;
	}
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::GDALRasterReader::get_proxied_raw_raster(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	// Memory managed by GDAL.
	GDALRasterBand *band = get_raster_band(band_number, read_errors);

	if (!band)
	{
		return boost::none;
	}

	GDALDataType data_type = band->GetRasterDataType();
	RasterBandReaderHandle raster_band_reader_handle = d_proxy_handle_function(band_number);

	switch (data_type)
	{
		case GDT_Byte:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedUInt8RawRaster>(
				band, raster_band_reader_handle);

		case GDT_UInt16:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedUInt16RawRaster>(
				band, raster_band_reader_handle);

		case GDT_Int16:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedInt16RawRaster>(
				band, raster_band_reader_handle);

		case GDT_UInt32:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedUInt32RawRaster>(
				band, raster_band_reader_handle);

		case GDT_Int32:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedInt32RawRaster>(
				band, raster_band_reader_handle);

		case GDT_Float32:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedFloatRawRaster>(
				band, raster_band_reader_handle);

		case GDT_Float64:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedDoubleRawRaster>(
				band, raster_band_reader_handle);

		default:
			return boost::none;
	}
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::GDALRasterReader::get_raw_raster(
		unsigned int band_number,
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	// Memory managed by GDAL.
	GDALRasterBand *band = get_raster_band(band_number, read_errors);

	if (!band)
	{
		return boost::none;
	}

	GDALDataType data_type = band->GetRasterDataType();

	switch (data_type)
	{
		case GDT_Byte:
			return create_raw_raster<GPlatesPropertyValues::UInt8RawRaster>(
				band, d_flip, data_type, region);

		case GDT_UInt16:
			return create_raw_raster<GPlatesPropertyValues::UInt16RawRaster>(
				band, d_flip, data_type, region);

		case GDT_Int16:
			return create_raw_raster<GPlatesPropertyValues::Int16RawRaster>(
				band, d_flip, data_type, region);

		case GDT_UInt32:
			return create_raw_raster<GPlatesPropertyValues::UInt32RawRaster>(
				band, d_flip, data_type, region);

		case GDT_Int32:
			return create_raw_raster<GPlatesPropertyValues::Int32RawRaster>(
				band, d_flip, data_type, region);

		case GDT_Float32:
			return create_raw_raster<GPlatesPropertyValues::FloatRawRaster>(
				band, d_flip, data_type, region);

		case GDT_Float64:
			return create_raw_raster<GPlatesPropertyValues::DoubleRawRaster>(
				band, d_flip, data_type, region);

		default:
			return boost::none;
	}
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::GDALRasterReader::get_type(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	// Memory managed by GDAL.
	GDALRasterBand *band = get_raster_band(band_number, read_errors);

	if (!band)
	{
		return GPlatesPropertyValues::RasterType::UNKNOWN;
	}

	GDALDataType data_type = band->GetRasterDataType();

	switch (data_type)
	{
		case GDT_Byte:
			return GPlatesPropertyValues::RasterType::UINT8;

		case GDT_UInt16:
			return GPlatesPropertyValues::RasterType::UINT16;

		case GDT_Int16:
			return GPlatesPropertyValues::RasterType::INT16;

		case GDT_UInt32:
			return GPlatesPropertyValues::RasterType::UINT32;

		case GDT_Int32:
			return GPlatesPropertyValues::RasterType::INT32;

		case GDT_Float32:
			return GPlatesPropertyValues::RasterType::FLOAT;

		case GDT_Float64:
			return GPlatesPropertyValues::RasterType::DOUBLE;

		default:
			return GPlatesPropertyValues::RasterType::UNKNOWN;
	}
}


void *
GPlatesFileIO::GDALRasterReader::get_data(
		unsigned int band_number,
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	// Memory managed by GDAL.
	GDALRasterBand *band = get_raster_band(band_number, read_errors);

	if (!band)
	{
		return NULL;
	}

	GDALDataType data_type = band->GetRasterDataType();

	switch (data_type)
	{
		case GDT_Byte:
			return read_data<boost::uint8_t>(band, d_flip, data_type, region);

		case GDT_UInt16:
			return read_data<boost::uint16_t>(band, d_flip, data_type, region);

		case GDT_Int16:
			return read_data<boost::int16_t>(band, d_flip, data_type, region);

		case GDT_UInt32:
			return read_data<boost::uint32_t>(band, d_flip, data_type, region);

		case GDT_Int32:
			return read_data<boost::int32_t>(band, d_flip, data_type, region);

		case GDT_Float32:
			return read_data<float>(band, d_flip, data_type, region);

		case GDT_Float64:
			return read_data<double>(band, d_flip, data_type, region);

		default:
			return NULL;
	}
}


GDALRasterBand *
GPlatesFileIO::GDALRasterReader::get_raster_band(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	if (!d_dataset)
	{
		return NULL;
	}

	// Returns NULL if getting raster band failed.
	GDALRasterBand *result = d_dataset->GetRasterBand(band_number);
	if (!result)
	{
		report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterBand);
	}

	return result;
}


void
GPlatesFileIO::GDALRasterReader::report_recoverable_error(
		ReadErrorAccumulation *read_errors,
		ReadErrors::Description description)
{
	if (read_errors)
	{
		read_errors->d_recoverable_errors.push_back(
				make_read_error_occurrence(
					d_filename,
					DataFormats::RasterImage,
					0,
					description,
					ReadErrors::FileNotLoaded));
	}
}


void
GPlatesFileIO::GDALRasterReader::report_failure_to_begin(
		ReadErrorAccumulation *read_errors,
		ReadErrors::Description description)
{
	if (read_errors)
	{
		read_errors->d_failures_to_begin.push_back(
				make_read_error_occurrence(
					d_filename,
					DataFormats::RasterImage,
					0,
					description,
					ReadErrors::FileNotLoaded));
	}
}

