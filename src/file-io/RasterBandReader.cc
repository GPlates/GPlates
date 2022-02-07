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

#include "RasterBandReader.h"

#include "RasterReader.h"
#include "ReadErrorAccumulation.h"

#include "property-values/RawRaster.h"


GPlatesFileIO::RasterBandReader::RasterBandReader(
		const RasterReader::non_null_ptr_type &raster_reader,
		unsigned int band_number) :
	d_raster_reader(raster_reader),
	d_band_number(band_number)
{  }


GPlatesFileIO::RasterBandReader::RasterBandReader(
		const RasterBandReader &other) :
	d_raster_reader(other.d_raster_reader),
	d_band_number(other.d_band_number)
{  }


GPlatesFileIO::RasterBandReader::~RasterBandReader()
{  }


const QString &
GPlatesFileIO::RasterBandReader::get_filename() const
{
	return d_raster_reader->get_filename();
}


unsigned int
GPlatesFileIO::RasterBandReader::get_band_number() const
{
	return d_band_number;
}


bool
GPlatesFileIO::RasterBandReader::can_read()
{
	return d_raster_reader->can_read() &&
		1 <= d_band_number && d_band_number <= d_raster_reader->get_number_of_bands();
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RasterBandReader::get_proxied_raw_raster(
		ReadErrorAccumulation *read_errors)
{
	return d_raster_reader->get_proxied_raw_raster(d_band_number, read_errors);
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RasterBandReader::get_raw_raster(
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	return d_raster_reader->get_raw_raster(d_band_number, region, read_errors);
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::RasterBandReader::get_type(
		ReadErrorAccumulation *read_errors)
{
	return d_raster_reader->get_type(d_band_number, read_errors);
}
