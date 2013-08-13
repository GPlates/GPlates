/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
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

#include <cstddef> // For std::size_t
#include <exception>
#include <iostream>
#include <limits>
#include <boost/scoped_array.hpp>
#include <QtCore/qglobal.h>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "RgbaRasterReader.h"

#include "ErrorOpeningFileForReadingException.h"
#include "ErrorOpeningFileForWritingException.h"
#include "RasterFileCacheFormat.h"
#include "TemporaryFileRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


namespace GPlatesFileIO
{
	namespace
	{
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
	}
}


GPlatesFileIO::RgbaRasterReader::RgbaRasterReader(
		const QString &filename,
		RasterReader *raster_reader,
		ReadErrorAccumulation *read_errors) :
	RasterReaderImpl(raster_reader),
	d_source_raster_filename(filename),
	d_source_width(0),
	d_source_height(0)
{
	// Open the source raster for reading to determine the source raster dimensions.
	QImageReader source_reader(d_source_raster_filename);
	if (!source_reader.canRead())
	{
		report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
		return;
	}

	// All Qt-provided formats support image size queries.
	const QSize image_size = source_reader.size();
	d_source_width = image_size.width();
	d_source_height = image_size.height();

	// Create the source raster file cache if there isn't one or it's out of date.
	create_source_raster_file_cache_format_reader(read_errors);

	if (!d_source_raster_file_cache_format_reader)
	{
		// We were unable to create a source raster file cache or unable to read it.
		report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
		return;
	}
}


bool
GPlatesFileIO::RgbaRasterReader::can_read()
{
	// Return true if we have successfully created a source raster file cache format reader.
	return d_source_raster_file_cache_format_reader;
}


unsigned int
GPlatesFileIO::RgbaRasterReader::get_number_of_bands(
		ReadErrorAccumulation *read_errors)
{
	if (can_read())
	{
		// We only read single-band rasters with Qt.
		return 1;
	}
	else
	{
		// 0 flags error.
		return 0;
	}
}


