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

#include <cmath>
#include <cstring> // for strcmp
#include <exception>
#include <limits>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <QDateTime>

#ifdef HAVE_CONFIG_H
// Include config header so we know whether (and how) to include "gdal_version.h"
// which contains the version of GDAL we're compiling against.
#include "global/config.h"
#ifdef HAVE_GDAL_VERSION_H
#if defined(HAVE_GDAL_VERSION_H_UPPERCASE_GDAL_PREFIX)
#include <GDAL/gdal_version.h>
#elif defined(HAVE_GDAL_VERSION_H_LOWERCASE_GDAL_PREFIX)
#include <gdal/gdal_version.h>
#else
#include <gdal_version.h>
#endif
#endif
#endif

#include "GdalRasterReader.h"

#include "ErrorOpeningFileForReadingException.h"
#include "ErrorOpeningFileForWritingException.h"
#include "GdalUtils.h"
#include "RasterFileCacheFormat.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"

#include "property-values/RasterStatistics.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


namespace
{
	template<class RawRasterType>
	void
	add_no_data_value(
			RawRasterType &raster,
			GDALRasterBand *band)
	{
		int no_data_success = 0;
		double no_data_value = band->GetNoDataValue(&no_data_success);

		if (no_data_success)
		{
			GPlatesPropertyValues::RawRasterUtils::add_no_data_value(
					raster,
					static_cast<typename RawRasterType::element_type>(no_data_value));
		}
	}

	template<class RawRasterType>
	GPlatesPropertyValues::RawRaster::non_null_ptr_type
	create_proxied_raw_raster(
			GDALRasterBand *band,
			const GPlatesPropertyValues::RasterStatistics &raster_statistics,
			const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle)
	{
		// Create a proxied raster.
		int source_width = band->GetXSize();
		int source_height = band->GetYSize();
		typename RawRasterType::non_null_ptr_type result =
			RawRasterType::create(source_width, source_height, raster_band_reader_handle);

		// Attempt to add a no-data value.
		// OK if no-data value not added.
		add_no_data_value(*result, band);

		// Add the statistics.
		result->statistics() = raster_statistics;

		return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
	}

	template<typename RasterElementType>
	void
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
		PROFILE_BLOCK("Read GDAL raster data");

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
					result_buf + qint64(i) * region_width,
					region_width,
					1 /* one row of buffer */,
					data_type,
					0 /* no offsets in buffer */,
					0 /* no offsets in buffer */);

			if (error != CE_None)
			{
				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE, "Unable to read GDAL raster data.");
			}
		}
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
				region.width() <= 0 || region.height() <= 0 ||
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
	boost::optional<typename RawRasterType::non_null_ptr_type>
	read_data(
			GDALRasterBand *band,
			bool flip,
			const QRect &region)
	{
		typedef typename RawRasterType::element_type raster_element_type;

		// Allocate the buffer to read into.
		int source_width = band->GetXSize();
		int source_height = band->GetYSize();
		unsigned int region_x_offset, region_y_offset, region_width, region_height;
		if (!unpack_region(region, source_width, source_height,
					region_x_offset, region_y_offset, region_width, region_height))
		{
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE, "Invalid region specified for GDAL raster.");
		}

		boost::optional<typename RawRasterType::non_null_ptr_type> result;

		try
		{
			result = RawRasterType::create(region_width, region_height);
		}
		catch (std::bad_alloc &)
		{
			// Memory allocation failure.
			return boost::none;
		}

		GDALDataType data_type = band->GetRasterDataType();

		add_data(result.get()->data(), band, flip, data_type,
					region_x_offset, region_y_offset, region_width, region_height);

		// Add the no-data value after adding the data - it's needed to determine coverage.
		add_no_data_value(*result.get(), band);

		return result;
	}
}


