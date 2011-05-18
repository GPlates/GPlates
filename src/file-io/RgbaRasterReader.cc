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

#include <iostream>
#include <boost/scoped_array.hpp>
#include <QtCore/qglobal.h>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

#include "RgbaRasterReader.h"

#include "TemporaryFileRegistry.h"

#include "utils/Profile.h"


GPlatesFileIO::RgbaRasterReader::RgbaRasterReader(
		const QString &filename,
		const boost::function<RasterBandReaderHandle (unsigned int)> &proxy_handle_function,
		ReadErrorAccumulation *read_errors) :
	d_filename(filename),
	d_proxy_handle_function(proxy_handle_function),
	d_source_width(0),
	d_source_height(0),
	d_can_read(false)
{
	QImageReader reader(d_filename);
	if (reader.canRead())
	{
		QSize size = reader.size();
		d_source_width = size.width();
		d_source_height = size.height();
	}

	if (d_source_width > 0 && d_source_height > 0)
	{
		d_can_read = true;
	}

	// Do all the failure to begin reporting here, so that there's only one
	// such report per file.
	if (!d_can_read)
	{
		report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
	}
}


bool
GPlatesFileIO::RgbaRasterReader::can_read()
{
	return d_can_read;
}


unsigned int
GPlatesFileIO::RgbaRasterReader::get_number_of_bands(
		ReadErrorAccumulation *read_errors)
{
	if (d_can_read)
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
	if (!d_can_read)
	{
		return boost::none;
	}

	if (band_number != 1)
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return boost::none;
	}

	if (!ensure_rgba_file_available(read_errors))
	{
		return boost::none;
	}

	GPlatesPropertyValues::ProxiedRgba8RawRaster::non_null_ptr_type result =
		GPlatesPropertyValues::ProxiedRgba8RawRaster::create(
				d_source_width, d_source_height, d_proxy_handle_function(band_number));
	return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RgbaRasterReader::get_raw_raster(
		unsigned int band_number,
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	if (!d_can_read)
	{
		return boost::none;
	}

	if (band_number != 1)
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return boost::none;
	}

	if (!ensure_rgba_file_available(read_errors))
	{
		return boost::none;
	}

	GPlatesGui::rgba8_t *data = read_rgba_file(region);
	if (!data)
	{
		report_recoverable_error(read_errors, ReadErrors::InvalidRegionInRaster);
	}

	GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type result =
		region.isValid() ?
		GPlatesPropertyValues::Rgba8RawRaster::create(region.width(), region.height(), data) :
		GPlatesPropertyValues::Rgba8RawRaster::create(d_source_width, d_source_height, data);

	return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::RgbaRasterReader::get_type(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	if (!d_can_read)
	{
		return GPlatesPropertyValues::RasterType::UNKNOWN;
	}

	if (band_number != 1)
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return GPlatesPropertyValues::RasterType::UNKNOWN;
	}

	// We only read RGBA rasters with ImageMagick.
	return GPlatesPropertyValues::RasterType::RGBA8;
}


void *
GPlatesFileIO::RgbaRasterReader::get_data(
		unsigned int band_number,
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	if (!d_can_read)
	{
		return NULL;
	}

	if (band_number != 1)
	{
		report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
		return NULL;
	}

	if (!ensure_rgba_file_available(read_errors))
	{
		return NULL;
	}

	GPlatesGui::rgba8_t *data = read_rgba_file(region);
	if (!data)
	{
		report_recoverable_error(read_errors, ReadErrors::InvalidRegionInRaster);
	}

	return data;
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
					d_filename,
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
					d_filename,
					DataFormats::RasterImage,
					0,
					description,
					ReadErrors::FileNotLoaded));
	}
}


