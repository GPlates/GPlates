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
 
#ifndef GPLATES_OPENGL_GLVERTEXARRAYCACHE_H
#define GPLATES_OPENGL_GLVERTEXARRAYCACHE_H

#include "GLCache.h"
#include "GLVertexArray.h"
#include "GLVolatileObject.h"


namespace GPlatesOpenGL
{
	namespace GLCacheInternals
	{
		/**
		 * Creates @a GLVertexArray objects.
		 */
		struct GLVertexArrayCreator
		{
			GLVertexArray::shared_ptr_type
			create()
			{
				return GLVertexArray::create();
			}
		};
	}


	/**
	 * A volatile vertex array allocated from a cache.
	 */
	typedef GLVolatileObject<GLVertexArray> GLVolatileVertexArray;


	/**
	 * A vertex array cache.
	 *
	 * Allocates objects of type @a GLVolatileVertexArray.
	 */
	typedef GLCache<GLVertexArray, GLCacheInternals::GLVertexArrayCreator> GLVertexArrayCache;


	/**
	 * Convenience function to create a vertex array cache.
	 */
	inline
	GLVertexArrayCache::non_null_ptr_type
	create_vertex_array_cache(
			std::size_t max_num_vertex_arrays)
	{
		return GLVertexArrayCache::create(max_num_vertex_arrays);
	}
}

#endif // GPLATES_OPENGL_GLVERTEXARRAYCACHE_H
