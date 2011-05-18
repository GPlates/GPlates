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

#include "GLVertex.h"

#include "GLContext.h"
#include "GLVertexArray.h"


namespace GPlatesOpenGL
{
	template <>
	void
	configure_vertex_array<GLVertex>(
			GLVertexArray &vertex_array)
	{
		//
		// The following reflects the structure of 'struct GLVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.gl_enable_client_state(GL_VERTEX_ARRAY);
		vertex_array.gl_vertex_pointer(3, GL_FLOAT, sizeof(GLVertex), 0);
	}


	template<>
	void
	configure_vertex_array<GLColouredVertex>(
			GLVertexArray &vertex_array)
	{
		//
		// The following reflects the structure of 'struct GLColouredVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.gl_enable_client_state(GL_VERTEX_ARRAY);
		vertex_array.gl_vertex_pointer(3, GL_FLOAT, sizeof(GLColouredVertex), 0);

		vertex_array.gl_enable_client_state(GL_COLOR_ARRAY);
		vertex_array.gl_color_pointer(4, GL_UNSIGNED_BYTE, sizeof(GLColouredVertex), 3 * sizeof(GLfloat));
	}


	template<>
	void
	configure_vertex_array<GLTexturedVertex>(
			GLVertexArray &vertex_array)
	{
		//
		// The following reflects the structure of 'struct GLTexturedVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.gl_enable_client_state(GL_VERTEX_ARRAY);
		vertex_array.gl_vertex_pointer(3, GL_FLOAT, sizeof(GLTexturedVertex), 0);

		vertex_array.gl_client_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
		vertex_array.gl_enable_client_state(GL_TEXTURE_COORD_ARRAY);
		vertex_array.gl_tex_coord_pointer(2, GL_FLOAT, sizeof(GLTexturedVertex), 3 * sizeof(GLfloat));

	}


	template<>
	void
	configure_vertex_array<GLColouredTexturedVertex>(
			GLVertexArray &vertex_array)
	{
		//
		// The following reflects the structure of 'struct GLColouredTexturedVertex'.
		// It defines how the elements of the vertex are packed together in the vertex.
		// It's an OpenGL thing.
		//

		vertex_array.gl_enable_client_state(GL_VERTEX_ARRAY);
		vertex_array.gl_vertex_pointer(3, GL_FLOAT, sizeof(GLColouredTexturedVertex), 0);

		vertex_array.gl_client_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
		vertex_array.gl_enable_client_state(GL_TEXTURE_COORD_ARRAY);
		vertex_array.gl_tex_coord_pointer(2, GL_FLOAT, sizeof(GLColouredTexturedVertex), 3 * sizeof(GLfloat));

		vertex_array.gl_enable_client_state(GL_COLOR_ARRAY);
		vertex_array.gl_color_pointer(4, GL_UNSIGNED_BYTE, sizeof(GLColouredTexturedVertex), 5 * sizeof(GLfloat));

	}
}