GPlatesGui::rgba8_t *
GPlatesFileIO::RgbaRasterReader::read_rgba_file(
		const QRect &region)
{
	unsigned int region_width, region_height, region_x_offset, region_y_offset;

	// Check the region.
	if (region.isValid())
	{
		if (region.x() < 0 || region.y() < 0 ||
				region.width() <= 0 || region.height() <= 0 ||
				static_cast<unsigned int>(region.x() + region.width()) > d_source_width ||
				static_cast<unsigned int>(region.y() + region.height()) > d_source_height)
		{
			return NULL;
		}

		region_width = static_cast<unsigned int>(region.width());
		region_height = static_cast<unsigned int>(region.height());
		region_x_offset = static_cast<unsigned int>(region.x());
		region_y_offset = static_cast<unsigned int>(region.y());
	}
	else
	{
		region_width = d_source_width;
		region_height = d_source_height;
		region_x_offset = 0;
		region_y_offset = 0;
	}

	//PROFILE_BLOCK("Read RGBA");

	GPlatesGui::rgba8_t * const dest_buf = new GPlatesGui::rgba8_t[region_width * region_height];
	GPlatesGui::rgba8_t *dest_ptr = dest_buf;

	// Read the bytes in from the file.
	for (unsigned int y = 0; y != region_height; ++y)
	{
		unsigned int row_in_file = y + region_y_offset;
		// Need to use 64-bit arithmetic when calculating file offsets since
		// some uncompressed rasters are already larger than 4Gb.
		d_rgba_file.seek(
				(qint64(row_in_file) * d_source_width + region_x_offset) * sizeof(GPlatesGui::rgba8_t));

		// Read the raw byte data for the row since it's *significantly* quicker than streaming
		// each pixel individually (as determined by profiling) - Qt does a lot at each '>>'.
		input_pixels(d_rgba_in, dest_ptr, region_width);

		// Move to the next row in the region.
		dest_ptr += region_width;
	}

	return dest_buf;
}


bool
GPlatesFileIO::RgbaRasterReader::ensure_rgba_file_available(
		ReadErrorAccumulation *read_errors)
{
	if (d_rgba_in.device())
	{
		// The RGBA file exists and is already open.
		return true;
	}

	// Is there such a file in the same directory?
	static const QString EXTENSION = ".mipmap-level0";
	QString rgba_filename;
	QString in_same_directory = d_filename + EXTENSION;
	QString in_tmp_directory = TemporaryFileRegistry::make_filename_in_tmp_directory(
			in_same_directory);
	if (QFile(in_same_directory).exists())
	{
		rgba_filename = in_same_directory;
	}
	else if (QFile(in_tmp_directory).exists())
	{
		rgba_filename = in_tmp_directory;
	}

	if (!rgba_filename.isEmpty())
	{
		// Check whether the RGBA file was created after the source file.
		if (QFileInfo(rgba_filename).lastModified() >
				QFileInfo(d_filename).lastModified())
		{
			// Attempt to open it.
			d_rgba_file.setFileName(rgba_filename);
			if (d_rgba_file.open(QIODevice::ReadOnly))
			{
				// Check that the RGBA file is the correct size - in case
				// it was partially written to by a previous instance of GPlates.
				// For example, if an error occurred part way through writing the file.
				QImageReader reader(d_filename);
				if (!reader.canRead())
				{
					report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
					// Can't open the image file so nothing more can be done.
					return false;
				}

				// All Qt-provided formats support image size queries.
				const QSize image_size = reader.size();
				if (d_rgba_file.size() ==
					qint64(image_size.width()) * qint64(image_size.height()) * qint64(sizeof(GPlatesGui::rgba8_t)))
				{
					// Existing RGBA file is the expected size.
					d_rgba_in.setDevice(&d_rgba_file);
					return true;
				}

				// Close the file - it'll get reopened for writing.
				d_rgba_file.close();
			}

			// Fall through, it should then attempt to write one in the temp directory.
		}
		else
		{
			// It's too old, delete it.
			QFile(rgba_filename).remove();
		}
	}

	// Try to open an RGBA file in the same directory for writing.
	d_rgba_file.setFileName(in_same_directory);
	bool can_open = d_rgba_file.open(QIODevice::WriteOnly | QIODevice::Truncate);

	// If that fails, try and open an RGBA file in the tmp directory for writing.
	if (!can_open)
	{
		d_rgba_file.setFileName(in_tmp_directory);
		can_open = d_rgba_file.open(QIODevice::WriteOnly | QIODevice::Truncate);

		if (!can_open)
		{
			report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
			return false;
		}
	}

	// Read the image and write it to the currently open RGBA file.
	if (!write_image_to_rgba_file(read_errors))
	{
		// Close the RGBA file and remove it.
		d_rgba_file.remove();

		return false;
	}

	// Close the RGBA file.
	d_rgba_file.close();

	// Copy the file permissions from the source raster to the RGBA file.
	d_rgba_file.setPermissions(QFile::permissions(d_filename));

	// Open the same file again for reading.
	if (d_rgba_file.open(QIODevice::ReadOnly))
	{
		d_rgba_in.setDevice(&d_rgba_file);
		return true;
	}

	report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
	return false;
}


