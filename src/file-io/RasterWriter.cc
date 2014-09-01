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

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>

#include "RasterWriter.h"

#include "GdalRasterWriter.h"
#include "RgbaRasterWriter.h"

#include "global/GPlatesAssert.h"

#include "opengl/OpenGLBadAllocException.h"
#include "opengl/OpenGLException.h"


using namespace GPlatesFileIO;

namespace
{
	void
	add_supported_formats(
			RasterWriter::supported_formats_type &supported_formats,
			RasterWriter::FormatHandler format_handler)
	{
		// Add formats that we support via the RGBA writer.
		RgbaRasterWriter::get_supported_formats(supported_formats);

		// Add formats that we support via the GDAL writer.
		GDALRasterWriter::get_supported_formats(supported_formats);
	}


	const RasterWriter::supported_formats_type &
	build_supported_formats_map(
			RasterWriter::FormatHandler format_handler)
	{
		static RasterWriter::supported_formats_type supported_formats;

		add_supported_formats(supported_formats, format_handler);

		return supported_formats;
	}


	const RasterWriter::supported_formats_type &
	build_supported_formats_map()
	{
		static RasterWriter::supported_formats_type supported_formats;

		for (unsigned int format_handler = 0;
			format_handler < RasterWriter::NUM_FORMAT_HANDLERS;
			++format_handler)
		{
			add_supported_formats(
					supported_formats,
					static_cast<RasterWriter::FormatHandler>(format_handler));
		}

		return supported_formats;
	}
}  // anonymous namespace


const GPlatesFileIO::RasterWriter::supported_formats_type &
GPlatesFileIO::RasterWriter::get_supported_formats()
{
	static const supported_formats_type &supported_formats = build_supported_formats_map();

	return supported_formats;
}


const GPlatesFileIO::RasterWriter::supported_formats_type &
GPlatesFileIO::RasterWriter::get_supported_formats(
		FormatHandler format_handler)
{
	static const supported_formats_type &supported_formats = build_supported_formats_map(format_handler);

	return supported_formats;
}


boost::optional<const GPlatesFileIO::RasterWriter::FormatInfo &>
GPlatesFileIO::RasterWriter::get_format(
		const QString &filename)
{
	QFileInfo file_info(filename);
	QString suffix = file_info.suffix().toLower();

	const supported_formats_type &supported_formats = get_supported_formats();
	supported_formats_type::const_iterator iter = supported_formats.find(suffix);

	// If a supported format was not found then return early.
	if (iter == supported_formats.end())
	{
		return boost::none;
	}

	return iter->second;
}


GPlatesFileIO::RasterWriter::non_null_ptr_type
GPlatesFileIO::RasterWriter::create(
		const QString &filename,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int num_raster_bands,
		GPlatesPropertyValues::RasterType::Type raster_band_type)
{
	return RasterWriter::non_null_ptr_type(
			new RasterWriter(filename, raster_width, raster_height, num_raster_bands, raster_band_type));
}


GPlatesFileIO::RasterWriter::RasterWriter(
		const QString &filename,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int num_raster_bands,
		GPlatesPropertyValues::RasterType::Type raster_band_type) :
	d_impl(NULL),
	d_filename(filename),
	d_width(raster_width),
	d_height(raster_height),
	d_num_bands(num_raster_bands),
	d_band_type(raster_band_type)
{
	boost::optional<const FormatInfo &> format_info = get_format(filename);

	// If a supported format was not found then return early without creating an impl.
	if (!format_info)
	{
		qWarning() << "Unable to find a raster format handler for writing '" << filename
			<< "': file will not get written.";
		return;
	}

	// If the supported format does not support the band type then return early without creating an impl.
	const std::vector<GPlatesPropertyValues::RasterType::Type> &supported_band_types = format_info->band_types;
	if (std::find(supported_band_types.begin(), supported_band_types.end(), d_band_type) ==
		supported_band_types.end())
	{
		qWarning() << "Raster band type '"
			<< GPlatesPropertyValues::RasterType::get_type_as_string(d_band_type)
			<< "' is not supported for writing to '" << filename << "': file will not get written.";
		return;
	}

	const FormatHandler handler = format_info->handler;
	switch (handler)
	{
		case RGBA:
			d_impl.reset(
					new RgbaRasterWriter(
							filename,
							format_info.get(),
							d_width,
							d_height,
							d_num_bands,
							d_band_type));
			break;
		
		case GDAL:
			d_impl.reset(
					new GDALRasterWriter(
							filename,
							format_info.get(),
							d_width,
							d_height,
							d_num_bands,
							d_band_type));
			break;

		default:
			// Shouldn't be able to get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}


bool
GPlatesFileIO::RasterWriter::can_write() const
{
	if (d_impl)
	{
		return d_impl->can_write();
	}
	else
	{
		return false;
	}
}


const QString &
GPlatesFileIO::RasterWriter::get_filename() const
{
	return d_filename;
}


std::pair<unsigned int, unsigned int>
GPlatesFileIO::RasterWriter::get_size()
{
	return std::make_pair(d_width, d_height);
}


unsigned int
GPlatesFileIO::RasterWriter::get_number_of_bands()
{
	return d_num_bands;
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::RasterWriter::get_raster_band_type()
{
	return d_band_type;
}


void
GPlatesFileIO::RasterWriter::set_georeferencing(
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing)
{
	if (d_impl)
	{
		d_impl->set_georeferencing(georeferencing);
	}
}


void
GPlatesFileIO::RasterWriter::set_spatial_reference_system(
		const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type& srs)
{
	if (d_impl)
	{
		d_impl->set_spatial_reference_system(srs);
	}
}


bool
GPlatesFileIO::RasterWriter::write_region_data(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raw_raster,
		unsigned int band_number,
		unsigned int x_offset,
		unsigned int y_offset)
{
	if (d_impl)
	{
		return d_impl->write_region_data(raw_raster, band_number, x_offset, y_offset);
	}
	else
	{
		return false;
	}
}


bool
GPlatesFileIO::RasterWriter::write_file()
{
	if (d_impl)
	{
		return d_impl->write_file();
	}
	else
	{
		return false;
	}
}


GPlatesFileIO::RasterWriterImpl::~RasterWriterImpl()
{  }
