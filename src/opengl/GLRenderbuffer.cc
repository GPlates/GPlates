/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <boost/cast.hpp>

#include "GLRenderbuffer.h"

#include "GL.h"
#include "GLContext.h"
#include "OpenGLFunctions.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GLuint
GPlatesOpenGL::GLRenderbuffer::Allocator::allocate(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities)
{
	GLuint renderbuffer;
	opengl_functions.glGenRenderbuffers(1, &renderbuffer);
	return renderbuffer;
}


void
GPlatesOpenGL::GLRenderbuffer::Allocator::deallocate(
		OpenGLFunctions &opengl_functions,
		GLuint renderbuffer)
{
	opengl_functions.glDeleteRenderbuffers(1, &renderbuffer);
}


GPlatesOpenGL::GLRenderbuffer::GLRenderbuffer(
		GL &gl) :
	d_resource(
			resource_type::create(
					gl.get_opengl_functions(),
					gl.get_capabilities(),
					gl.get_context().get_shared_state()->get_renderbuffer_resource_manager()))
{
}


GLuint
GPlatesOpenGL::GLRenderbuffer::get_resource_handle() const
{
	return d_resource->get_resource_handle();
}
