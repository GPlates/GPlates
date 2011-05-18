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
 
#ifndef GPLATES_OPENGL_GLTEXTURERESOURCE_H
#define GPLATES_OPENGL_GLTEXTURERESOURCE_H

#include <opengl/OpenGL.h>

#include "GLResource.h"
#include "GLResourceManager.h"


namespace GPlatesOpenGL
{
	/**
	 * Policy class to allocate and deallocate OpenGL texture objects.
	 */
	class GLTextureObjectAllocator
	{
	public:
		GLint
		allocate()
		{
			GLuint texture;
			glGenTextures(1, &texture);
			return texture;
		}

		void
		deallocate(
				GLuint texture)
		{
			glDeleteTextures(1, &texture);
		}
	};


	/**
	 * Typedef for a texture object resource.
	 */
	typedef GLResource<GLuint, GLTextureObjectAllocator> GLTextureResource;


	/**
	 * Typedef for a texture object resource manager.
	 */
	typedef GLResourceManager<GLuint, GLTextureObjectAllocator> GLTextureResourceManager;
}

#endif // GPLATES_OPENGL_GLTEXTURERESOURCE_H