GPlatesFileIO::GDALRasterReader::GDALRasterReader(
		const QString &filename,
		RasterReader *raster_reader,
		ReadErrorAccumulation *read_errors) :
	RasterReaderImpl(raster_reader),
	d_source_raster_filename(filename),
	d_dataset(GdalUtils::gdal_open(filename, read_errors)),
	d_flip(false),
	d_source_width(0),
	d_source_height(0)
{
	// Prior to 1st Dec 2009 there was a bug in GDAL that incorrectly flipped (in y-direction)
	// non-GMT-style GRDs. So GDAL releases after this date do not need any flipping
	// (GMT-style or non-GMT-style).
	// The ticket http://trac.osgeo.org/gdal/ticket/2654 describes the bug and refers
	// to the changeset http://trac.osgeo.org/gdal/changeset/18151 that fixes it.
	//
	// We noticed that some Windows FWTools releases (that include GDAL) define
	//   GDAL_VERSION_MAJOR 1
	//   GDAL_VERSION_MINOR 7
	//   GDAL_VERSION_REV   0
	//   GDAL_VERSION_BUILD 0
	// for FWTools versions 2.4.5, 2.4.6 and 2.4.7 but only 2.4.7 has the bug fix included so
	// we can't use those defines.
	// Instead we use the 'GDAL_RELEASE_DATE' define and compare against the date when
	// the bug was fixed in GDAL (1st Dec 2009 or 20091201).

	// 'GDAL_RELEASE_DATE' is defined in the GDAL 'gdal_version.h' if it exists.
	// If the 'gdal_version.h' file does not exist then we are looking at an old version
	// of GDAL which does not have the bug fix (and hence we need to flip non-GMT-style GRDs).
#if !defined(GDAL_RELEASE_DATE) || (GDAL_RELEASE_DATE < 20091201)
	if (d_dataset &&
		std::strcmp(d_dataset->GetDriver()->GetDescription(), "GMT") != 0)
	{
		d_flip = true;
	}
#endif

	//
	// UPDATE:
	//
	// It looks like there's a few bugs in GDAL related to flipping.
	// The changesets related to image flipping in the netCDF driver...
	//
	//  http://trac.osgeo.org/gdal/log/trunk/gdal/frmts/netcdf/netcdfdataset.cpp
	//
	// ...are...
	//
	//  http://trac.osgeo.org/gdal/changeset/18151/trunk/gdal/frmts/netcdf/netcdfdataset.cpp
	//     (the fix we currently work around in GPlates)
	//  http://trac.osgeo.org/gdal/changeset/20006/trunk/gdal/frmts/netcdf/netcdfdataset.cpp
	//  http://trac.osgeo.org/gdal/changeset/23615/trunk/gdal/frmts/netcdf/netcdfdataset.cpp
	//  http://trac.osgeo.org/gdal/changeset/23617/trunk/gdal/frmts/netcdf/netcdfdataset.cpp
	//
	// So it looks like any workarounds we come up with might depend on the content of each netCDF
	// raster file and we don't want to analyse that in GPlates.
	// Probably the best bet is to increase the minimum GDAL requirement
	// (although that may be difficult with the Ubuntu systems).
	// Which means avoiding certain GDAL versions between the first bug-fix changeset listed above
	// and the last (and write that off as unknown territory).
	//
	// According to the history of GDAL releases...
	//
	//   http://trac.osgeo.org/gdal/browser/tags
	//
	// ...it looks like the above changesets probably went into the following releases...
	//
	// 18151 -> 1.7.0
	// 20006 -> 1.7.3
	// 23615 -> 1.9.0
	// 23617 -> 1.9.0
	//
	// Testing with GDAL 1.9.0 worked on two rasters where one of those rasters was incorrectly
	// flipped on GDAL 1.7.

	if (!can_read())
	{
		report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
		return;
	}

	// Get the source raster dimensions.
	const std::pair<unsigned int, unsigned int> source_raster_dimensions = get_size(read_errors);
	if (source_raster_dimensions.first == 0 ||
		source_raster_dimensions.second == 0)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE, "Raster has zero dimensions.");
	}
	d_source_width = source_raster_dimensions.first;
	d_source_height = source_raster_dimensions.second;

	// Create the source raster file cache for each band if there isn't one or it's out of date.
	for (unsigned int band_number = 1; band_number <= get_number_of_bands(read_errors); ++band_number)
	{
		boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader> raster_band_file_cache_reader =
				create_source_raster_file_cache_format_reader(band_number, read_errors);

		if (!raster_band_file_cache_reader)
		{
			// We were unable to create a raster band file cache or unable to read it.
			report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
			continue;
		}

		d_raster_band_file_cache_format_readers.push_back(raster_band_file_cache_reader);
	}
}


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


