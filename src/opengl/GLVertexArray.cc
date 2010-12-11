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

#include <boost/cast.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLVertexArray.h"

#include "GLContext.h"
#include "GLTexture.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLVertexArray::GLVertexArray() :
	d_enable_vertex_array(false),
	d_enable_color_array(false),
	d_enable_normal_array(false),
	d_client_active_texture_ARB(GL_TEXTURE0_ARB)
{
}


GPlatesOpenGL::GLVertexArray::GLVertexArray(
		const GLArray &array_data) :
	d_array_data(array_data),
	d_enable_vertex_array(false),
	d_enable_color_array(false),
	d_enable_normal_array(false),
	d_client_active_texture_ARB(GL_TEXTURE0_ARB)
{
}


GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_client_active_texture_ARB(
		GLenum texture)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture >= GL_TEXTURE0_ARB &&
					boost::numeric_cast<GLint>(texture) < GL_TEXTURE0_ARB +
							GLContext::get_texture_parameters().gl_max_texture_units_ARB,
			GPLATES_ASSERTION_SOURCE);

	d_client_active_texture_ARB = texture;

	return *this;
}


GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_vertex_pointer(
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint array_offset)
{
	const VertexPointer vertex_pointer = { size, type, stride, array_offset };
	d_vertex_pointer = vertex_pointer;

	return *this;
}


GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_color_pointer(
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint array_offset)
{
	const ColorPointer color_pointer = { size, type, stride, array_offset };
	d_color_pointer = color_pointer;

	return *this;
}


GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_normal_pointer(
		GLenum type,
		GLsizei stride,
		GLint array_offset)
{
	const NormalPointer normal_pointer = { type, stride, array_offset };
	d_normal_pointer = normal_pointer;

	return *this;
}


GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_tex_coord_pointer(
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint array_offset)
{
	const TexCoordPointer tex_coord_pointer = { size, type, stride, array_offset };
	d_tex_coord_pointer[d_client_active_texture_ARB - GL_TEXTURE0_ARB] = tex_coord_pointer;

	return *this;
}


void
GPlatesOpenGL::GLVertexArray::bind() const
{
	bind_vertex_pointer();
	bind_color_pointer();
	bind_normal_pointer();
	bind_tex_coord_pointers();
}


void
GPlatesOpenGL::GLVertexArray::bind_vertex_pointer() const
{
	// If the client:
	// - disabled the array, or
	// - hasn't set the array pointer, or
	// - hasn't specified array data
	// then disable the array.
	if (!d_enable_vertex_array || !d_vertex_pointer || !d_array_data)
	{
		glDisableClientState(GL_VERTEX_ARRAY);

		return;
	}

	glEnableClientState(GL_VERTEX_ARRAY);

	const void *pointer = static_cast<const GLubyte *>(d_array_data->get_array()) +
			d_vertex_pointer->array_offset;

	glVertexPointer(
			d_vertex_pointer->size,
			d_vertex_pointer->type,
			d_vertex_pointer->stride,
			pointer);
}


void
GPlatesOpenGL::GLVertexArray::bind_color_pointer() const
{
	// If the client:
	// - disabled the array, or
	// - hasn't set the array pointer, or
	// - hasn't specified array data
	// then disable the array.
	if (!d_enable_color_array || !d_color_pointer || !d_array_data)
	{
		glDisableClientState(GL_COLOR_ARRAY);

		return;
	}

	glEnableClientState(GL_COLOR_ARRAY);

	const void *pointer = static_cast<const GLubyte *>(d_array_data->get_array()) +
			d_color_pointer->array_offset;

	glColorPointer(
			d_color_pointer->size,
			d_color_pointer->type,
			d_color_pointer->stride,
			pointer);
}


void
GPlatesOpenGL::GLVertexArray::bind_normal_pointer() const
{
	// If the client:
	// - disabled the array, or
	// - hasn't set the array pointer, or
	// - hasn't specified array data
	// then disable the array.
	if (!d_enable_normal_array || !d_normal_pointer || !d_array_data)
	{
		glDisableClientState(GL_NORMAL_ARRAY);

		return;
	}

	glEnableClientState(GL_NORMAL_ARRAY);

	const void *pointer = static_cast<const GLubyte *>(d_array_data->get_array()) +
			d_normal_pointer->array_offset;

	glNormalPointer(
			d_normal_pointer->type,
			d_normal_pointer->stride,
			pointer);
}


void
GPlatesOpenGL::GLVertexArray::bind_tex_coord_pointers() const
{
	const std::size_t MAX_TEXTURE_UNITS = GLContext::get_texture_parameters().gl_max_texture_units_ARB;

	if (MAX_TEXTURE_UNITS == 1)
	{
		// Only have one texture unit - might mean GL_ARB_multitexture is not supported
		// so avoid calling 'glClientActiveTextureARB()'.
		bind_tex_coord_pointer(0/*texture_unit*/);

		return;
	}

	// else MAX_TEXTURE_UNITS > 1 ...

	for (std::size_t texture_unit = 0; texture_unit < MAX_TEXTURE_UNITS; ++texture_unit)
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB + texture_unit);

		bind_tex_coord_pointer(texture_unit);
	}

	// Restore to default state.
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
}


void
GPlatesOpenGL::GLVertexArray::bind_tex_coord_pointer(
		std::size_t texture_unit) const
{
	// If the client:
	// - disabled the array, or
	// - hasn't set the array pointer, or
	// - hasn't specified array data
	// then disable the array.
	if (!d_enable_texture_coord_array.test(texture_unit) ||
		!d_tex_coord_pointer[texture_unit] ||
		!d_array_data)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		return;
	}

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	const TexCoordPointer &tex_coord_pointer = d_tex_coord_pointer[texture_unit].get();

	const void *pointer = static_cast<const GLubyte *>(d_array_data->get_array()) +
			tex_coord_pointer.array_offset;

	glTexCoordPointer(
			tex_coord_pointer.size,
			tex_coord_pointer.type,
			tex_coord_pointer.stride,
			pointer);
}


void
GPlatesOpenGL::GLVertexArray::set_enable_disable_client_state(
		GLenum array,
		bool enable)
{
	switch (array)
	{
	case GL_VERTEX_ARRAY:
		d_enable_vertex_array = enable;
		break;
	case GL_COLOR_ARRAY:
		d_enable_color_array = enable;
		break;
	case GL_NORMAL_ARRAY:
		d_enable_normal_array = enable;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		// Enable the texture unit currently set by 'gl_client_active_texture_ARB()'.
		d_enable_texture_coord_array.set(
				d_client_active_texture_ARB - GL_TEXTURE0_ARB,
				enable);
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}
