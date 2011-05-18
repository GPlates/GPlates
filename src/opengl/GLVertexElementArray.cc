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

#include "GLVertexElementArray.h"

#include "global/CompilerWarnings.h"

#include "utils/Profile.h"


void
GPlatesOpenGL::GLVertexElementArray::gl_draw_elements(
		GLenum mode,
		GLsizei count,
		GLint indices_offset)
{
	const DrawElements draw_elements = { mode, count, indices_offset };
	d_draw_elements = draw_elements;
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLVertexElementArray::gl_draw_range_elements_EXT(
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLint indices_offset)
{
	// If the extension is supported then also specify the 'start' and 'end'.
	if (GLEW_EXT_draw_range_elements)
	{
		const DrawRangeElementsEXT draw_range_elements = { start, end };
		d_draw_range_elements = draw_range_elements;
	}

	GLVertexElementArray::gl_draw_elements(mode, count, indices_offset);
}


void
GPlatesOpenGL::GLVertexElementArray::draw() const
{
	//PROFILE_FUNC();

	// If no data has been set yet then do nothing.
	if (!d_type)
	{
		return;
	}

	// Bind to the array so that when we set the vertex attribute pointers they
	// will be directed to the bound array.
	const GLubyte *array_data = d_array_data->bind();

	if (d_draw_range_elements)
	{
		const void *indices = array_data + d_draw_elements->indices_offset;

		glDrawRangeElementsEXT(
				d_draw_elements->mode,
				d_draw_range_elements->start,
				d_draw_range_elements->end,
				d_draw_elements->count,
				d_type.get(),
				indices);
	}
	else if (d_draw_elements)
	{
		const void *indices = array_data + d_draw_elements->indices_offset;

		glDrawElements(
				d_draw_elements->mode,
				d_draw_elements->count,
				d_type.get(),
				indices);
	}

	// We've finished binding the vertex element pointer to the bound array so
	// release the binding to the array - we want to make sure we don't leave OpenGL
	// in a non-default state when we're finished drawing - this can happen if the bound
	// array is implemented using the vertex buffer objects OpenGL extension in which case
	// if we don't unbind then any subsequent vertex element arrays (that are using plain CPU
	// arrays) will not work.
	d_array_data->unbind();
}

// We use macros in <GL/glew.h> that contain old-style casts.
ENABLE_GCC_WARNING("-Wold-style-cast")
