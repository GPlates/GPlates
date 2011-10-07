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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLVertex.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "GLVertexArray.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	template <>
	void
	bind_vertex_buffer_to_vertex_array<GLVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset)
	{
		//
		// The following reflects the structure of 'struct GLVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.set_enable_client_state(renderer, GL_VERTEX_ARRAY, true/*enable*/);
		vertex_array.set_vertex_pointer(
				renderer,
				vertex_buffer,
				3,
				GL_FLOAT,
				sizeof(GLVertex),
				offset);
	}


	template<>
	void
	bind_vertex_buffer_to_vertex_array<GLColouredVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset)
	{
		//
		// The following reflects the structure of 'struct GLColouredVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.set_enable_client_state(renderer, GL_VERTEX_ARRAY, true/*enable*/);
		vertex_array.set_vertex_pointer(
				renderer,
				vertex_buffer,
				3,
				GL_FLOAT,
				sizeof(GLColouredVertex),
				offset);

		vertex_array.set_enable_client_state(renderer, GL_COLOR_ARRAY, true/*enable*/);
		vertex_array.set_color_pointer(
				renderer,
				vertex_buffer,
				4,
				GL_UNSIGNED_BYTE,
				sizeof(GLColouredVertex),
				offset + 3 * sizeof(GLfloat));
	}


	template<>
	void
	bind_vertex_buffer_to_vertex_array<GLTexturedVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset)
	{
		//
		// The following reflects the structure of 'struct GLTexturedVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.set_enable_client_state(renderer, GL_VERTEX_ARRAY, true/*enable*/);
		vertex_array.set_vertex_pointer(
				renderer,
				vertex_buffer,
				3,
				GL_FLOAT,
				sizeof(GLTexturedVertex),
				offset);

		vertex_array.set_enable_client_texture_state(renderer, GL_TEXTURE0, true/*enable*/);
		vertex_array.set_tex_coord_pointer(
				renderer,
				vertex_buffer,
				GL_TEXTURE0,
				2,
				GL_FLOAT,
				sizeof(GLTexturedVertex),
				offset + 3 * sizeof(GLfloat));
	}


	template<>
	void
	bind_vertex_buffer_to_vertex_array<GLColouredTexturedVertex>(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
			GLint offset)
	{
		//
		// The following reflects the structure of 'struct GLColouredTexturedVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.set_enable_client_state(renderer, GL_VERTEX_ARRAY, true/*enable*/);
		vertex_array.set_vertex_pointer(
				renderer,
				vertex_buffer,
				3,
				GL_FLOAT,
				sizeof(GLColouredTexturedVertex),
				offset);

		vertex_array.set_enable_client_texture_state(renderer, GL_TEXTURE0, true/*enable*/);
		vertex_array.set_tex_coord_pointer(
				renderer,
				vertex_buffer,
				GL_TEXTURE0,
				2,
				GL_FLOAT,
				sizeof(GLColouredTexturedVertex),
				offset + 3 * sizeof(GLfloat));

		vertex_array.set_enable_client_state(renderer, GL_COLOR_ARRAY, true/*enable*/);
		vertex_array.set_color_pointer(
				renderer,
				vertex_buffer,
				4,
				GL_UNSIGNED_BYTE,
				sizeof(GLColouredTexturedVertex),
				offset + 5 * sizeof(GLfloat));
	}
}
