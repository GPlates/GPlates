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
 
#ifndef GPLATES_OPENGL_GLPROXIEDRASTERSOURCE_H
#define GPLATES_OPENGL_GLPROXIEDRASTERSOURCE_H

#include <boost/optional.hpp>
#include <QImage>

#include "GLMultiResolutionRasterSource.h"

#include "global/PointerTraits.h"

#include "gui/RasterColourPalette.h"

#include "property-values/RawRaster.h"


namespace GPlatesPropertyValues
{
	class ProxiedRasterResolver;
}

namespace GPlatesOpenGL
{
	/**
	 * An arbitrary dimension source of RGBA data made accessible by a proxied raster.
	 */
	class GLProxiedRasterSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLProxiedRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLProxiedRasterSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLProxiedRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLProxiedRasterSource> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLProxiedRasterSource object.
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
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
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


		/**
		 * Change to a new raster of the same dimensions as the current internal raster.
		 *
		 * This method is useful for time-dependent rasters sharing the same georeferencing
		 * and raster dimensions.
		 *
		 * Returns false if @a raster has different dimensions than the current internal raster.
		 * In this case you'll need to create a new @a GLProxiedRasterSource.
		 *
		 * NOTE: The opposite, changing the georeferencing without changing the raster,
		 * will require creating a new @a GLMultiResolutionRaster object.
		 */
		bool
		change_raster(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette);

	private:
		/**
		 * The proxied raster resolver to get region/level data from raster and
		 * optionally converted to RGBA (using @a d_raster_colour_palette).
		 */
		GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type
					d_proxied_raster_resolver;

		/**
		 * The colour palette used to convert non-RGBA raster data to RGBA.
		 */
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_raster_colour_palette;

		//! Original raster width.
		unsigned int d_raster_width;

		//! Original raster height.
		unsigned int d_raster_height;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * Images containing error messages when fail to load proxied raster tiles.
		 */
		QImage d_error_text_image_level_zero;
		QImage d_error_text_image_mipmap_levels;


		GLProxiedRasterSource(
				const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
						proxy_raster_resolver,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension);
	};
}

#endif // GPLATES_OPENGL_GLPROXIEDRASTERSOURCE_H