std::pair<unsigned int, unsigned int>
GPlatesFileIO::GDALRasterReader::get_size(
		ReadErrorAccumulation *read_errors)
{
	unsigned int number_of_bands = get_number_of_bands(read_errors);
	if (number_of_bands == 0)
	{
		return std::make_pair(0, 0);
	}


	// Note: GDAL bands are 1-based.
	GDALRasterBand *band = get_raster_band(1, read_errors);
	if (!band)
	{
		return std::make_pair(0, 0);
	}

	int width = band->GetXSize();
	int height = band->GetYSize();

	// Make sure all bands are the same size.
	for (unsigned int i = 2; i != number_of_bands + 1; ++i)
	{
		band = get_raster_band(i, read_errors);
		if (band &&
				(band->GetXSize() != width ||
				 band->GetYSize() != height))
		{
			return std::make_pair(0, 0);
		}
	}

	return std::make_pair(width, height);
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::GDALRasterReader::get_proxied_raw_raster(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	if (!can_read())
	{
		return boost::none;
	}

	// Memory managed by GDAL.
	GDALRasterBand *band = get_raster_band(band_number, read_errors);

	if (!band)
	{
		return boost::none;
	}

	if (band_number == 0 ||
		band_number > d_raster_band_file_cache_format_readers.size())
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return boost::none;
	}

	if (!d_raster_band_file_cache_format_readers[band_number - 1])
	{
		return boost::none;
	}

	// Read the raster statistics from the raster file cache.
	//
	// NOTE: We avoid reading them directly using GDAL since that can require rescanning the source
	// data which is not necessary since we've cached the statistics in the cache format reader.
	// This saves a few seconds when the raster is first loaded into GPlates.
	boost::optional<GPlatesPropertyValues::RasterStatistics> raster_statistics =
			d_raster_band_file_cache_format_readers[band_number - 1]->get_raster_statistics();
	if (!raster_statistics)
	{
		// We normally wouldn't get here since GDAL should always be able to provide statistics
		// which should have been stored in the raster cache file.
		// However there was a bug in GPlates 1.2 that failed to store the raster statistics in the
		// cache file, so we need to get the statistics here.
		double min, max, mean, std_dev;
		if (band->GetStatistics(
				false /* approx ok */,
				true /* force */,
				&min, &max, &mean, &std_dev) != CE_None)
		{
			// Not OK if statistics not added, as all rasters read through GDAL should be able to
			// report back statistics even if it involves GDAL scanning the image data.

			// Log an error message so we know why a raster is not being displayed.
			// NOTE: This failure actually didn't happen now - it happened when GPlates created the
			// raster cache file (which could've been a different instance of GPlates).
			qWarning() << "Failed to read GDAL statistics from '"
					<< d_raster_band_file_cache_format_readers[band_number - 1]->get_filename() << "'.";


			report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
			return boost::none;
		}

		raster_statistics = GPlatesPropertyValues::RasterStatistics();
		raster_statistics->minimum = min;
		raster_statistics->maximum = max;
		raster_statistics->mean = mean;
		raster_statistics->standard_deviation = std_dev;
	}

	GDALDataType data_type = band->GetRasterDataType();
	RasterBandReaderHandle raster_band_reader_handle =
			create_raster_band_reader_handle(band_number);

	switch (data_type)
	{
		case GDT_Byte:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedUInt8RawRaster>(
				band, raster_statistics.get(), raster_band_reader_handle);

		case GDT_UInt16:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedUInt16RawRaster>(
				band, raster_statistics.get(), raster_band_reader_handle);

		case GDT_Int16:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedInt16RawRaster>(
				band, raster_statistics.get(), raster_band_reader_handle);

		case GDT_UInt32:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedUInt32RawRaster>(
				band, raster_statistics.get(), raster_band_reader_handle);

		case GDT_Int32:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedInt32RawRaster>(
				band, raster_statistics.get(), raster_band_reader_handle);

		case GDT_Float32:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedFloatRawRaster>(
				band, raster_statistics.get(), raster_band_reader_handle);

		case GDT_Float64:
			return create_proxied_raw_raster<GPlatesPropertyValues::ProxiedDoubleRawRaster>(
				band, raster_statistics.get(), raster_band_reader_handle);

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
	if (!can_read())
	{
		return boost::none;
	}

	if (band_number == 0 ||
		band_number > d_raster_band_file_cache_format_readers.size())
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return boost::none;
	}

	unsigned int region_x_offset, region_y_offset, region_width, region_height;
	if (!unpack_region(region, d_source_width, d_source_height,
				region_x_offset, region_y_offset, region_width, region_height))
	{
		return boost::none;
	}

	if (!d_raster_band_file_cache_format_readers[band_number - 1])
	{
		return boost::none;
	}

	// Read the specified source region from the raster file cache.
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> data =
			d_raster_band_file_cache_format_readers[band_number - 1]->read_raster(
					region_x_offset, region_y_offset, region_width, region_height);

	if (!data)
	{
		report_recoverable_error(read_errors, ReadErrors::InvalidRegionInRaster);
		return boost::none;
	}

	return data.get();
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
					d_source_raster_filename,
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
					d_source_raster_filename,
					DataFormats::RasterImage,
					0,
					description,
					ReadErrors::FileNotLoaded));
	}
}


boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader>
GPlatesFileIO::GDALRasterReader::create_source_raster_file_cache_format_reader(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	// Find the existing source raster file cache (if exists).
	boost::optional<QString> cache_filename =
			RasterFileCacheFormat::get_existing_source_cache_filename(d_source_raster_filename, band_number);
	if (cache_filename)
	{
		// If the source raster was modified after the raster file cache then we need
		// to regenerate the raster file cache.
		QDateTime source_last_modified = QFileInfo(d_source_raster_filename).lastModified();
		QDateTime cache_last_modified = QFileInfo(cache_filename.get()).lastModified();
		if (source_last_modified > cache_last_modified)
		{
			// Remove the cache file.
			QFile(cache_filename.get()).remove();
			// Create a new cache file.
			if (!create_source_raster_file_cache(band_number, read_errors))
			{
				// Unable to create cache file.
				return boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader>();
			}
		}
	}
	// Generate the cache file if it doesn't exist...
	else
	{
		if (!create_source_raster_file_cache(band_number, read_errors))
		{
			// Unable to create cache file.
			return boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader>();
		}

		cache_filename =
				RasterFileCacheFormat::get_existing_source_cache_filename(d_source_raster_filename, band_number);
		if (!cache_filename)
		{
			// Cache file was created but unable to read it for some reason.
			return boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader>();
		}
	}

	boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader> raster_band_file_cache_format_reader;

	try
	{
		try
		{
			raster_band_file_cache_format_reader =
					create_source_raster_file_cache_format_reader(
							band_number, cache_filename.get(), read_errors);
		}
		catch (RasterFileCacheFormat::UnsupportedVersion &exc)
		{
			// Log the exception so we know what caused the failure.
			qWarning() << exc;

			qWarning() << "Attempting rebuild of source raster file cache '"
					<< cache_filename.get() << "' for current version of GPlates.";

			// We'll have to remove the file and build it for the current GPlates version.
			// This means if the future version of GPlates (the one that created the
			// unrecognised version file) runs again it will either know how to
			// load our version (or rebuild it for itself also if it determines its
			// new format is much better or much more efficient).
			QFile(cache_filename.get()).remove();

			// Build it with the current version format.
			if (create_source_raster_file_cache(band_number, read_errors))
			{
				// Try reading it again.
				raster_band_file_cache_format_reader =
						create_source_raster_file_cache_format_reader(
								band_number, cache_filename.get(), read_errors);
			}
		}
		catch (std::exception &exc)
		{
			// Log the exception so we know what caused the failure.
			qWarning() << "Error reading source raster file cache '" << cache_filename.get()
					<< "', attempting rebuild: " << exc.what();

			// Remove the cache file in case it is corrupted somehow.
			// Eg, it was partially written to by a previous instance of GPlates and
			// not immediately removed for some reason.
			QFile(cache_filename.get()).remove();

			// Try building it again.
			if (create_source_raster_file_cache(band_number, read_errors))
			{
				// Try reading it again.
				raster_band_file_cache_format_reader =
						create_source_raster_file_cache_format_reader(
								band_number, cache_filename.get(), read_errors);
			}
		}
	}
	catch (std::exception &exc)
	{
		// Log the exception so we know what caused the failure.
		qWarning() << exc.what();

		// Log a warning message.
		qWarning() << "Unable to read, or generate, source raster file cache for raster '"
				<< d_source_raster_filename << "', giving up on it.";
	}
	catch (...)
	{
		qWarning() << "Unknown exception";

		// Log a warning message.
		qWarning() << "Unable to read, or generate, source raster file cache for raster '"
				<< d_source_raster_filename << "', giving up on it.";
	}

	return raster_band_file_cache_format_reader;
}


boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader>
GPlatesFileIO::GDALRasterReader::create_source_raster_file_cache_format_reader(
		unsigned int band_number,
		const QString &cache_filename,
		ReadErrorAccumulation *read_errors)
{
	// Memory managed by GDAL.
	GDALRasterBand *band = get_raster_band(band_number, read_errors);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			band,
			GPLATES_ASSERTION_SOURCE);

	GDALDataType data_type = band->GetRasterDataType();

	// Attempt to create the source raster file cache format reader.
	switch (data_type)
	{
		case GDT_Byte:
			return boost::shared_ptr<SourceRasterFileCacheFormatReader>(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::UInt8RawRaster>(
							cache_filename));

		case GDT_UInt16:
			return boost::shared_ptr<SourceRasterFileCacheFormatReader>(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::UInt16RawRaster>(
							cache_filename));

		case GDT_Int16:
			return boost::shared_ptr<SourceRasterFileCacheFormatReader>(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::Int16RawRaster>(
							cache_filename));

		case GDT_UInt32:
			return boost::shared_ptr<SourceRasterFileCacheFormatReader>(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::UInt32RawRaster>(
					cache_filename));

		case GDT_Int32:
			return boost::shared_ptr<SourceRasterFileCacheFormatReader>(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::Int32RawRaster>(
							cache_filename));

		case GDT_Float32:
			return boost::shared_ptr<SourceRasterFileCacheFormatReader>(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::FloatRawRaster>(
							cache_filename));

		case GDT_Float64:
			return boost::shared_ptr<SourceRasterFileCacheFormatReader>(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::DoubleRawRaster>(
							cache_filename));

		default:
			break;
	}

	throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE, "Unexpected GDAL raster type.");
}


bool
GPlatesFileIO::GDALRasterReader::create_source_raster_file_cache(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	PROFILE_FUNC();

	boost::optional<QString> cache_filename =
			RasterFileCacheFormat::get_writable_source_cache_filename(d_source_raster_filename, band_number);

	if (!cache_filename)
	{
		// Can't write raster file cache anywhere.
		return false;
	}

	// Write the cache file.
	try
	{
		// Memory managed by GDAL.
		GDALRasterBand *band = get_raster_band(band_number, read_errors);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				band,
				GPLATES_ASSERTION_SOURCE);

		GDALDataType data_type = band->GetRasterDataType();

		switch (data_type)
		{
			case GDT_Byte:
				write_source_raster_file_cache<GPlatesPropertyValues::UInt8RawRaster>(
						band_number, cache_filename.get(), read_errors);
				break;

			case GDT_UInt16:
				write_source_raster_file_cache<GPlatesPropertyValues::UInt16RawRaster>(
						band_number, cache_filename.get(), read_errors);
				break;

			case GDT_Int16:
				write_source_raster_file_cache<GPlatesPropertyValues::Int16RawRaster>(
						band_number, cache_filename.get(), read_errors);
				break;

			case GDT_UInt32:
				write_source_raster_file_cache<GPlatesPropertyValues::UInt32RawRaster>(
						band_number, cache_filename.get(), read_errors);
				break;

			case GDT_Int32:
				write_source_raster_file_cache<GPlatesPropertyValues::Int32RawRaster>(
						band_number, cache_filename.get(), read_errors);
				break;

			case GDT_Float32:
				write_source_raster_file_cache<GPlatesPropertyValues::FloatRawRaster>(
						band_number, cache_filename.get(), read_errors);
				break;

			case GDT_Float64:
				write_source_raster_file_cache<GPlatesPropertyValues::DoubleRawRaster>(
						band_number, cache_filename.get(), read_errors);
				break;

			default:
				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE, "Unexpected GDAL raster type.");
		}

		// Copy the file permissions from the source raster file to the cache file.
		QFile::setPermissions(cache_filename.get(), QFile::permissions(d_source_raster_filename));
	}
	catch (std::exception &exc)
	{
		// Log the exception so we know what caused the failure.
		qWarning() << "Error writing source raster file cache '" << cache_filename.get()
				<< "', removing it: " << exc.what();

		// Remove the cache file in case it was partially written.
		QFile(cache_filename.get()).remove();

		return false;
	}
	catch (...)
	{
		// Log the exception so we know what caused the failure.
		qWarning() << "Unknown error writing source raster file cache '" << cache_filename.get()
				<< "', removing it";

		// Remove the cache file in case it was partially written.
		QFile(cache_filename.get()).remove();

		return false;
	}

	return true;
}