bool
GPlatesFileIO::RgbaRasterReader::write_image_to_rgba_file(
		ReadErrorAccumulation *read_errors)
{
	PROFILE_FUNC();

	QImageReader reader(d_filename);
	if (!reader.canRead())
	{
		report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
		return false;
	}

	// All Qt-provided formats support image size queries.
	const QSize image_size = reader.size();

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

	if (reader.supportsOption(QImageIOHandler::ClipRect))
	{
		// Using 64-bit integer in case uncompressed image is larger than 4Gb.
		const qint64 image_size_in_bytes =
				quint64(image_size.width()) * image_size.height() * sizeof(GPlatesGui::rgba8_t);

		if (image_size_in_bytes > MIN_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT)
		{
			return write_image_to_rgba_file_using_clip_rects(image_size, read_errors);
		}
	}
	// ...either the image is not large enough to worry about memory allocation failure or
	// the image can only be read into a single QImage covering the entire image (in which
	// case a memory allocation failure can still occur, but there's not much that can be
	// done about that other than use something besides Qt image reading).

	// Read the entire image into memory.
	const QImage image = reader.read();
	if (image.isNull())
	{
		// Most likely ran out of memory because image was too large - we couldn't read it
		// into sub-rectangles so this can happen for super-large rasters.
		// Report insufficient memory to load raster.
		report_failure_to_begin(read_errors, ReadErrors::InsufficientMemoryToLoadRaster);
		return false;
	}

	// And then write the image out as RGBA.
	QDataStream out(&d_rgba_file);

	// Write the image rect to the output file.
	if (!convert_image_to_gl_and_append_to_rgba_file(image, out, read_errors))
	{
		return false;
	}

	return true;
}


