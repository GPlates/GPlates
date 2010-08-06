/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
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

#include "GLRasterProxy.h"

#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "property-values/RawRasterUtils.h"


GPlatesOpenGL::GLRasterProxy::GLRasterProxy(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster)
{
	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*raster);

	// If raster happens to be uninitialised then just return early.
	if (!raster_dimensions)
	{
		return;
	}

	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
}


unsigned int
GPlatesOpenGL::GLRasterProxy::get_texel_width(
		unsigned int level_of_detail) const
{
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
}


unsigned int
GPlatesOpenGL::GLRasterProxy::get_texel_height(
		unsigned int level_of_detail) const
{
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
}


GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
GPlatesOpenGL::GLRasterProxy::get_raster_region(
		unsigned int level_of_detail,
		unsigned int x_texel_offset,
		unsigned int num_x_texels,
		unsigned int y_texel_offset,
		unsigned int num_y_texels) const
{
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
}
