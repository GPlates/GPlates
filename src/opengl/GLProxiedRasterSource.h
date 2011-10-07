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

#include <vector>
#include <boost/optional.hpp>
#include <QImage>

#include "GLCompiledDrawState.h"
#include "GLMultiResolutionRasterSource.h"
#include "GLTexture.h"

#include "global/PointerTraits.h"

#include "gui/Colour.h"
#include "gui/RasterColourPalette.h"

#include "property-values/RawRaster.h"

#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesPropertyValues
{
	class ProxiedRasterResolver;
}

namespace GPlatesOpenGL
{
	class GLRenderer;

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
				GLRenderer &renderer,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const GPlatesGui::Colour &raster_modulate_colour = GPlatesGui::Colour::get_white(),
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
		cache_handle_type
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
				GLRenderer &renderer,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette);


		/**
		 * Change the colour to modulate the raster texture with.
		 */
		void
		change_modulate_colour(
				GLRenderer &renderer,
				const GPlatesGui::Colour &raster_modulate_colour);

	private:
		class Tile
		{
		public:
			GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type &
			get_raster_texture(
					GPlatesUtils::ObjectCache<GLTexture> &raster_texture_cache)
			{
				if (!raster_texture)
				{
					raster_texture = raster_texture_cache.allocate_volatile_object();
				}
				return raster_texture;
			}

			/**
			 * Used to detect if @a raster_texture is out-of-date due to changed raster data such
			 * as a time-dependent raster or new raster colour palette.
			 */
			GPlatesUtils::ObserverToken raster_data_observer_token;

		private:
			//! Volatile reference to raster texture tile loaded from proxied raster resolver.
			GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type raster_texture;
		};

		typedef std::vector<Tile> tile_seq_type;


		class LevelOfDetail :
				public GPlatesUtils::ReferenceCount<LevelOfDetail>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<LevelOfDetail> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const LevelOfDetail> non_null_ptr_to_const_type;

			static
			non_null_ptr_type
			create(
					unsigned int num_x_tiles_,
					unsigned int num_y_tiles_)
			{
				return non_null_ptr_type(new LevelOfDetail(num_x_tiles_, num_y_tiles_));
			}

			Tile &
			get_tile(
					unsigned int tile_x_offset,
					unsigned int tile_y_offset)
			{
				return tiles[tile_y_offset * num_x_tiles + tile_x_offset];
			}

			unsigned int num_x_tiles;
			unsigned int num_y_tiles;

			//! A 2D array of tiles indexed by the tile offset in this level of detail.
			tile_seq_type tiles;

		private:
			LevelOfDetail(
					unsigned int num_x_tiles_,
					unsigned int num_y_tiles_) :
				num_x_tiles(num_x_tiles_),
				num_y_tiles(num_y_tiles_)
			{
				tiles.resize(num_x_tiles * num_y_tiles);
			}
		};

		typedef std::vector<LevelOfDetail::non_null_ptr_type> level_of_detail_seq_type;


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
		 * Used for allocating temporary textures when modulating a raster tile with a colour.
		 */
		GPlatesUtils::ObjectCache<GLTexture>::shared_ptr_type d_raster_texture_cache;

		/**
		 * Keeps track of changes to the raster data itself (the data sourced from the proxied raster resolver).
		 *
		 * Changes include a new raster (eg, time-dependent raster) and/or a new raster colour palette.
		 * NOTE: Does *not* include changes to the modulate colour as this affects the raster data
		 * *after* it's loaded from the proxied raster resolver.
		 */
		GPlatesUtils::SubjectToken d_raster_data_subject_token;

		/**
		 * The cached source textures across the different levels of detail.
		 */
		level_of_detail_seq_type d_levels;

		/**
		 * The colour used to modulate the raster texture with - the default is white (1,1,1,1).
		 */
		GPlatesGui::Colour d_raster_modulate_colour;

		/**
		 * Used to draw a coloured full-screen quad into render texture (for colour modulation of raster).
		 *
		 * The vertex colours are @a d_raster_modulate_colour.
		 */
		GLCompiledDrawState::non_null_ptr_to_const_type d_full_screen_quad_drawable;

		/**
		 * Images containing error messages when fail to load proxied raster tiles.
		 */
		QImage d_error_text_image_level_zero;
		QImage d_error_text_image_mipmap_levels;


		GLProxiedRasterSource(
				GLRenderer &renderer,
				const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
						proxy_raster_resolver,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const GPlatesGui::Colour &raster_modulate_colour,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension);

		void
		initialise_level_of_detail_pyramid();

		Tile &
		get_tile(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset);

		void
		load_proxied_raster_data_into_raster_texture(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &raster_texture,
				Tile &tile,
				GLRenderer &renderer);

		void
		render_error_text_into_texture(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &texture,
				GLRenderer &renderer);

		void
		write_raster_texture_into_tile_target_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &target_texture,
				const GLTexture::shared_ptr_type &raster_texture);

		void
		create_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture);
	};
}

#endif // GPLATES_OPENGL_GLPROXIEDRASTERSOURCE_H
