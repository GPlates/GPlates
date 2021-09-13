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
 
#ifndef GPLATES_OPENGL_GLVISUALRASTERSOURCE_H
#define GPLATES_OPENGL_GLVISUALRASTERSOURCE_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
#include <QImage>

#include "GLMultiResolutionRasterSource.h"
#include "GLTexture.h"

#include "global/PointerTraits.h"

#include "gui/Colour.h"
#include "gui/RasterColourPalette.h"

#include "property-values/RawRaster.h"

#include "utils/ReferenceCount.h"


namespace GPlatesPropertyValues
{
	class ProxiedRasterResolver;
}

namespace GPlatesOpenGL
{
	class GL;

	/**
	 * An arbitrary dimension source of fixed-point RGBA8 data made accessible by a proxied raster.
	 *
	 * This raster is meant for visual display by applying a colour palette if the raster source
	 * is floating-point or simply leaving the data in RGBA format if it's a standard colour format
	 * such as JPEG.
	 *
	 * There is also support for modulating the opacity and intensity of the raster for visual purposes.
	 */
	class GLVisualRasterSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLVisualRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLVisualRasterSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLVisualRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLVisualRasterSource> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLVisualRasterSource object.
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
				GL &gl,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const GPlatesGui::Colour &raster_modulate_colour = GPlatesGui::Colour::get_white(),
				unsigned int tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION);


		/**
		 * Change to a new raster of the same dimensions as the current internal raster.
		 *
		 * This method is useful for time-dependent rasters sharing the same georeferencing
		 * and raster dimensions.
		 *
		 * Returns false if @a raster has different dimensions than the current internal raster.
		 * In this case you'll need to create a new @a GLVisualRasterSource.
		 *
		 * NOTE: The opposite, changing the georeferencing without changing the raster,
		 * will require creating a new @a GLMultiResolutionRaster object.
		 */
		bool
		change_raster(
				GL &gl,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette);


		/**
		 * Change the colour to modulate the raster texture with.
		 */
		void
		change_modulate_colour(
				GL &gl,
				const GPlatesGui::Colour &raster_modulate_colour);


		/**
		 * Return the colour to modulate the raster texture with.
		 */
		const GPlatesGui::Colour &
		get_modulate_colour() const;


		unsigned int
		get_raster_width() const override
		{
			return d_raster_width;
		}


		unsigned int
		get_raster_height() const override
		{
			return d_raster_height;
		}


		unsigned int
		get_tile_texel_dimension() const override
		{
			return d_tile_texel_dimension;
		}


		GLint
		get_tile_texture_internal_format() const override
		{
			// Fixed-point 8-bit textures are all that's required for visual rendering.
			return GL_RGBA8;
		}


		bool
		tile_texture_is_visual() const override
		{
			return true;
		}


		bool
		tile_texture_has_coverage() const override
		{
			return false;
		}


		cache_handle_type
		load_tile(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &target_texture,
				GL &gl) override;

	private:

		/**
		 * The proxied raster resolver to get region/level data from raster and
		 * optionally converted to RGBA (using @a d_raster_colour_palette).
		 */
		GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type d_proxied_raster_resolver;

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
		 * The colour used to modulate the raster texture with - the default is white (1,1,1,1).
		 */
		GPlatesGui::Colour d_raster_modulate_colour;

		/**
		 * Uses as temporary space to duplicate a tile's vertical or horizontal edge when the data in
		 * the tile does not consume the full @a d_tile_texel_dimension x @a d_tile_texel_dimension area.
		 */
		boost::scoped_array<GPlatesGui::rgba8_t> d_tile_edge_working_space;

		/**
		 * Images containing error messages when fail to load proxied raster tiles.
		 */
		QImage d_error_text_image_level_zero;
		QImage d_error_text_image_mipmap_levels;

		/**
		 * We log a load-tile-failure warning message only once for each raster source.
		 */
		bool d_logged_tile_load_failure_warning;


		GLVisualRasterSource(
				GL &gl,
				const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &proxy_raster_resolver,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const GPlatesGui::Colour &raster_modulate_colour,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension);

		void
		render_error_text_into_texture(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &target_texture,
				GL &gl);
	};
}

#endif // GPLATES_OPENGL_GLVISUALRASTERSOURCE_H
