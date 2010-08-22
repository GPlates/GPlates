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
 
#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERSOURCE_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERSOURCE_H

#include "GLTexture.h"
#include "GLTextureUtils.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * Interface for an arbitrary dimension source of RGBA data that's used as input
	 * to a @a GLMultiResolutionRaster.
	 */
	class GLMultiResolutionRasterSource :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionRasterSource>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionRasterSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionRasterSource> non_null_ptr_to_const_type;


		/**
		 * The default tile dimension is 256.
		 *
		 * This size gives us a small enough tile region on the globe to make good use
		 * of view frustum culling of tiles.
		 */
		static const std::size_t DEFAULT_TILE_TEXEL_DIMENSION = 256;


		virtual
		~GLMultiResolutionRasterSource()
		{  }


		/**
		 * Returns a token that clients can store with each tile they cache and compare
		 * against the current token to determine if they should reload that tile.
		 */
		GLTextureUtils::ValidToken
		get_current_valid_token() const
		{
			return d_current_valid_token;
		}


		virtual
		unsigned int
		get_raster_width() const = 0;


		virtual
		unsigned int
		get_raster_height() const = 0;


		/**
		 * The requests to @a load_tile *must* have texel offsets that are integer
		 * multiples of this tile dimension. This enables derived classes to
		 * use textures of the tile dimension and satify load requests using these textures.
		 *
		 * NOTE: Since textures can be used for tiles the tile dimension should be a power-of-two.
		 */
		virtual
		unsigned int
		get_tile_texel_dimension() const = 0;


		/**
		 * Loads RGBA8 data into @a target_texture using the specified tile offsets
		 * and level.
		 *
		 * The caller must ensure that @a target_texture has been created in OpenGL -
		 * in other words, not only allocated by also created using glTexImage2D
		 * (you can pass NULL to glTexImage2D to create without loading image data).
		 *
		 * @a renderer is provided in case the data needs to be rendered into the texture.
		 *
		 * @a texel_x_offset and @a texel_y_offset are guaranteed to be a multiple of
		 * the tile texel dimension.
		 * NOTE: This is important as it allows derived classes to maintain their own tiles
		 * without having to deal with load requests crossing tile boundaries.
		 *
		 * @a texel_width and @a texel_height are guaranteed to be less than or equal to
		 * the tile texel dimension - the only cases where they will be less (rather than equal)
		 * is for tiles at the highest tile offsets (near the raster bottom-right boundary - well
		 * for top-to-bottom images).
		 *
		 * When @a texel_width and @a texel_height are not both equal to the tile dimension then
		 * the region of @a target_texture loaded is the lower-left region, in other words
		 * at texture coordinate (0,0).
		 *
		 * Loads a region of the source raster at level of detail @a level where
		 * the region is bounded horizontally by
		 *   [texel_x_offset, texel_x_offset + texel_width)
		 * and vertically by
		 *   [texel_y_offset, texel_y_offset + texel_height)
		 *
		 * The dimension of the Rgba8 raster region loaded is 'texel_height' * 'texel_height'.
		 *
		 * @a level of zero is the original source raster (the highest resolution level-of-detail).
		 *
		 * For example, the texels needed by a 5x5 raster image are:
		 * Level 0: 5x5
		 * Level 1: 3x3 (covers equivalent of 6x6 level 0 texels)
		 * Level 2: 2x2 (covers equivalent of 4x4 level 1 texels or 8x8 level 0 texels)
		 * Level 3: 1x1 (covers same area as level 2)
		 *
		 * Whereas the *same* area on the globe must be covered by all levels of detail so
		 * the area covered on the globe in units of texels (at that level-of-detail) is:
		 * Level 0: 5x5
		 * Level 1: 2.5 x 2.5
		 * Level 2: 1.25 * 1.25
		 * Level 3: 0.625 * 0.625
		 */
		virtual
		void
		load_tile(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &target_texture,
				GLRenderer &renderer) = 0;

	protected:
		GLMultiResolutionRasterSource()
		{  }

		/**
		 * Used by derived classes to signal that the entire source data has changed -
		 * such as a new raster or a new colour scheme or a change in reconstruction time
		 * resulting in new age grid mask data.
		 */
		void
		invalidate()
		{
			d_current_valid_token.invalidate();
		}

	private:
		GLTextureUtils::ValidToken d_current_valid_token;
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERSOURCE_H
