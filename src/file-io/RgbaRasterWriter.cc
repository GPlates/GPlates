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

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <QDebug>
#include <QImageWriter>

#include "RgbaRasterWriter.h"

#include "gui/Colour.h"

#include "property-values/RasterType.h"
#include "property-values/RawRasterUtils.h"


void
GPlatesFileIO::RgbaRasterWriter::get_supported_formats(
		RasterWriter::supported_formats_type &supported_formats)
{
	// RGBA supports only the RGBA raster data type.
	const std::vector<GPlatesPropertyValues::RasterType::Type>
			rgba_band_types(1, GPlatesPropertyValues::RasterType::RGBA8);

	// Note: The descriptions are those used by the GIMP.
	supported_formats.insert(std::make_pair(
			"bmp",
			RasterWriter::FormatInfo(
				"Windows BMP image",
				"image/bmp",
				RasterWriter::RGBA,
				rgba_band_types)));
	supported_formats.insert(std::make_pair(
			"gif",
			RasterWriter::FormatInfo(
				"GIF image",
				"image/gif",
				RasterWriter::RGBA,
				rgba_band_types)));
	supported_formats.insert(std::make_pair(
			"jpg",
			RasterWriter::FormatInfo(
				"JPEG image",
				"image/jpeg",
				RasterWriter::RGBA,
				rgba_band_types)));
	supported_formats.insert(std::make_pair(
			"jpeg",
			RasterWriter::FormatInfo(
				"JPEG image",
				"image/jpeg",
				RasterWriter::RGBA,
				rgba_band_types)));
	supported_formats.insert(std::make_pair(
			"png",
			RasterWriter::FormatInfo(
				"PNG image",
				"image/png",
				RasterWriter::RGBA,
				rgba_band_types)));
	supported_formats.insert(std::make_pair(
			"svg",
			RasterWriter::FormatInfo(
				"SVG image",
				"image/svg+xml",
				RasterWriter::RGBA,
				rgba_band_types)));
}


GPlatesFileIO::RgbaRasterWriter::RgbaRasterWriter(
		const QString &filename,
		const RasterWriter::FormatInfo &format_info,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int num_raster_bands,
		GPlatesPropertyValues::RasterType::Type raster_band_type) :
	d_filename(filename)
{
	// We only support a single colour (RGBA) band.
	if (num_raster_bands != 1 ||
		raster_band_type != GPlatesPropertyValues::RasterType::RGBA8)
	{
		qWarning() << "RGBA rasters (being written) only support a single band.";
		return;
	}

	// Allocate image of uninitialised data for standard Qt format...
	d_image = QImage(raster_width, raster_height, QImage::Format_ARGB32);
	if (d_image.isNull())
	{
		qWarning() << "Unable to allocate memory for writing RGBA raster of dimensions "
			<< raster_width << " x " << raster_height << ".";
	}
}


bool
GPlatesFileIO::RgbaRasterWriter::can_write() const
{
	// A null image is most likely a memory allocation failure if image dimensions were too large.
	return !d_image.isNull();
}


void
GPlatesFileIO::RgbaRasterWriter::set_georeferencing(
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing)
{
	// Do nothing - RGBA raster writing via Qt does not support georeferencing.
}


void
GPlatesFileIO::RgbaRasterWriter::set_spatial_reference_system(
		const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type& srs)
{
	// Do nothing - RGBA raster writing via Qt does not support spatial reference systems.
}


bool
GPlatesFileIO::RgbaRasterWriter::write_region_data(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &region_data,
		unsigned int band_number,
		unsigned int x_offset,
		unsigned int y_offset)
{
	if (!can_write())
	{
		return false;
	}

	// There should only be one band for colour rasters.
	if (band_number != 1)
	{
		qWarning() << "RGBA raster band number (being written) should be one.";
		return false;
	}

	// The raster data must be RGBA data.
	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> rgba8_region_data =
			GPlatesPropertyValues::RawRasterUtils::try_rgba8_raster_cast(*region_data);
	if (!rgba8_region_data)
	{
		qWarning() << "Expecting RGBA region data when writing to RGBA raster.";
		return false;
	}

	const unsigned int region_width = rgba8_region_data.get()->width();
	const unsigned int region_height = rgba8_region_data.get()->height();

	// The raster data region being written must fit within the raster dimensions.
	if (x_offset + region_width > boost::numeric_cast<unsigned int>(d_image.width()) ||
		y_offset + region_height > boost::numeric_cast<unsigned int>(d_image.height()))
	{
		qWarning() << "Region written to RGBA raster is outside raster boundary.";
		return false;
	}

	const GPlatesGui::rgba8_t *const region_pixel_data = rgba8_region_data.get()->data();

	// Iterate over the pixel lines in the raw raster and copy into a sub-rect of the image.
	for (unsigned int j = 0; j < region_height; ++j)
	{
		// Source pixel data.
		const GPlatesGui::rgba8_t *src_pixel_data = region_pixel_data + j * region_width;

		// Destination pixel data.
		const unsigned int y = y_offset + j;
		boost::uint32_t *dst_pixel_data =
				reinterpret_cast<boost::uint32_t *>(d_image.scanLine(y)) + x_offset;

		// Convert the current line to the QImage::Format_ARGB32 format supported by QImage.
		GPlatesGui::convert_rgba8_to_argb32(src_pixel_data, dst_pixel_data, region_width);
	}

	return true;
}


bool
GPlatesFileIO::RgbaRasterWriter::write_file()
{
	if (!can_write())
	{
		return false;
	}

	QImageWriter image_writer(d_filename);
	const bool success = image_writer.write(d_image);
	if (!success)
	{
		qWarning() << "Unable to create RGBA raster file '" << d_filename << "'.";
	}

	// Release the image memory.
	// This also causes subsequent calls to 'can_write()' to return false.
	d_image = QImage();

	return success;
}
