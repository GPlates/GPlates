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
 
#ifndef GPLATES_OPENGL_GLAGEGRIDMASKSOURCE_H
#define GPLATES_OPENGL_GLAGEGRIDMASKSOURCE_H

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>

#include "GLClearBuffers.h"
#include "GLClearBuffersState.h"
#include "GLCompositeStateSet.h"
#include "GLMaskBuffersState.h"
#include "GLMultiResolutionRasterSource.h"
#include "GLResourceManager.h"
#include "GLStateSet.h"
#include "GLTexture.h"
#include "GLTextureCache.h"
#include "GLTextureUtils.h"
#include "GLVertexArray.h"
#include "GLVertexElementArray.h"
#include "GLViewport.h"
#include "GLViewportState.h"

#include "gui/Colour.h"

#include "maths/types.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRaster.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * An arbitrary dimension source of RGBA data made accessible by a proxied raster.
	 */
	class GLAgeGridMaskSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLAgeGridMaskSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLAgeGridMaskSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLAgeGridMaskSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLAgeGridMaskSource> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLAgeGridMaskSource object.
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
				const double &reconstruction_time,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_raster,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
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
		 * Updates the reconstruction time - if it's changed since the last call then
		 * this source will invalidate itself and cause any connected clients to
		 * refresh their texture caches.
		 * This is because a change in reconstruction time means the age grid mask will change.
		 */
		void
		update_reconstruction_time(
				const double &reconstruction_time);

	private:
		class Tile
		{
		public:
			mutable GLVolatileTexture low_byte_age_texture;
			mutable GLVolatileTexture high_byte_age_texture;
			mutable GLVolatileTexture intermediate_mask_texture;
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
		 * The current reconstruction time determines whether to update the age grid mask.
		 */
		GPlatesMaths::real_t d_current_reconstruction_time;

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
		 * Texture cache for age grid mask texture.
		 */
		GLTextureCache::non_null_ptr_type d_age_grid_mask_texture_cache;

		/**
		 * Texture cache for the actual floating-point age values read from a proxied raster.
		 */
		GLTextureCache::non_null_ptr_type d_age_grid_texture_cache;

		/**
		 * Used for render textures to store intermediate results.
		 */
		GLTextureCache::non_null_ptr_type d_intermediate_render_texture_cache;

		/**
		 * The cached textures across the different levels of detail.
		 */
		level_of_detail_seq_type d_levels;

		//
		// Various state used when rendering to age grid mask render texture.
		//
		GLClearBuffersState::non_null_ptr_type d_clear_buffers_state;
		GLClearBuffers::non_null_ptr_type d_clear_buffers;
		GLMaskBuffersState::non_null_ptr_type d_mask_alpha_channel_state;
		GLViewport d_viewport;
		GLViewportState::non_null_ptr_type d_viewport_state;

		// Used to draw a textured full-screen quad into render texture.
		GLVertexArray::shared_ptr_type d_full_screen_quad_vertex_array;
		GLVertexElementArray::non_null_ptr_type d_full_screen_quad_vertex_element_array;

		// The composite state sets used for each of the three render passes required to
		// render an age grid mask.
		GLCompositeStateSet::non_null_ptr_type d_first_render_pass_state;
		GLCompositeStateSet::non_null_ptr_type d_second_render_pass_state;
		GLCompositeStateSet::non_null_ptr_type d_third_render_pass_state;


		// The minimum and maximum age grid values in the raster.
		float d_raster_min_age;
		float d_raster_max_age;
		float d_raster_inv_age_range_factor;

		// The current reconstruction time translated/scaled to a 16-bit unsigned integer
		// where 0 is min age and 2^16 - 1 is max age.
		boost::uint8_t d_current_reconstruction_time_high_byte;
		boost::uint8_t d_current_reconstruction_time_low_byte;

		boost::scoped_array<GPlatesGui::rgba8_t> d_age_high_byte_tile_working_space;
		boost::scoped_array<GPlatesGui::rgba8_t> d_age_low_byte_tile_working_space;


		GLAgeGridMaskSource(
				const double &reconstruction_time,
				const GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type &proxy_raster_resolver,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension,
				double min_age_in_raster,
				double max_age_in_raster,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager);

		void
		initialise_level_of_detail_pyramid();

		Tile &
		get_tile(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset);

		bool
		should_reload_high_and_low_byte_age_textures(
				Tile &tile,
				GLTexture::shared_ptr_type &high_byte_age_texture,
				GLTexture::shared_ptr_type &low_byte_age_texture);

		bool
		load_age_grid_into_high_and_low_byte_tile(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_tile,
				GLTexture::shared_ptr_type &high_byte_age_texture,
				GLTexture::shared_ptr_type &low_byte_age_texture,
				unsigned int texel_width,
				unsigned int texel_height);

		template <typename RealType>
		void
		load_age_grid_into_high_and_low_byte_tile(
				const RealType *age_grid_tile,
				GLTexture::shared_ptr_type &high_byte_age_texture,
				GLTexture::shared_ptr_type &low_byte_age_texture,
				unsigned int texel_width,
				unsigned int texel_height);

		void
		render_age_grid_mask(
				const GLTexture::shared_ptr_type &target_texture,
				const GLTexture::shared_ptr_type &high_byte_age_texture,
				const GLTexture::shared_ptr_type &low_byte_age_texture,
				GLRenderer &renderer);

		void
		render_age_grid_intermediate_mask(
				const GLTexture::shared_ptr_type &intermediate_texture,
				const GLTexture::shared_ptr_type &high_byte_age_texture,
				const GLTexture::shared_ptr_type &low_byte_age_texture,
				GLRenderer &renderer);

		void
		create_tile_texture(
				const GLTexture::shared_ptr_type &texture);

		/**
		 * Converts a floating-point age to a 16-bit unsigned integer.
		 */
		inline
		void
		convert_age_to_16_bit_integer(
				float age,
				boost::uint8_t &age_high_byte,
				boost::uint8_t &age_low_byte)
		{
			int age_int = static_cast<int>(
					d_raster_inv_age_range_factor * (age - d_raster_min_age));

			// Probably no need to clamp but we'll do it anyway in case the raster
			// provides an age value that is slightly outside the min/max range it provided.
			if (age_int < 0)
			{
				age_int = 0;
			}
			else if (age_int > 0xffff)
			{
				age_int = 0xffff;
			}

			age_high_byte = (age_int >> 8);
			age_low_byte = (age_int & 0xff);
		}
	};
}

#endif // GPLATES_OPENGL_GLAGEGRIDMASKSOURCE_H