bool
GPlatesFileIO::RgbaRasterReader::write_image_to_rgba_file_using_clip_rects(
		const QSize &image_size,
		ReadErrorAccumulation *read_errors)
{
	// Divide image into rectangles spanning the entire width of image
	// but partially spanning the height.
	const int image_width = image_size.width();
	const int image_height = image_size.height();

	// The min/max clip rect sizes to try.
	const int min_rows_per_clip_rect = MIN_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT /
			(image_width * sizeof(GPlatesGui::rgba8_t));
	const int max_rows_per_clip_rect = MAX_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT /
			(image_width * sizeof(GPlatesGui::rgba8_t));

	// Start off attempting the largest clip rect size and reduce if memory allocation fails.
	int rows_per_clip_rect = max_rows_per_clip_rect;

	// The output RGBA file.
	QDataStream out(&d_rgba_file);

	int y = 0;
	while (y < image_height)
	{
		int clip_rect_height = image_height - y;
		if (clip_rect_height > rows_per_clip_rect)
		{
			clip_rect_height = rows_per_clip_rect;
		}

		// The input image reader.
		QImageReader reader(d_filename);
		if (!reader.canRead())
		{
			report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
			return false;
		}

		// Only want to read the current set of 'clip_rect_height' rows.
		reader.setClipRect(QRect(0, y, image_width, clip_rect_height));

		// Read the clip rectangle.
		PROFILE_BEGIN(read_jpg, "read jpeg");
		QImage image_rect = reader.read();
		PROFILE_END(read_jpg);
		if (image_rect.isNull())
		{
			// Most likely a memory allocation failure.

			// If we've already reduced the clip rect size to a reasonable value and
			// it still fails then report an error.
			if (rows_per_clip_rect < min_rows_per_clip_rect)
			{
				// Report insufficient memory to load raster.
				report_failure_to_begin(read_errors, ReadErrors::InsufficientMemoryToLoadRaster);
				return false;
			}

			// Keep reducing the clip rect height until it succeeds or
			// we've reached a clip rect size that really should not fail.
			// Half the clip rect height and try again.
			rows_per_clip_rect >>= 1;

			continue;
		}

		// Write the image rect to the output file.
		if (!convert_image_to_gl_and_append_to_rgba_file(image_rect, out, read_errors))
		{
			return false;
		}

		// Move to start of the next clip rectangle.
		y += clip_rect_height;
	}

	return true;
}


bool
GPlatesFileIO::RgbaRasterReader::convert_image_to_gl_and_append_to_rgba_file(
		const QImage &image,
		QDataStream &out,
		ReadErrorAccumulation *read_errors)
{
	//
	// We've just loaded an image - it could be any size so we want to minimise the
	// risk of a memory allocation failure by converting it to GL format and appending to
	// the output file in *sections*.
	//

	const int image_width = image.width();
	const int image_height = image.height();
	int max_rows_per_convert = MAX_BYTES_TO_CONVERT_IMAGE_TO_ARGB32_FORMAT /
			(image_width * sizeof(GPlatesGui::rgba8_t));
	if (max_rows_per_convert == 0)
	{
		// Shouldn't happen but just in case image is super-large and
		// MAX_BYTES_TO_CONVERT_IMAGE_TO_ARGB32_FORMAT gets set to super-small for some reason.
		max_rows_per_convert = 1;
	}

	// A buffer to storage a single row of pixels.
	boost::scoped_array<GPlatesGui::rgba8_t> row_buf(new GPlatesGui::rgba8_t[image_width]);

	int y = 0;
	while (y < image_height)
	{
		int num_rows_per_convert = image_height - y;
		if (num_rows_per_convert > max_rows_per_convert)
		{
			num_rows_per_convert = max_rows_per_convert;
		}

		// Only want to convert the current set of 'num_rows_per_convert' rows.
		const QImage image_rect = image.copy(0, y, image_width, num_rows_per_convert);

		// Convert the image into ARGB32 format.
		const QImage argb_image_rect = image_rect.convertToFormat(QImage::Format_ARGB32);
		if (argb_image_rect.isNull())
		{
			// Most likely ran out of memory - shouldn't happen here.
			// Report insufficient memory to load raster.
			report_failure_to_begin(read_errors, ReadErrors::InsufficientMemoryToLoadRaster);
			return false;
		}

		const unsigned int image_rect_height = argb_image_rect.height();

		//PROFILE_BLOCK("Write to RGBA file");

		// Iterate over the rows.
		for (unsigned int i = 0; i < image_rect_height; ++i)
		{
			// Convert QImage::Format_ARGB32 format to GPlatesGui::rgba8_t.
			convert_argb32_to_rgba8(
					reinterpret_cast<const boost::uint32_t *>(argb_image_rect.scanLine(i)),
					row_buf.get(),
					image_width);

			// Output converted row of pixels to the data stream.
			output_pixels(out, row_buf.get(), image_width);
		}

		// Move to start of the next group of rows to convert.
		y += num_rows_per_convert;
	}

	return true;
}
