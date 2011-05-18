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

#include "GLVertexBufferResource.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

bool
GPlatesOpenGL::are_vertex_buffer_objects_supported()
{
	return GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object);
}


GLint
GPlatesOpenGL::GLVertexBufferObjectAllocator::allocate()
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	GLuint vbo;
	glGenBuffersARB(1, &vbo);
	return vbo;
}


void
GPlatesOpenGL::GLVertexBufferObjectAllocator::deallocate(
		GLuint vbo)
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	glDeleteBuffersARB(1, &vbo);
}

// We use macros in <GL/glew.h> that contain old-style casts.
ENABLE_GCC_WARNING("-Wold-style-cast")
