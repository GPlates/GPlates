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

#include "RasterBandReaderHandle.h"

#include "property-values/RawRaster.h"


GPlatesFileIO::RasterBandReaderHandle::RasterBandReaderHandle(
		const RasterBandReader &raster_band_reader) :
	d_raster_band_reader(raster_band_reader)
{  }


const QString &
GPlatesFileIO::RasterBandReaderHandle::get_filename() const
{
	return d_raster_band_reader.get_filename();
}


unsigned int
GPlatesFileIO::RasterBandReaderHandle::get_band_number() const
{
	return d_raster_band_reader.get_band_number();
}


bool
GPlatesFileIO::RasterBandReaderHandle::can_read()
{
	return d_raster_band_reader.can_read();
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RasterBandReaderHandle::get_raw_raster(
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	return d_raster_band_reader.get_raw_raster(region, read_errors);
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::RasterBandReaderHandle::get_type(
		ReadErrorAccumulation *read_errors)
{
	return d_raster_band_reader.get_type(read_errors);
}
