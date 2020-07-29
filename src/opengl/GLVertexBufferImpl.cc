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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include "GLVertexBufferImpl.h"

#include "GLRenderer.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::GLVertexBufferImpl::GLVertexBufferImpl(
		GLRenderer &renderer,
		const GLBufferImpl::shared_ptr_type &buffer) :
	d_buffer(buffer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// We should only get here if the vertex buffer object extension is *not* supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!capabilities.buffer.gl_ARB_vertex_buffer_object,
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLBuffer::shared_ptr_to_const_type
GPlatesOpenGL::GLVertexBufferImpl::get_buffer() const
{
	return d_buffer;
}


void
GPlatesOpenGL::GLVertexBufferImpl::gl_vertex_pointer(
		GLRenderer &renderer,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset) const
{
	renderer.gl_vertex_pointer(size, type, stride, offset,
			// 'static_pointer_cast' to keep earlier boost versions happy...
			boost::static_pointer_cast<const GLBufferImpl>(d_buffer));
}


void
GPlatesOpenGL::GLVertexBufferImpl::gl_color_pointer(
		GLRenderer &renderer,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset) const
{
	renderer.gl_color_pointer(size, type, stride, offset,
			// 'static_pointer_cast' to keep earlier boost versions happy...
			boost::static_pointer_cast<const GLBufferImpl>(d_buffer));
}


void
GPlatesOpenGL::GLVertexBufferImpl::gl_normal_pointer(
		GLRenderer &renderer,
		GLenum type,
		GLsizei stride,
		GLint offset) const
{
	renderer.gl_normal_pointer(type, stride, offset,
			// 'static_pointer_cast' to keep earlier boost versions happy...
			boost::static_pointer_cast<const GLBufferImpl>(d_buffer));
}


void
GPlatesOpenGL::GLVertexBufferImpl::gl_tex_coord_pointer(
		GLRenderer &renderer,
		GLenum texture_unit,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset) const
{
	renderer.gl_tex_coord_pointer(
			size,
			type,
			stride,
			offset,
			// 'static_pointer_cast' to keep earlier boost versions happy...
			boost::static_pointer_cast<const GLBufferImpl>(d_buffer),
			texture_unit);
}


void
GPlatesOpenGL::GLVertexBufferImpl::gl_vertex_attrib_pointer(
		GLRenderer &renderer,
		GLuint attribute_index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		GLint offset) const
{
	renderer.gl_vertex_attrib_pointer(
			attribute_index,
			size,
			type,
			normalized,
			stride,
			offset,
			// 'static_pointer_cast' to keep earlier boost versions happy...
			boost::static_pointer_cast<const GLBufferImpl>(d_buffer));
}


void
GPlatesOpenGL::GLVertexBufferImpl::gl_vertex_attrib_i_pointer(
		GLRenderer &renderer,
		GLuint attribute_index,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset) const
{
	renderer.gl_vertex_attrib_i_pointer(
			attribute_index,
			size,
			type,
			stride,
			offset,
			// 'static_pointer_cast' to keep earlier boost versions happy...
			boost::static_pointer_cast<const GLBufferImpl>(d_buffer));
}


void
GPlatesOpenGL::GLVertexBufferImpl::gl_vertex_attrib_l_pointer(
		GLRenderer &renderer,
		GLuint attribute_index,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset) const
{
	renderer.gl_vertex_attrib_l_pointer(
			attribute_index,
			size,
			type,
			stride,
			offset,
			// 'static_pointer_cast' to keep earlier boost versions happy...
			boost::static_pointer_cast<const GLBufferImpl>(d_buffer));
}
