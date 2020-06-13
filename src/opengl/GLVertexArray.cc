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

#include "GLVertexArray.h"

#include "GLRenderer.h"
#include "GLVertexArrayImpl.h"
#include "GLVertexArrayObject.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


std::unique_ptr<GPlatesOpenGL::GLVertexArray>
GPlatesOpenGL::GLVertexArray::create_as_unique_ptr(
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Create an OpenGL vertex array object if we can.
#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	if (capabilities.buffer.gl_ARB_vertex_array_object)
	{
		return std::unique_ptr<GPlatesOpenGL::GLVertexArray>(
				GLVertexArrayObject::create_as_unique_ptr(renderer));
	}
#endif

	return std::unique_ptr<GPlatesOpenGL::GLVertexArray>(
			GLVertexArrayImpl::create_as_unique_ptr(renderer));
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::compile_vertex_array_draw_state(
		GLRenderer &renderer,
		GLVertexArray &vertex_array,
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		GLint indices_offset)
{
	// Start compiling draw state.
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

	// Bind the vertex array.
	vertex_array.gl_bind(renderer);

	// Draw the vertex array.
	vertex_array.gl_draw_range_elements(
			renderer,
			mode,
			start,
			end,
			count,
			type,
			indices_offset);

	return compile_draw_state_scope.get_compiled_draw_state();
}
