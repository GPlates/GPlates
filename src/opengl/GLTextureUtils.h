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
 
#ifndef GPLATES_OPENGL_GLTEXTUREUTILS_H
#define GPLATES_OPENGL_GLTEXTUREUTILS_H

#include <boost/cstdint.hpp>

#include "GLTexture.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	namespace GLTextureUtils
	{
		/**
		 * Loads the specified region of the RGBA texture with a single colour.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * NOTE: This will bind @a texture to whatever the currently active texture unit is.
		 */
		void
		load_colour_into_texture(
				const GLTexture::shared_ptr_type &texture,
				const GPlatesGui::rgba8_t &colour,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified RGBA8 image into the specified RGBA texture.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 * It is also the caller's responsibility to ensure that @a image points
		 * to @a image_width by @a image_height colour values.
		 *
		 * NOTE: This will bind @a texture to whatever the currently active texture unit is.
		 */
		void
		load_rgba8_image_into_texture(
				const GLTexture::shared_ptr_type &texture,
				const GPlatesGui::rgba8_t *image,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Used to determine if some data is still valid.
		 *
		 * This is currently used for texture tiles where a @a ValidToken is stored with
		 * each cached texture and used in subsequent renders to determine if the cached
		 * tile is still valid.
		 */
		class ValidToken
		{
		public:
			ValidToken() :
				d_invalidate_counter(0)
			{  }

			/**
			 * Returns true if 'this' token matches that of @a current_token.
			 *
			 * Typically 'this' token is cached by the client and subsequently queried
			 * against the current token to determine if their cache should be invalidated.
			 */
			bool
			is_still_valid(
					const ValidToken &current_token) const
			{
				return d_invalidate_counter == current_token.d_invalidate_counter;
			}

			/**
			 * Invalidates this token such it will no longer return true in @a is_still_valid
			 * when compared with a token that @a is_still_valid previously returned true for.
			 */
			void
			invalidate()
			{
				d_invalidate_counter++;
			}

		private:
			boost::intmax_t d_invalidate_counter;
		};
	}
}

#endif // GPLATES_OPENGL_GLTEXTUREUTILS_H
