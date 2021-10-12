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
 
#ifndef GPLATES_OPENGL_GLTEXTURECACHE_H
#define GPLATES_OPENGL_GLTEXTURECACHE_H

#include "GLCache.h"
#include "GLResource.h"
#include "GLResourceManager.h"
#include "GLTexture.h"
#include "GLVolatileObject.h"


namespace GPlatesOpenGL
{
	namespace GLCacheInternals
	{
		/**
		 * Creates @a GLTexture objects.
		 */
		class GLTextureCreator
		{
		public:
			explicit
			GLTextureCreator(
					const GLTextureResourceManager::shared_ptr_type &texture_manager) :
				d_texture_manager(texture_manager)
			{  }

			GLTexture::shared_ptr_type
			create()
			{
				return GLTexture::create(d_texture_manager);
			}

		private:
			GLTextureResourceManager::shared_ptr_type d_texture_manager;
		};
	}


	/**
	 * A volatile texture allocated from a texture cache.
	 */
	typedef GLVolatileObject<GLTexture> GLVolatileTexture;


	/**
	 * A texture cache.
	 *
	 * Allocates objects of type @a GLVolatileTexture.
	 */
	typedef GLCache<GLTexture, GLCacheInternals::GLTextureCreator> GLTextureCache;


	/**
	 * Convenience function to create a texture cache.
	 */
	inline
	GLTextureCache::non_null_ptr_type
	create_texture_cache(
			std::size_t max_num_textures,
			const GLTextureResourceManager::shared_ptr_type &texture_manager)
	{
		return GLTextureCache::create(
				max_num_textures,
				GLCacheInternals::GLTextureCreator(texture_manager));
	}
}

#endif // GPLATES_OPENGL_GLTEXTURECACHE_H
