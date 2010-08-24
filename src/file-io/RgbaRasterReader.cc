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
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QImageReader>
#include <QGLWidget>

#include "RgbaRasterReader.h"

#include "TemporaryFileRegistry.h"


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
		// We only read single-band rasters with ImageMagick.
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

	if (!ensure_rgba_file_available())
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

	if (!ensure_rgba_file_available())
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

	if (!ensure_rgba_file_available())
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

	GPlatesGui::rgba8_t * const dest_buf = new GPlatesGui::rgba8_t[region_width * region_height];
	GPlatesGui::rgba8_t *dest_ptr = dest_buf;

	// Read the bytes in from the file.
	for (unsigned int y = 0; y != region_height; ++y)
	{
		unsigned int row_in_file = y + region_y_offset;
		d_rgba_file.seek((row_in_file * d_source_width + region_x_offset) * sizeof(GPlatesGui::rgba8_t));
		
		for (unsigned int x = 0; x != region_width; ++x)
		{
			d_rgba_in >> *dest_ptr;
			++dest_ptr;
		}
	}

	return dest_buf;
}


bool
GPlatesFileIO::RgbaRasterReader::ensure_rgba_file_available()
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
				d_rgba_in.setDevice(&d_rgba_file);
				return true;
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
			return false;
		}
	}

	// Read the entire image into memory and then write it out as RGBA.
	// Note that Qt reads images with the first scanline as the bottom of the image.
	QImageReader reader(d_filename);
	if (!reader.canRead())
	{
		return false;
	}
	QImage rgba_image = QGLWidget::convertToGLFormat(reader.read());

	QDataStream out(&d_rgba_file);
	for (unsigned int i = d_source_height; i != 0; --i)
	{
		GPlatesGui::rgba8_t *data_ptr = reinterpret_cast<GPlatesGui::rgba8_t *>(rgba_image.scanLine(i - 1));
		GPlatesGui::rgba8_t *data_end = data_ptr + d_source_width;
		while (data_ptr != data_end)
		{
			out << *data_ptr;
			++data_ptr;
		}
	}
	d_rgba_file.close();

	// Copy the file permissions from the source raster to the RGBA file.
	d_rgba_file.setPermissions(QFile::permissions(d_filename));

	// Open the same file again for reading.
	if (d_rgba_file.open(QIODevice::ReadOnly))
	{
		d_rgba_in.setDevice(&d_rgba_file);
		return true;
	}
	else
	{
		return false;
	}
}

