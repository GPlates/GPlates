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
 
#ifndef GPLATES_OPENGL_GLRASTERPROXY_H
#define GPLATES_OPENGL_GLRASTERPROXY_H

#include "property-values/RawRaster.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A temporary raster proxy class that works only with RGBA8 rasters and brute-force
	 * generates all levels of detail for the entire raster when it is constructed.
	 *
	 * Enoch will be implementing a general purpose raster proxy class that also handles
	 * floating-point rasters and interfaces with raster loading to load raster regions on demand
	 * rather than loading the entire raster into memory (and consuming valuable time and resources).
	 */
	class GLRasterProxy :
			public GPlatesUtils::ReferenceCount<GLRasterProxy>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRasterProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRasterProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRasterProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRasterProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRasterProxy object.
		 *
		 * Loads entire raster into memory and generates mipmap rasters for the
		 * levels of detail.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster)
		{
			return non_null_ptr_type(new GLRasterProxy(raster));
		}


		/*
		 * The number of texels that we need to cover the entire raster at any
		 * level-of-detail has some interesting behaviour.
		 *
		 * The number of texels at level 'n+1' is a function of the number of texels
		 * at level 'n' according to the following (where variables are integers).
		 *   lod_texel_width = (lod_texel_width + 1) / 2;
		 *   lod_texel_height = (lod_texel_height + 1) / 2;
		 *
		 * For example, the texels needed by a 5x5 raster image are:
		 * Level 0: 5x5
		 * Level 1: 3x3 (covers equivalent of 6x6 level 0 texels)
		 * Level 2: 2x2 (covers equivalent of 4x4 level 1 texels or 8x8 level 0 texels)
		 * Level 3: 1x1 (covers same area as level 2)
		 *
		 * Whereas the same area on the globe must be covered by all levels of detail so
		 * the area covered on the globe in units of texels (at that level-of-detail) is:
		 * Level 0: 5x5
		 * Level 1: 2.5 x 2.5
		 * Level 2: 1.25 * 1.25
		 * Level 3: 0.625 * 0.625
		 *
		 * ...so for a level 1 level-of-detail the number of texels needed is 3x3 whereas
		 * only 2.5 x 2.5 texels actually contribute to the raster on the globe. However
		 * we need 3x3 texels because you can't have partial texels.
		 */


		/**
		 * Returns the width in texels of the raster at level-of-detail @a level_of_detail.
		 *
		 * @a level_of_detail of zero is the original raster (the highest resolution level-of-detail).
		 */
		unsigned int
		get_texel_width(
				unsigned int level_of_detail) const;

		/**
		 * Returns the height in texels of the raster at level-of-detail @a level_of_detail.
		 *
		 * @a level_of_detail of zero is the original raster (the highest resolution level-of-detail).
		 */
		unsigned int
		get_texel_height(
				unsigned int level_of_detail) const;


		/**
		 * Returns a region of the raster at level of detail @a level_of_detail where
		 * the region is bounded horizontally by
		 *   [x_texel_offset, x_texel_offset + num_x_texels)
		 * and vertically by
		 *   [y_texel_offset, y_texel_offset + num_y_texels)
		 *
		 * The dimensions of the returned Rgba8 raster are 'num_x_texels' * 'num_y_texels'.
		 *
		 * @a level_of_detail of zero is the original raster (the highest resolution level-of-detail).
		 *
		 * For example, the texels needed by a 5x5 raster image are:
		 * Level 0: 5x5
		 * Level 1: 3x3 (covers equivalent of 6x6 level 0 texels)
		 * Level 2: 2x2 (covers equivalent of 4x4 level 1 texels or 8x8 level 0 texels)
		 * Level 3: 1x1 (covers same area as level 2)
		 *
		 * Whereas the same area on the globe must be covered by all levels of detail so
		 * the area covered on the globe in units of texels (at that level-of-detail) is:
		 * Level 0: 5x5
		 * Level 1: 2.5 x 2.5
		 * Level 2: 1.25 * 1.25
		 * Level 3: 0.625 * 0.625
		 */
		GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
		get_raster_region(
				unsigned int level_of_detail,
				unsigned int x_texel_offset,
				unsigned int num_x_texels,
				unsigned int y_texel_offset,
				unsigned int num_y_texels) const;

	private:

		explicit
		GLRasterProxy(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster);
	};
}


#endif // GPLATES_OPENGL_GLRASTERPROXY_H