std::pair<unsigned int, unsigned int>
GPlatesFileIO::RgbaRasterReader::get_size(
		ReadErrorAccumulation *read_errors)
{
	return std::make_pair(d_source_width, d_source_height);
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RgbaRasterReader::get_proxied_raw_raster(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	if (!can_read())
	{
		return boost::none;
	}

	if (band_number != 1)
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return boost::none;
	}

	GPlatesPropertyValues::ProxiedRgba8RawRaster::non_null_ptr_type result =
		GPlatesPropertyValues::ProxiedRgba8RawRaster::create(
				d_source_width,
				d_source_height,
				create_raster_band_reader_handle(band_number));

	return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RgbaRasterReader::get_raw_raster(
		unsigned int band_number,
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	if (!can_read())
	{
		return boost::none;
	}

	if (band_number != 1)
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

	if (!d_source_raster_file_cache_format_reader)
	{
		return boost::none;
	}

	// Read the specified source region from the raster file cache.
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> data =
			d_source_raster_file_cache_format_reader->read_raster(
					region_x_offset, region_y_offset, region_width, region_height);

	if (!data)
	{
		report_recoverable_error(read_errors, ReadErrors::InvalidRegionInRaster);
		return boost::none;
	}

	return data.get();
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::RgbaRasterReader::get_type(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	if (!can_read())
	{
		return GPlatesPropertyValues::RasterType::UNKNOWN;
	}

	if (band_number != 1)
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return GPlatesPropertyValues::RasterType::UNKNOWN;
	}

	// We only read RGBA rasters (with Qt).
	return GPlatesPropertyValues::RasterType::RGBA8;
}


void
GPlatesFileIO::RgbaRasterReader::report_recoverable_error(
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
GPlatesFileIO::RgbaRasterReader::report_failure_to_begin(
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


void
GPlatesFileIO::RgbaRasterReader::create_source_raster_file_cache_format_reader(
		ReadErrorAccumulation *read_errors)
{
	d_source_raster_file_cache_format_reader.reset();

	// Find the existing source raster file cache (if exists).
	boost::optional<QString> cache_filename =
			RasterFileCacheFormat::get_existing_source_cache_filename(d_source_raster_filename, 1/*band_number*/);
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
			if (!create_source_raster_file_cache(read_errors))
			{
				// Unable to create cache file.
				return;
			}
		}
	}
	// Generate the cache file if it doesn't exist...
	else
	{
		if (!create_source_raster_file_cache(read_errors))
		{
			// Unable to create cache file.
			return;
		}

		cache_filename =
				RasterFileCacheFormat::get_existing_source_cache_filename(d_source_raster_filename, 1/*band_number*/);
		if (!cache_filename)
		{
			// Cache file was created but unable to read it for some reason.
			return;
		}
	}

	try
	{
		try
		{
			// Attempt to create the source raster file cache format reader.
			d_source_raster_file_cache_format_reader.reset(
					new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::Rgba8RawRaster>(
							cache_filename.get()));
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
			if (create_source_raster_file_cache(read_errors))
			{
				// Try reading it again.
				d_source_raster_file_cache_format_reader.reset(
						new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::Rgba8RawRaster>(
								cache_filename.get()));
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
			if (create_source_raster_file_cache(read_errors))
			{
				// Try reading it again.
				d_source_raster_file_cache_format_reader.reset(
						new SourceRasterFileCacheFormatReaderImpl<GPlatesPropertyValues::Rgba8RawRaster>(
								cache_filename.get()));
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
}


bool
GPlatesFileIO::RgbaRasterReader::create_source_raster_file_cache(
		ReadErrorAccumulation *read_errors)
{
	PROFILE_FUNC();

	boost::optional<QString> cache_filename =
			RasterFileCacheFormat::get_writable_source_cache_filename(d_source_raster_filename, 1/*band_number*/);

	if (!cache_filename)
	{
		// Can't write raster file cache anywhere.
		return false;
	}

	// Write the cache file.
	try
	{
		write_source_raster_file_cache(cache_filename.get(), read_errors);

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


void
GPlatesFileIO::RgbaRasterReader::write_source_raster_file_cache(
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
	const qint64 file_size_offset = cache_file.pos();
	qint64 total_cache_file_size = 0;
	out << static_cast<qint64>(total_cache_file_size);

	// Write version number.
	out << static_cast<quint32>(RasterFileCacheFormat::VERSION_NUMBER);

	// Write source raster type.
	out << static_cast<quint32>(
			RasterFileCacheFormat::get_type_as_enum<GPlatesPropertyValues::Rgba8RawRaster::element_type>());

	// No coverage is necessary for RGBA rasters (it's embedded in the alpha channel).
	out << static_cast<quint32>(false/*has_coverage*/);

	// Write the source raster dimensions.
	out << static_cast<quint32>(d_source_width);
	out << static_cast<quint32>(d_source_height);

	// The source raster will get written to the cache file in blocks.
	RasterFileCacheFormat::BlockInfos block_infos(d_source_width, d_source_height);

	// Write the number of blocks in the source raster.
	out << static_cast<quint32>(block_infos.get_num_blocks());

	// Write the (optional) raster no-data value.
	//
	// NOTE: The source raster is RGBA which does not have a no-data value.
	out << static_cast<quint32>(false);
	out << GPlatesPropertyValues::Rgba8RawRaster::element_type(0, 0, 0, 0/*RGBA*/); // Doesn't matter what gets stored.

	// Write the (optional) raster statistics.
	//
	// NOTE: The source raster is RGBA which does not have raster statistics.
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
	const qint64 block_info_pos = cache_file.pos();

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

	// Write the source raster image to the cache file.
	write_source_raster_file_cache_image_data(cache_file, out, block_infos, read_errors);

	// Now that we've initialised the block information we can go back and write it to the cache file.
	cache_file.seek(block_info_pos);
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
	cache_file.seek(file_size_offset);
	total_cache_file_size = cache_file.size();
	out << total_cache_file_size;
}


void
GPlatesFileIO::RgbaRasterReader::write_source_raster_file_cache_image_data(
		QFile &cache_file,
		QDataStream &out,
		RasterFileCacheFormat::BlockInfos &block_infos,
		ReadErrorAccumulation *read_errors)
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
	// Only if partial reads are supported for the source raster file format can we read the source
	// raster more than once (in sub-regions).
	unsigned int read_source_raster_depth = 0;

	// If the source raster file format supports partial reads (ie, not forced to read entire image)
	// then we can read the source raster deeper in the quad tree which means sub-regions of the
	// entire raster are read avoiding the possibility of memory allocation failures for very high
	// resolution source rasters.
	if (QImageReader(d_source_raster_filename).supportsOption(QImageIOHandler::ClipRect))
	{
		// Using 64-bit integer in case uncompressed image is larger than 4Gb.
		const quint64 image_size_in_bytes =
				quint64(d_source_width) * d_source_height * sizeof(GPlatesGui::rgba8_t);

		// If we're not compiled for 64-bit and the image size is greater than 32 bits then reduce size.
		if (sizeof(std::size_t) < 8 &&
			image_size_in_bytes > Q_UINT64_C(0xffffffff))
		{
			quint64 image_allocation_size =
					// Using 64-bit integer in case uncompressed image is larger than 4Gb...
					quint64(source_raster_dimension_next_power_of_two) * source_raster_dimension_next_power_of_two *
						sizeof(GPlatesGui::rgba8_t);
			// Increase the read depth until the image allocation size is under the maximum.
			while (read_source_raster_depth < write_source_raster_depth)
			{
				++read_source_raster_depth;
				image_allocation_size /= 4;
				if (image_allocation_size < Q_UINT64_C(0xffffffff))
				{
					break;
				}
			}
		}
	}

	// Some rasters have dimensions less than RasterFileCacheFormat::BLOCK_SIZE.
	const unsigned int dimension =
			(source_raster_dimension_next_power_of_two > RasterFileCacheFormat::BLOCK_SIZE)
			? source_raster_dimension_next_power_of_two
			: RasterFileCacheFormat::BLOCK_SIZE;

	// Traverse the Hilbert curve of blocks of the source raster using quad-tree recursion.
	// The leaf nodes of the traversal correspond to the blocks in the source raster.
	hilbert_curve_traversal(
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
			boost::shared_array<GPlatesGui::rgba8_t>(), // No source region data read yet.
			QRect(), // A null rectangle - no source region yet.
			read_errors);
}


void
GPlatesFileIO::RgbaRasterReader::hilbert_curve_traversal(
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
		boost::shared_array<GPlatesGui::rgba8_t> source_region_data,
		QRect source_region,
		ReadErrorAccumulation *read_errors)
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

		// Open the source raster for reading.
		QImageReader source_reader(d_source_raster_filename);
		if (!source_reader.canRead())
		{
			throw ErrorOpeningFileForReadingException(
					GPLATES_EXCEPTION_SOURCE, d_source_raster_filename);
		}

		// Read the source raster data from the current region.
		source_region = QRect(x_offset, y_offset, source_region_width, source_region_height);
		source_region_data = read_source_raster_region(source_reader, source_region, read_errors);

		// If there was a memory allocation failure.
		if (!source_region_data)
		{
			// If:
			//  - the source raster format does not support clip rects, or
			//  - the lower clip rect size is less than a minimum value, or
			//  - we're at the leaf quad tree node level,
			// then report insufficient memory.
			if (!source_reader.supportsOption(QImageIOHandler::ClipRect) ||
				// Using 64-bit integer in case uncompressed image is larger than 4Gb...
				qint64(source_region.width() / 2) * (source_region.height() / 2) * sizeof(GPlatesGui::rgba8_t) <
					static_cast<qint64>(MIN_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT) ||
				read_source_raster_depth == write_source_raster_depth)
			{
				// Report insufficient memory to load raster.
				report_failure_to_begin(read_errors, ReadErrors::InsufficientMemoryToLoadRaster);

				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE, "Insufficient memory to load raster.");
			}

			// Keep reducing the source region until it succeeds or
			// we've reached a clip rect size that really should not fail.
			// We do this by attempting to read the source raster again at the child quad tree level
			// which is half the dimension of the current level.
			++read_source_raster_depth;

			// Invalidate the source region again - the child level will re-specify it.
			source_region = QRect();
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

		// NOTE: There's no coverage data for RGBA rasters.
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

		PROFILE_BLOCK("Write Rgba raster data to file cache");

		// Write the current block from the source region to the output stream.
		for (unsigned int y = 0; y < block_info.height; ++y)
		{
			const GPlatesGui::rgba8_t *const source_region_row =
					source_region_data.get() +
						// Using std::size_t in case 64-bit and in case source region is larger than 4Gb...
						std::size_t(block_info.y_offset - source_region.y() + y) * source_region.width() +
						block_info.x_offset - source_region.x();

#if 1
			output_pixels(out, source_region_row, block_info.width);
#else
			for (unsigned int x = 0; x < block_info.width; ++x)
			{
				out << source_region_row[x];
			}
#endif

		}

		return;
	}

	const unsigned int child_depth = depth + 1;
	const unsigned int child_dimension = (dimension >> 1);

	const unsigned int child_x_offset_hilbert0 = hilbert_start_point;
	const unsigned int child_y_offset_hilbert0 = hilbert_start_point;
	hilbert_curve_traversal(
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
			read_errors);

	const unsigned int child_x_offset_hilbert1 = hilbert_end_point;
	const unsigned int child_y_offset_hilbert1 = 1 - hilbert_end_point;
	hilbert_curve_traversal(
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
			read_errors);

	const unsigned int child_x_offset_hilbert2 = 1 - hilbert_start_point;
	const unsigned int child_y_offset_hilbert2 = 1 - hilbert_start_point;
	hilbert_curve_traversal(
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
			read_errors);

	const unsigned int child_x_offset_hilbert3 = 1 - hilbert_end_point;
	const unsigned int child_y_offset_hilbert3 = hilbert_end_point;
	hilbert_curve_traversal(
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
			read_errors);
}


boost::shared_array<GPlatesGui::rgba8_t> 
GPlatesFileIO::RgbaRasterReader::read_source_raster_region(
		QImageReader &source_reader,
		const QRect &source_region,
		ReadErrorAccumulation *read_errors)
{
	PROFILE_FUNC();

	//
	// To avoid a memory allocation failure we try not to read very large images into a single image array.
	//
	// Very large images are read in sections (where supported) to avoid a memory allocation failure.
	//
	// Currently JPEG with its clip-rect support in Qt 4.6.1 (see below) should be able to read
	// *any* resolution image without a memory allocation failure.
	//
	// The other formats (not supporting clip-rect) can fail on memory allocation - probably
	// will happen when image is higher resolution than a global 1-minute resolution image
	// (~20000 x 10000) on 32-bit systems (especially Windows where 32-bit processes only
	// get 2GB user-mode virtual address space unless /LARGEADDRESSAWARE linker option set in which
	// case can get ~3GB on 32-bit OS or 4GB on 64-bit OS).
	//

	if (source_reader.supportsOption(QImageIOHandler::ClipRect))
	{
		// Only want to read the specified source region.
		//
		// NOTE: We want a new instance of QImageReader for each clip rect read.
		// Otherwise the data will not be read properly (at least this was the case for jpeg files).
		source_reader.setClipRect(source_region);
	}
	else
	{
		// If the source reader doesn't support clip rects then we must be reading the entire source raster.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				source_region.x() == 0 &&
					source_region.y() == 0 &&
					source_region.width() == int(d_source_width) &&
					source_region.height() == int(d_source_height),
				GPLATES_ASSERTION_SOURCE);
	}

	// Read the clip rectangle.
	PROFILE_BEGIN(read_qimage, "QImageReader::read");
	QImage source_region_image = source_reader.read();
	PROFILE_END(read_qimage);
	if (source_region_image.isNull())
	{
		// Most likely a memory allocation failure.
		return boost::shared_array<GPlatesGui::rgba8_t>();
	}

	// The source region data to return to the caller.
	boost::shared_array<GPlatesGui::rgba8_t> source_raster_rgba_data;

	try
	{
		source_raster_rgba_data.reset(
				new GPlatesGui::rgba8_t[
						// Using std::size_t in case 64-bit and in case uncompressed image is larger than 4Gb...
						std::size_t(source_region.width()) * source_region.height()]);
	}
	catch (std::bad_alloc &)
	{
		// Memory allocation failure.
		return boost::shared_array<GPlatesGui::rgba8_t>();
	}

	// Convert each row of the source region to RGBA8.
	for (int y = 0; y < source_region.height(); ++y)
	{
		const QImage source_region_row = source_region_image.copy(0, y, source_region.width(), 1/*height*/);
		if (source_region_row.isNull())
		{
			// Most likely ran out of memory - shouldn't happen since only a single row allocated.
			return boost::shared_array<GPlatesGui::rgba8_t>();
		}

		// Convert the row into ARGB32 format.
		const QImage source_region_row_argb = source_region_row.convertToFormat(QImage::Format_ARGB32);
		if (source_region_row_argb.isNull())
		{
			// Most likely ran out of memory - shouldn't happen since only a single row allocated.
			return boost::shared_array<GPlatesGui::rgba8_t>();
		}

		// Convert the current row from QImage::Format_ARGB32 format to GPlatesGui::rgba8_t.
		convert_argb32_to_rgba8(
				reinterpret_cast<const boost::uint32_t *>(source_region_row_argb.scanLine(0)),
				// Using std::size_t in case 64-bit and in case uncompressed image is larger than 4Gb...
				source_raster_rgba_data.get() + std::size_t(y) * source_region.width(),
				source_region.width());
	}

	return source_raster_rgba_data;
}
