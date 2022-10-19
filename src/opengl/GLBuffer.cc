/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "GLBuffer.h"

#include "OpenGL.h"  // For Class GL


GPlatesOpenGL::GLBuffer::GLBuffer(
		GL &gl) :
	d_resource(gl.get_opengl_functions(), gl.get_context())
{
}


GLuint
GPlatesOpenGL::GLBuffer::get_resource_handle() const
{
	return d_resource.get_resource_handle();
}


GLuint
GPlatesOpenGL::GLBuffer::Allocator::allocate(
		OpenGLFunctions &opengl_functions)
{
	GLuint buffer_object;
	opengl_functions.glCreateBuffers(1, &buffer_object);
	return buffer_object;
}


void
GPlatesOpenGL::GLBuffer::Allocator::deallocate(
		OpenGLFunctions &opengl_functions,
		GLuint buffer_object)
{
	opengl_functions.glDeleteBuffers(1, &buffer_object);
}
