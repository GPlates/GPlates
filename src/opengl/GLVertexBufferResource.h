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
 
#ifndef GPLATES_OPENGL_GLVERTEXBUFFERRESOURCE_H
#define GPLATES_OPENGL_GLVERTEXBUFFERRESOURCE_H

#include <opengl/OpenGL.h>

#include "GLResource.h"
#include "GLResourceManager.h"


namespace GPlatesOpenGL
{
	/**
	 * Returns true if vertex buffer objects are supported and
	 * hence @a GLVertexBufferResource can be used.
	 *
	 * Vertex buffer objects is an OpenGL extension "GL_ARB_vertex_buffer_object" for
	 * storing/transferring vertices/indices to/from the CPU/GPU.
	 */
	bool
	are_vertex_buffer_objects_supported();


	/**
	 * Policy class to allocate and deallocate OpenGL vertex buffer objects.
	 */
	class GLVertexBufferObjectAllocator
	{
	public:
		GLint
		allocate();

		void
		deallocate(
				GLuint vbo);
	};


	/**
	 * Typedef for a vertex buffer object resource.
	 */
	typedef GLResource<GLuint, GLVertexBufferObjectAllocator> GLVertexBufferResource;


	/**
	 * Typedef for a vertex buffer object resource manager.
	 */
	typedef GLResourceManager<GLuint, GLVertexBufferObjectAllocator> GLVertexBufferResourceManager;
}

#endif // GPLATES_OPENGL_GLVERTEXBUFFERRESOURCE_H
