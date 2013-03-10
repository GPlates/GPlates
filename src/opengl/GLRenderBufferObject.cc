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
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLRenderBufferObject.h"

#include "GLContext.h"
#include "GLRenderer.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLRenderBufferObject::resource_handle_type
GPlatesOpenGL::GLRenderBufferObject::Allocator::allocate()
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	GLuint render_buffer;
	glGenRenderbuffersEXT(1, &render_buffer);
	return render_buffer;
}


void
GPlatesOpenGL::GLRenderBufferObject::Allocator::deallocate(
		GLuint render_buffer)
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	glDeleteRenderbuffersEXT(1, &render_buffer);
}


GPlatesOpenGL::GLRenderBufferObject::GLRenderBufferObject(
		GLRenderer &renderer) :
	d_resource(
			resource_type::create(
					renderer.get_context().get_non_shared_state()->get_render_buffer_object_resource_manager()))
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLRenderBufferObject::gl_render_buffer_storage(
		GLRenderer &renderer,
		GLint internalformat,
		GLsizei width,
		GLsizei height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			width < boost::numeric_cast<GLsizei>(
					renderer.get_capabilities().framebuffer.gl_max_renderbuffer_size) &&
				height < boost::numeric_cast<GLsizei>(
						renderer.get_capabilities().framebuffer.gl_max_renderbuffer_size),
			GPLATES_ASSERTION_SOURCE);

	// Bind this render buffer object.
	//
	// TODO: Make this a bind method in the GLRenderer interface.
	// For now it's fine since the only reason for binding a render buffer is to set the storage on it.
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, get_render_buffer_resource_handle());

	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, internalformat, width, height);
	d_internal_format = internalformat;
	d_dimensions = std::make_pair(width, height);

	// Unbind render buffer object.
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
}
