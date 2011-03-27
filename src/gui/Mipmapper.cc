/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "Mipmapper.h"


GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type
GPlatesGui::MipmapperInternals::get_opaque_coverage_raster(
		unsigned int width,
		unsigned int height)
{
	GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type result =
		GPlatesPropertyValues::CoverageRawRaster::create(width, height);
	std::fill(
			result->data(),
			result->data() + width * height,
			1.0f /* all pixels are fully opaque (non-transparent) */);
	return result;
}