template <class RawRasterType>
void
GPlatesFileIO::GDALRasterReader::write_source_raster_file_cache(
		unsigned int band_number,
		const QString &cache_filename,
		ReadErrorAccumulation *read_errors)
{
	PROFILE_FUNC();

	// Open the cache file for writing.
	QFile cache_file(cache_filename);
	if (!cache_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		throw ErrorOpeningFileForWritingException(
				GPLATES_EXCEPTION_SOURCE, cache_filename);
	}
	QDataStream out(&cache_file);

	out.setVersion(RasterFileCacheFormat::Q_DATA_STREAM_VERSION);

	// Write magic number/string.
	for (unsigned int n = 0; n < sizeof(RasterFileCacheFormat::MAGIC_NUMBER); ++n)
	{
		out << static_cast<quint8>(RasterFileCacheFormat::MAGIC_NUMBER[n]);
	}

	// Write the file size - write zero for now and come back later to fill it in.
	const qint64 file_size_file_offset = cache_file.pos();
	qint64 total_cache_file_size = 0;
	out << static_cast<qint64>(total_cache_file_size);

	// Write version number.
	out << static_cast<quint32>(RasterFileCacheFormat::VERSION_NUMBER);

	// Write source raster type.
	out << static_cast<quint32>(RasterFileCacheFormat::get_type_as_enum<RawRasterType::element_type>());

	// TODO: Add coverage data.
	out << static_cast<quint32>(false/*has_coverage*/);

	// Write the source raster dimensions.
	out << static_cast<quint32>(d_source_width);
	out << static_cast<quint32>(d_source_height);

	// The source raster will get written to the cache file in blocks.
	RasterFileCacheFormat::BlockInfos block_infos(d_source_width, d_source_height);

	// Write the number of blocks in the source raster.
	out << static_cast<quint32>(block_infos.get_num_blocks());

	GDALRasterBand *band = get_raster_band(band_number, read_errors);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			band,
			GPLATES_ASSERTION_SOURCE);
	// Get the raster no-data value.
	int no_data_success = 0;
	double no_data_value = band->GetNoDataValue(&no_data_success);
	// Write the (optional) raster no-data value.
	if (no_data_success)
	{
		out << static_cast<quint32>(true);
		out << static_cast<RawRasterType::element_type>(no_data_value);
	}
	else
	{
		out << static_cast<quint32>(false);
		out << RawRasterType::element_type(); // Doesn't matter what gets stored.
	}

	// Write the (optional) raster statistics as zeros for now and come back later to fill it in.
	const qint64 statistics_file_offset = cache_file.pos();
	out << static_cast<quint32>(false); // has_raster_statistics
	out << static_cast<quint32>(false); // has_raster_minimum
	out << static_cast<quint32>(false); // has_raster_maximum
	out << static_cast<quint32>(false); // has_raster_mean
	out << static_cast<quint32>(false); // has_raster_standard_deviation
	out << static_cast<double>(0); // raster_minimum - doesn't matter what gets read.
	out << static_cast<double>(0); // raster_maximum - doesn't matter what gets read.
	out << static_cast<double>(0); // raster_mean - doesn't matter what gets read.
	out << static_cast<double>(0); // raster_standard_deviation - doesn't matter what gets read.

	// The block information will get written next.
	const qint64 block_info_file_offset = cache_file.pos();

	unsigned int block_index;

	// Write the block information to the cache file.
	// NOTE: Later we'll need to come back and fill out the block information.
	const unsigned int num_blocks = block_infos.get_num_blocks();
	for (block_index = 0; block_index < num_blocks; ++block_index)
	{
		RasterFileCacheFormat::BlockInfo &block_info =
				block_infos.get_block_info(block_index);

		// Set all values to zero - we'll come back later and fill it out properly.
		block_info.x_offset = 0;
		block_info.y_offset = 0;
		block_info.width = 0;
		block_info.height = 0;
		block_info.main_offset = 0;
		block_info.coverage_offset = 0;

		// Write out the dummy block information.
		out << block_info.x_offset
			<< block_info.y_offset
			<< block_info.width
			<< block_info.height
			<< block_info.main_offset
			<< block_info.coverage_offset;
	}

	// Raster statistics to calculate as we write the source raster file cache.
	//
	// NOTE: We no longer use "GDALRasterBand::GetStatistics()" because it can scan the entire
	// file and calculate the statistics if the file does not store statistics and for very large
	// files this can take a very long time. So now we calculate them ourselves as we read the file.
	//
	double raster_min = (std::numeric_limits<double>::max)();
	double raster_max = (std::numeric_limits<double>::min)();
	double raster_sum = 0;
	double raster_sum_squares = 0;
	qint64 num_valid_raster_samples = 0;

	// Write the source raster image to the cache file.
	write_source_raster_file_cache_image_data<RawRasterType>(
			band_number, cache_file, out, block_infos, read_errors,
			raster_min, raster_max, raster_sum, raster_sum_squares, num_valid_raster_samples);

	// Calculate the raster statistics from the accumulated raster data.
	//
	// mean = M = sum(Ci * Xi) / sum(Ci)
	// std_dev  = sqrt[sum(Ci * (Xi - M)^2) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) - 2 * M * sum(Ci * Xi) + M^2 * sum(Ci)) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) - 2 * M * M * sum(Ci) + M^2 * sum(Ci)) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) - M^2 * sum(Ci)) / sum(Ci)]
	//          = sqrt[(sum(Ci * Xi^2) / sum(Ci) - M^2]
	//
	// For the raster the coverage (or mask) values Ci can be 0.0 or 1.0.
	// So only the valid raster samples contribute.
	//
	// mean = M = sum(Xi) / N
	// std_dev  = sqrt[(sum(Xi^2) / N - M^2]
	//
	// ...where N is number of 'valid' raster samples (ie, samples that are not "no-data" values).
	//
	const double raster_mean = (num_valid_raster_samples > 0)
			? (raster_sum / num_valid_raster_samples)
			: 0;
	const double raster_variance = (num_valid_raster_samples > 0)
			? (raster_sum_squares / num_valid_raster_samples - raster_mean * raster_mean)
			: 0;
	// Protect 'sqrt' in case variance is slightly negative due to numerical precision.
	const double raster_std_dev = (raster_variance > 0) ? std::sqrt(raster_variance) : 0;

	// Now that we've calculated the raster statistics we can go back and write it to the cache file.
	cache_file.seek(statistics_file_offset);
	out << static_cast<quint32>(true); // has_raster_statistics
	out << static_cast<quint32>(true); // has_raster_minimum
	out << static_cast<quint32>(true); // has_raster_maximum
	out << static_cast<quint32>(true); // has_raster_mean
	out << static_cast<quint32>(true); // has_raster_standard_deviation
	out << static_cast<double>(raster_min);
	out << static_cast<double>(raster_max);
	out << static_cast<double>(raster_mean);
	out << static_cast<double>(raster_std_dev);

	// Now that we've initialised the block information we can go back and write it to the cache file.
	cache_file.seek(block_info_file_offset);
	for (block_index = 0; block_index < num_blocks; ++block_index)
	{
		const RasterFileCacheFormat::BlockInfo &block_info =
				block_infos.get_block_info(block_index);

		// Write out the proper block information.
		out << block_info.x_offset
			<< block_info.y_offset
			<< block_info.width
			<< block_info.height
			<< block_info.main_offset
			<< block_info.coverage_offset;
	}

	// Write the total size of the cache file so the reader can verify that the
	// file was not partially written.
	cache_file.seek(file_size_file_offset);
	total_cache_file_size = cache_file.size();
	out << total_cache_file_size;
}


