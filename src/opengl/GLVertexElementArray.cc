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


void
GPlatesOpenGL::GLVertexElementArray::gl_draw_elements(
		GLenum mode,
		GLsizei count,
		GLenum type,
		GLint indices_offset)
{
	const DrawElements draw_elements = { mode, count, type, indices_offset };
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
		GLenum type,
		GLint indices_offset)
{
	// If the extension is supported then also specify the 'start' and 'end'.
	if (GLEW_EXT_draw_range_elements)
	{
		const DrawRangeElementsEXT draw_range_elements = { start, end };
		d_draw_range_elements = draw_range_elements;
	}

	GLVertexElementArray::gl_draw_elements(mode, count, type, indices_offset);
}

// We use macros in <GL/glew.h> that contain old-style casts.
ENABLE_GCC_WARNING("-Wold-style-cast")


void
GPlatesOpenGL::GLVertexElementArray::draw() const
{
	// If no array data has been specified then nothing to draw.
	if (!d_array_data)
	{
		return;
	}

	if (d_draw_range_elements)
	{
		const void *indices = static_cast<const GLubyte *>(d_array_data->get_array()) +
				d_draw_elements->indices_offset;

		glDrawRangeElementsEXT(
				d_draw_elements->mode,
				d_draw_range_elements->start,
				d_draw_range_elements->end,
				d_draw_elements->count,
				d_draw_elements->type,
				indices);
		return;
	}

	if (d_draw_elements)
	{
		const void *indices = static_cast<const GLubyte *>(d_array_data->get_array()) +
				d_draw_elements->indices_offset;

		glDrawElements(
				d_draw_elements->mode,
				d_draw_elements->count,
				d_draw_elements->type,
				indices);
		return;
	}
}
