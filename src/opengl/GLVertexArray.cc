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

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLVertexArray::GLVertexArray(
		const GLArray::non_null_ptr_type &array_data) :
	d_array_data(array_data),
	d_client_active_texture_ARB(GL_TEXTURE0_ARB),
	d_enable_vertex_array(false),
	d_enable_color_array(false),
	d_enable_normal_array(false)
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
		const VertexPointer &vertex_pointer)
{
	d_vertex_pointer = vertex_pointer;

	return *this;
}


GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_color_pointer(
		const ColorPointer &color_pointer)
{
	d_color_pointer = color_pointer;

	return *this;
}


GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_normal_pointer(
		const NormalPointer &normal_pointer)
{
	d_normal_pointer = normal_pointer;

	return *this;
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

GPlatesOpenGL::GLVertexArray &
GPlatesOpenGL::GLVertexArray::gl_tex_coord_pointer(
		const TexCoordPointer &tex_coord_pointer)
{
	const std::size_t texture_unit = d_client_active_texture_ARB - GL_TEXTURE0_ARB;

	d_tex_coord_pointer[texture_unit] = tex_coord_pointer;

	return *this;
}


void
GPlatesOpenGL::GLVertexArray::bind() const
{
	//PROFILE_FUNC();

	// Bind to the array so that when we set the vertex attribute pointers they
	// will be directed to the bound array.
	const GLubyte *array_data = d_array_data->bind();

	// Bind our vertex attribute pointers to the array.
	bind_attribute_pointers(array_data);

	// We've finished binding the vertex attribute pointers to the bound array so
	// release the binding to the array - we can do this because the attribute pointers
	// now carry the "bind" information - also we want to make sure we don't leave OpenGL
	// in a non-default state when we're finished drawing - this can happen if the bound
	// array is implemented using the vertex buffer objects OpenGL extension in which case
	// if we don't unbind then any subsequent vertex arrays (that are using plain CPU
	// arrays) will not work.
	d_array_data->unbind();

	// Enable/disable the requested vertex attribute client state.
	bind_client_state();
}


void
GPlatesOpenGL::GLVertexArray::disable_client_state()
{
	d_enable_vertex_array = false;
	d_enable_color_array = false;
	d_enable_normal_array = false;
	d_enable_texture_coord_array.reset();
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


void
GPlatesOpenGL::GLVertexArray::bind_client_state() const
{
	if (d_enable_vertex_array)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
	}
	else
	{
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	if (d_enable_color_array)
	{
		glEnableClientState(GL_COLOR_ARRAY);
	}
	else
	{
		glDisableClientState(GL_COLOR_ARRAY);
	}

	if (d_enable_normal_array)
	{
		glEnableClientState(GL_NORMAL_ARRAY);
	}
	else
	{
		glDisableClientState(GL_NORMAL_ARRAY);
	}

	const std::size_t MAX_TEXTURE_UNITS = GLContext::get_texture_parameters().gl_max_texture_units_ARB;

	for (std::size_t texture_unit = 0; texture_unit < MAX_TEXTURE_UNITS; ++texture_unit)
	{
		// If the multitexture extension is not supported then MAX_TEXTURE_UNITS should be one.
		if (GLEW_ARB_multitexture)
		{
			glClientActiveTextureARB(GL_TEXTURE0_ARB + texture_unit);
		}

		if (d_enable_texture_coord_array.test(texture_unit))
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		else
		{
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
	}

	// Reset to the default OpenGL state (ie, point to texture unit 0).
	if (GLEW_ARB_multitexture)
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
}


void
GPlatesOpenGL::GLVertexArray::bind_attribute_pointers(
		const GLubyte *array_data) const
{
	if (d_color_pointer)
	{
		glColorPointer(
				d_color_pointer->size,
				d_color_pointer->type,
				d_color_pointer->stride,
				array_data + d_color_pointer->array_offset);
	}

	if (d_normal_pointer)
	{
		glNormalPointer(
				d_normal_pointer->type,
				d_normal_pointer->stride,
				array_data + d_normal_pointer->array_offset);
	}

	const std::size_t MAX_TEXTURE_UNITS = GLContext::get_texture_parameters().gl_max_texture_units_ARB;

	for (std::size_t texture_unit = 0; texture_unit < MAX_TEXTURE_UNITS; ++texture_unit)
	{
		// If the multitexture extension is not supported then MAX_TEXTURE_UNITS should be one.
		if (GLEW_ARB_multitexture)
		{
			glClientActiveTextureARB(GL_TEXTURE0_ARB + texture_unit);
		}

		if (d_tex_coord_pointer[texture_unit])
		{
			glTexCoordPointer(
					d_tex_coord_pointer[texture_unit]->size,
					d_tex_coord_pointer[texture_unit]->type,
					d_tex_coord_pointer[texture_unit]->stride,
					array_data + d_tex_coord_pointer[texture_unit]->array_offset);
		}
	}

	// Reset to the default OpenGL state (ie, point to texture unit 0).
	if (GLEW_ARB_multitexture)
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}

	// Apparently for some OpenGL vendors it's a bit more efficient to do the
	// 'glVertexPointer()' call last since that function sets up all the attribute pointers.
	if (d_vertex_pointer)
	{
		glVertexPointer(
				d_vertex_pointer->size,
				d_vertex_pointer->type,
				d_vertex_pointer->stride,
				array_data + d_vertex_pointer->array_offset);
	}
}

// We use macros in <GL/glew.h> that contain old-style casts.
ENABLE_GCC_WARNING("-Wold-style-cast")