template <class RawRasterType>
void
GPlatesFileIO::GDALRasterReader::write_source_raster_file_cache_image_data(
		unsigned int band_number,
		QFile &cache_file,
		QDataStream &out,
		RasterFileCacheFormat::BlockInfos &block_infos,
		ReadErrorAccumulation *read_errors,
		double &raster_min,
		double &raster_max,
		double &raster_sum,
		double &raster_sum_squares,
		qint64 &num_valid_raster_samples)
{
	// Find the smallest power-of-two that is greater than (or equal to) both the source
	// raster width and height - this will be used during the Hilbert curve traversal.
	const unsigned int source_raster_width_next_power_of_two =
			GPlatesUtils::Base2::next_power_of_two(d_source_width);
	const unsigned int source_raster_height_next_power_of_two =
			GPlatesUtils::Base2::next_power_of_two(d_source_height);
	const unsigned int source_raster_dimension_next_power_of_two =
			(std::max)(
					source_raster_width_next_power_of_two,
					source_raster_height_next_power_of_two);

	// The quad tree depth at which to write to the source raster file cache.
	// Each of these writes is of dimension RasterFileCacheFormat::BLOCK_SIZE (or less near the
	// right or bottom edges of the raster).
	unsigned int write_source_raster_depth = 0;
	if (source_raster_dimension_next_power_of_two > RasterFileCacheFormat::BLOCK_SIZE)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				GPlatesUtils::Base2::is_power_of_two(RasterFileCacheFormat::BLOCK_SIZE),
				GPLATES_ASSERTION_SOURCE);

		// The quad tree depth at which the dimension/coverage of a quad tree node is
		// 'RasterFileCacheFormat::BLOCK_SIZE'. Each depth increment reduces dimension by factor of two.
		write_source_raster_depth = GPlatesUtils::Base2::log2_power_of_two(
				source_raster_dimension_next_power_of_two / RasterFileCacheFormat::BLOCK_SIZE);
	}

	// The quad tree depth at which to read the source raster.
	// A depth of zero means read the entire raster once at the root of the quad tree.
	unsigned int read_source_raster_depth = 0;

	// For the reasons mentioned in the comment for 'MAX_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT' we're
	// not limiting the maximum image size (essentially the file seeks from one row of a sub-section
	// to the next row - or GDAL reads the entire row of the full-size image even if a sub-section
	// is requested - really slow down reading). In any case if an allocation fails then the
	// next quarter size allocation will be attempted and so on until a minimum allocation size.
#if 0
	// If necessary read the source raster deeper in the quad tree which means sub-regions of the
	// entire raster are read avoiding the possibility of memory allocation failures for very high
	// resolution source rasters.
	// Using 64-bit integer in case uncompressed image is larger than 4Gb.
	const qint64 image_size_in_bytes =
			quint64(d_source_width) * d_source_height * sizeof(RawRasterType::element_type);

	// Allocating higher than this is likely to cause memory to start paging to disk which
	// will just slow things down - so limit the size of partial read that we first attempt.
	if (image_size_in_bytes > MAX_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT)
	{
		qint64 image_allocation_size =
				// Using 64-bit integer in case uncompressed image is larger than 4Gb...
				qint64(source_raster_dimension_next_power_of_two) * source_raster_dimension_next_power_of_two *
					sizeof(RawRasterType::element_type);
		// Increase the read depth until the image allocation size is under the maximum.
		while (read_source_raster_depth < write_source_raster_depth)
		{
			++read_source_raster_depth;
			image_allocation_size /= 4;
			if (image_allocation_size < MAX_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT)
			{
				break;
			}
		}
	}
