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

#include "GLVertexArray.h"

#include "GLContext.h"
#include "OpenGL.h"  // For Class GL
#include "OpenGLFunctions.h"


GPlatesOpenGL::GLVertexArray::GLVertexArray(
		GL &gl) :
	d_resource(
			resource_type::create(
					gl.get_opengl_functions(),
					gl.get_capabilities(),
					gl.get_context().get_shared_state()->get_vertex_array_resource_manager()))
{
}


void
GPlatesOpenGL::GLVertexArray::clear(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind this vertex array first.
	// Note that this is context state (not VAO state) and will get restored on leaving the current scope.
	gl.BindVertexArray(shared_from_this());

	// Unbind element array buffer.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, boost::none);

	// Unbind array buffer.
	// Note that this is context state (not VAO state) and will get restored on leaving the current scope.
	gl.BindBuffer(GL_ARRAY_BUFFER, boost::none);

	// Reset the attribute arrays.
	const unsigned int num_attribute_arrays = gl.get_capabilities().gl_max_vertex_attribs;
	for (GLuint attribute_index = 0; attribute_index < num_attribute_arrays; ++attribute_index)
	{
		gl.DisableVertexAttribArray(attribute_index);
		gl.VertexAttribDivisor(attribute_index, 0);
		gl.VertexAttribPointer(attribute_index, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	}
}


GLuint
GPlatesOpenGL::GLVertexArray::get_resource_handle() const
{
	return d_resource->get_resource_handle();
}


GLuint
GPlatesOpenGL::GLVertexArray::Allocator::allocate(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities)
{
	GLuint vertex_array_object;
	opengl_functions.glGenVertexArrays(1, &vertex_array_object);
	return vertex_array_object;
}


void
GPlatesOpenGL::GLVertexArray::Allocator::deallocate(
		OpenGLFunctions &opengl_functions,
		GLuint vertex_array_object)
{
	opengl_functions.glDeleteVertexArrays(1, &vertex_array_object);
}
