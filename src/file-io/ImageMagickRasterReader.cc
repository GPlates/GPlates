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

#include "ImageMagickRasterReader.h"

#include <QDebug>


GPlatesFileIO::ImageMagickRasterReader::ImageMagickRasterReader(
		const QString &filename,
		const boost::function<RasterBandReaderHandle (unsigned int)> &proxy_handle_function,
		ReadErrorAccumulation *read_errors) :
	d_filename(filename),
	d_filename_as_std_string(filename.toStdString()),
	d_proxy_handle_function(proxy_handle_function),
	d_can_read(false)
{
	try
	{
		Magick::Image image;
		image.ping(d_filename_as_std_string); // ping() loads size but not the actual data.
		Magick::Geometry size = image.size();
		d_can_read = size.width() > 0 && size.height() > 0;
	}
	catch (const Magick::Exception &)
	{
		// Do nothing.
	}

	// Do all the failure to begin reporting here, so that there's only one
	// such report per file.
	if (!d_can_read)
	{
		report_failure_to_begin(read_errors, ReadErrors::ErrorReadingRasterFile);
	}
}


bool
GPlatesFileIO::ImageMagickRasterReader::can_read()
{
	return d_can_read;
}


unsigned int
GPlatesFileIO::ImageMagickRasterReader::get_number_of_bands(
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


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::ImageMagickRasterReader::get_proxied_raw_raster(
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

	// Because ProxiedRgba8RawRaster doesn't store statistics or a no-data value,
	// the only thing we need to get from the file for now is the size.
	try
	{
		Magick::Image image;
		image.ping(d_filename_as_std_string); // ping() loads size but not the actual data.
		Magick::Geometry size = image.size();
		if (size.width() > 0 && size.height() > 0)
		{
			GPlatesPropertyValues::ProxiedRgba8RawRaster::non_null_ptr_type result =
				GPlatesPropertyValues::ProxiedRgba8RawRaster::create(
						size.width(), size.height(), d_proxy_handle_function(band_number));
			return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
		}
		else
		{
			return boost::none;
		}
	}
	catch (const Magick::Geometry &)
	{
		return boost::none;
	}
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::ImageMagickRasterReader::get_raw_raster(
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

	boost::optional<std::pair<Magick::Image, QSize> > image_info = get_region_as_image(region, read_errors);
	if (!image_info)
	{
		return boost::none;
	}

	Magick::Image &image = image_info->first;
	QSize &size = image_info->second;
	GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type result =
		GPlatesPropertyValues::Rgba8RawRaster::create(size.width(), size.height());
	image.write(0, 0, size.width(), size.height(), "RGBA", Magick::CharPixel, result->data());

	return GPlatesPropertyValues::RawRaster::non_null_ptr_type(result.get());
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::ImageMagickRasterReader::get_type(
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
GPlatesFileIO::ImageMagickRasterReader::get_data(
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

	boost::optional<std::pair<Magick::Image, QSize> > image_info = get_region_as_image(region, read_errors);
	if (!image_info)
	{
		return NULL;
	}

	Magick::Image &image = image_info->first;
	QSize &size = image_info->second;
	GPlatesGui::rgba8_t *result_buf = new GPlatesGui::rgba8_t[size.width() * size.height()];
	image.write(0, 0, size.width(), size.height(), "RGBA", Magick::CharPixel, result_buf);

	return result_buf;
}


boost::optional<std::pair<Magick::Image, QSize> >
GPlatesFileIO::ImageMagickRasterReader::get_region_as_image(
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	try
	{
		if (region.isValid())
		{
			Magick::Image full_image;
			full_image.ping(d_filename_as_std_string); // ping() loads size but not the actual data.
			Magick::Geometry full_image_size = full_image.size();

			if (full_image_size.width() > 0 && full_image_size.height() > 0 &&
				region.x() >= 0 && region.y() >= 0 &&
				region.width() > 0 && region.height() > 0 &&
				static_cast<size_t>(region.x() + region.width()) <= full_image_size.width() &&
				static_cast<size_t>(region.y() + region.height()) <= full_image_size.height())
			{
				Magick::Image clipped_image;
				Magick::Geometry region_as_geometry(
						region.width(), region.height(), region.x(), region.y());
				clipped_image.read(region_as_geometry, d_filename_as_std_string);
				return std::make_pair(
						clipped_image,
						QSize(region.width(), region.height()));
			}
		}
		else
		{
			Magick::Image full_image;
			full_image.read(d_filename_as_std_string);
			Magick::Geometry full_image_size = full_image.size();

			if (full_image_size.width() > 0 && full_image_size.height() > 0)
			{
				return std::make_pair(
						full_image,
						QSize(full_image_size.width(), full_image_size.height()));
			}
		}
	}
	catch (const Magick::Exception &)
	{
		// Fall through...
	}

	report_recoverable_error(read_errors, ReadErrors::ErrorReadingRasterBand);
	return boost::none;
}


void
GPlatesFileIO::ImageMagickRasterReader::report_recoverable_error(
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
GPlatesFileIO::ImageMagickRasterReader::report_failure_to_begin(
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

