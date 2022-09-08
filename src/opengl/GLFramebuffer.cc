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

#include "GLFramebuffer.h"

#include "GLContext.h"
#include "OpenGL.h"  // For Class GL
#include "OpenGLFunctions.h"


GPlatesOpenGL::GLFramebuffer::GLFramebuffer(
		GL &gl) :
	d_resource(
			resource_type::create(
					gl.get_opengl_functions(),
					gl.get_capabilities(),
					gl.get_context().d_framebuffer_resource_manager))
{
}


GLuint
GPlatesOpenGL::GLFramebuffer::get_resource_handle() const
{
	return d_resource->get_resource_handle();
}


GLuint
GPlatesOpenGL::GLFramebuffer::Allocator::allocate(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities)
{
	GLuint fbo;
	opengl_functions.glGenFramebuffers(1, &fbo);
	return fbo;
}


void
GPlatesOpenGL::GLFramebuffer::Allocator::deallocate(
		OpenGLFunctions &opengl_functions,
		GLuint fbo)
{
	opengl_functions.glDeleteFramebuffers(1, &fbo);
}
