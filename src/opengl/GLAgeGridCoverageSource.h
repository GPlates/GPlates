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
 
#ifndef GPLATES_OPENGL_GLAGEGRIDCOVERAGESOURCE_H
#define GPLATES_OPENGL_GLAGEGRIDCOVERAGESOURCE_H

#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
	
#include "GLMultiResolutionRasterSource.h"

#include "gui/Colour.h"
#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRaster.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * An arbitrary dimension source of RGBA data extracted from the coverage of an age grid
	 * raster into an RGBA image that contains white colour and the coverage in the alpha channel.
	 *
	 * The age grid raster itself is input via a proxied raster.
	 */
	class GLAgeGridCoverageSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLAgeGridCoverageSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLAgeGridCoverageSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLAgeGridCoverageSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLAgeGridCoverageSource> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLAgeGridCoverageSource object.
		 *
		 * @a tile_texel_dimension must be a power-of-two - it is the OpenGL square texture
		 * dimension to use for the tiled textures that represent the multi-resolution raster.
		 *
		 * If @a tile_texel_dimension is greater than the maximum texture size supported
		 * by the run-time system then it will be reduced to the maximum texture size.
		 *
		 * Returns false if @a raster is not a proxy raster or if it's uninitialised.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_raster,
				unsigned int tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION);


		virtual
		unsigned int
		get_raster_width() const
		{
			return d_raster_width;
		}


		virtual
		unsigned int
		get_raster_height() const
		{
			return d_raster_height;
		}


		virtual
		unsigned int
		get_tile_texel_dimension() const
		{
			return d_tile_texel_dimension;
		}


		virtual
		void
		load_tile(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &target_texture,
				GLRenderer &renderer);

	private:
		/**
		 * The proxied raster resolver to get region/level float-point data from the age grid raster.
		 */
		GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type d_proxied_raster_resolver;

		//! Original raster width.
		unsigned int d_raster_width;

		//! Original raster height.
		unsigned int d_raster_height;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * Uses as temporary space to convert float coverage values to alpha before loading texture.
		 */
		boost::scoped_array<GPlatesGui::rgba8_t> d_age_grid_coverage_tile_working_space;


		GLAgeGridCoverageSource(
				const GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type &proxy_raster_resolver,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension);
	};
}

#endif // GPLATES_OPENGL_GLAGEGRIDCOVERAGESOURCE_H