#endif

	// Some rasters have dimensions less than RasterFileCacheFormat::BLOCK_SIZE.
	const unsigned int dimension =
			(source_raster_dimension_next_power_of_two > RasterFileCacheFormat::BLOCK_SIZE)
			? source_raster_dimension_next_power_of_two
			: RasterFileCacheFormat::BLOCK_SIZE;

	// Traverse the Hilbert curve of blocks of the source raster using quad-tree recursion.
	// The leaf nodes of the traversal correspond to the blocks in the source raster.
	hilbert_curve_traversal<RawRasterType>(
			band_number,
			0/*depth*/,
			read_source_raster_depth,
			write_source_raster_depth,
			0/*x_offset*/,
			0/*y_offset*/,
			dimension,
			0/*hilbert_start_point*/,
			0/*hilbert_end_point*/,
			out,
			block_infos,
			boost::none, // No source region data read yet.
			QRect(), // A null rectangle - no source region yet.
			read_errors,
			raster_min,
			raster_max,
			raster_sum,
			raster_sum_squares,
			num_valid_raster_samples);
}


template <class RawRasterType>
void
GPlatesFileIO::GDALRasterReader::hilbert_curve_traversal(
		unsigned int band_number,
		unsigned int depth,
		unsigned int read_source_raster_depth,
		unsigned int write_source_raster_depth,
		unsigned int x_offset,
		unsigned int y_offset,
		unsigned int dimension,
		unsigned int hilbert_start_point,
		unsigned int hilbert_end_point,
		QDataStream &out,
		RasterFileCacheFormat::BlockInfos &block_infos,
		// The source raster data in the region covering the current quad tree node.
		// NOTE: This is only initialised when 'depth == read_source_raster_depth'.
		boost::optional<typename RawRasterType::non_null_ptr_type> source_region_data,
		QRect source_region,
		ReadErrorAccumulation *read_errors,
		double &raster_min,
		double &raster_max,
		double &raster_sum,
		double &raster_sum_squares,
		qint64 &num_valid_raster_samples)
{
	// See if the current quad-tree region is outside the source raster.
	// This can happen because the Hilbert traversal operates on power-of-two dimensions
	// which encompass the source raster (leaving regions that contain no source raster data).
	if (x_offset >= d_source_width || y_offset >= d_source_height)
	{
		return;
	}

	// If we've reached the depth at which to read from the source raster.
	// This depth is such that the entire source raster does not need to be read in (for those
	// raster formats that support partial reads) thus avoiding the possibility of memory allocation
	// failures for very high resolution rasters.
	if (depth == read_source_raster_depth)
	{
		// We not already have source region data from a parent quad tree node.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!source_region_data && !source_region.isValid(),
				GPLATES_ASSERTION_SOURCE);

		// Determine the region of the source raster covered by the current quad tree node.
		unsigned int source_region_width = d_source_width - x_offset;
		if (source_region_width > dimension)
		{
			source_region_width = dimension;
		}
		unsigned int source_region_height = d_source_height - y_offset;
		if (source_region_height > dimension)
		{
			source_region_height = dimension;
		}

		GDALRasterBand *band = get_raster_band(band_number, read_errors);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				band,
				GPLATES_ASSERTION_SOURCE);

		// Read the source raster data from the current region.
		source_region = QRect(x_offset, y_offset, source_region_width, source_region_height);

		source_region_data = read_data<RawRasterType>(band, d_flip, source_region);

		// If there was a memory allocation failure.
		if (!source_region_data)
		{
			// If:
			//  - the lower source region size is less than a minimum value, or
			//  - we're at the leaf quad tree node level,
			// then report insufficient memory.
			if (// Using 64-bit integer in case uncompressed image is larger than 4Gb...
				qint64(source_region.width() / 2) * (source_region.height() / 2) *
					sizeof(typename RawRasterType::element_type) <
						static_cast<qint64>(MIN_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT) ||
				read_source_raster_depth == write_source_raster_depth)
			{
				// Report insufficient memory to load raster.
				report_failure_to_begin(read_errors, ReadErrors::InsufficientMemoryToLoadRaster);

				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE, "Insufficient memory to load raster.");
			}

			// Keep reducing the source region until it succeeds or
			// we've reached a source region size that really should not fail.
			// We do this by attempting to read the source raster again at the child quad tree level
			// which is half the dimension of the current level.
			++read_source_raster_depth;

			// Invalidate the source region again - the child level will re-specify it.
			source_region = QRect();
		}

		// Update the raster statistics.
		if (source_region_data)
		{
			boost::function<bool (typename RawRasterType::element_type)> is_no_data_value_function =
					GPlatesPropertyValues::RawRasterUtils::get_is_no_data_value_function(
							*source_region_data.get());

			const typename RawRasterType::element_type *const source_region_data_array =
					source_region_data.get()->data();

			// Iterate over the source region.
			const qint64 num_values_in_region =
					source_region_data.get()->width() * source_region_data.get()->height();
			for (qint64 n = 0; n < num_values_in_region; ++n)
			{
				const typename RawRasterType::element_type value = source_region_data_array[n];

				// Only pixels with valid data contribute to the raster statistics.
				if (!is_no_data_value_function(value))
				{
					if (value < raster_min)
					{
						raster_min = value;
					}
					if (value > raster_max)
					{
						raster_max = value;
					}
					raster_sum += value;
					raster_sum_squares += value * value;
					++num_valid_raster_samples;
				}
			}
		}
	}

	// If we've reached the leaf node depth then write the source raster data to the cache file.
	if (depth == write_source_raster_depth)
	{
		// We should be the size of a block.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				dimension == RasterFileCacheFormat::BLOCK_SIZE,
				GPLATES_ASSERTION_SOURCE);

		// Get the current block based on the block x/y offsets.
		RasterFileCacheFormat::BlockInfo &block_info =
				block_infos.get_block_info(
						x_offset / RasterFileCacheFormat::BLOCK_SIZE,
						y_offset / RasterFileCacheFormat::BLOCK_SIZE);

		// The pixel offsets of the current block within the source raster.
		block_info.x_offset = x_offset;
		block_info.y_offset = y_offset;

		// For most blocks the dimensions will be RasterFileCacheFormat::BLOCK_SIZE but
		// for blocks near the right or bottom edge of source raster they can be less.
		block_info.width = d_source_width - x_offset;
		if (block_info.width > RasterFileCacheFormat::BLOCK_SIZE)
		{
			block_info.width = RasterFileCacheFormat::BLOCK_SIZE;
		}
		block_info.height = d_source_height - y_offset;
		if (block_info.height > RasterFileCacheFormat::BLOCK_SIZE)
		{
			block_info.height = RasterFileCacheFormat::BLOCK_SIZE;
		}

		// Record the file offset of the current block of data.
		block_info.main_offset = out.device()->pos();

		// TODO: Add coverage data.
		block_info.coverage_offset = 0;

		// We should already have source region data.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				source_region_data && source_region.isValid(),
				GPLATES_ASSERTION_SOURCE);

		// The current block should be contained within the source region.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				int(block_info.x_offset) >= source_region.x() &&
					int(block_info.y_offset) >= source_region.y() &&
					int(block_info.x_offset + block_info.width) <= source_region.x() + source_region.width() &&
					int(block_info.y_offset + block_info.height) <= source_region.y() + source_region.height(),
				GPLATES_ASSERTION_SOURCE);

		PROFILE_BLOCK("Write GDAL raster data to file cache");

		// Write the current block from the source region to the output stream.
		for (unsigned int y = 0; y < block_info.height; ++y)
		{
			const typename RawRasterType::element_type *const source_region_row =
					source_region_data.get()->data() +
						(block_info.y_offset - source_region.y() + y) * source_region.width() +
						block_info.x_offset - source_region.x();

			for (unsigned int x = 0; x < block_info.width; ++x)
			{
				out << source_region_row[x];
			}
		}

		return;
	}

	const unsigned int child_depth = depth + 1;
	const unsigned int child_dimension = (dimension >> 1);

	const unsigned int child_x_offset_hilbert0 = hilbert_start_point;
	const unsigned int child_y_offset_hilbert0 = hilbert_start_point;
	hilbert_curve_traversal<RawRasterType>(
			band_number,
			child_depth,
			read_source_raster_depth,
			write_source_raster_depth,
			x_offset + child_x_offset_hilbert0 * child_dimension,
			y_offset + child_y_offset_hilbert0 * child_dimension,
			child_dimension,
			hilbert_start_point,
			1 - hilbert_end_point,
			out,
			block_infos,
			source_region_data,
			source_region,
			read_errors,
			raster_min,
			raster_max,
			raster_sum,
			raster_sum_squares,
			num_valid_raster_samples);

	const unsigned int child_x_offset_hilbert1 = hilbert_end_point;
	const unsigned int child_y_offset_hilbert1 = 1 - hilbert_end_point;
	hilbert_curve_traversal<RawRasterType>(
			band_number,
			child_depth,
			read_source_raster_depth,
			write_source_raster_depth,
			x_offset + child_x_offset_hilbert1 * child_dimension,
			y_offset + child_y_offset_hilbert1 * child_dimension,
			child_dimension,
			hilbert_start_point,
			hilbert_end_point,
			out,
			block_infos,
			source_region_data,
			source_region,
			read_errors,
			raster_min,
			raster_max,
			raster_sum,
			raster_sum_squares,
			num_valid_raster_samples);

	const unsigned int child_x_offset_hilbert2 = 1 - hilbert_start_point;
	const unsigned int child_y_offset_hilbert2 = 1 - hilbert_start_point;
	hilbert_curve_traversal<RawRasterType>(
			band_number,
			child_depth,
			read_source_raster_depth,
			write_source_raster_depth,
			x_offset + child_x_offset_hilbert2 * child_dimension,
			y_offset + child_y_offset_hilbert2 * child_dimension,
			child_dimension,
			hilbert_start_point,
			hilbert_end_point,
			out,
			block_infos,
			source_region_data,
			source_region,
			read_errors,
			raster_min,
			raster_max,
			raster_sum,
			raster_sum_squares,
			num_valid_raster_samples);

	const unsigned int child_x_offset_hilbert3 = 1 - hilbert_end_point;
	const unsigned int child_y_offset_hilbert3 = hilbert_end_point;
	hilbert_curve_traversal<RawRasterType>(
			band_number,
			child_depth,
			read_source_raster_depth,
			write_source_raster_depth,
			x_offset + child_x_offset_hilbert3 * child_dimension,
			y_offset + child_y_offset_hilbert3 * child_dimension,
			child_dimension,
			1 - hilbert_start_point,
			hilbert_end_point,
			out,
			block_infos,
			source_region_data,
			source_region,
			read_errors,
			raster_min,
			raster_max,
			raster_sum,
			raster_sum_squares,
			num_valid_raster_samples);
}
