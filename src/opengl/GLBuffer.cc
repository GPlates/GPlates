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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLBuffer.h"

#include "GLBufferImpl.h"
#include "GLContext.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"

const GPlatesOpenGL::GLBuffer::target_type GPlatesOpenGL::GLBuffer::TARGET_ARRAY_BUFFER = GL_ARRAY_BUFFER_ARB;
const GPlatesOpenGL::GLBuffer::target_type GPlatesOpenGL::GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER = GL_ELEMENT_ARRAY_BUFFER_ARB;
const GPlatesOpenGL::GLBuffer::target_type GPlatesOpenGL::GLBuffer::TARGET_PIXEL_UNPACK_BUFFER = GL_PIXEL_UNPACK_BUFFER_ARB;
const GPlatesOpenGL::GLBuffer::target_type GPlatesOpenGL::GLBuffer::TARGET_PIXEL_PACK_BUFFER = GL_PIXEL_PACK_BUFFER_ARB;

const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_STATIC_DRAW = GL_STATIC_DRAW_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_STATIC_READ = GL_STATIC_READ_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_STATIC_COPY = GL_STATIC_COPY_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_DYNAMIC_DRAW = GL_DYNAMIC_DRAW_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_DYNAMIC_READ = GL_DYNAMIC_READ_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_DYNAMIC_COPY = GL_DYNAMIC_COPY_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_STREAM_DRAW = GL_STREAM_DRAW_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_STREAM_READ = GL_STREAM_READ_ARB;
const GPlatesOpenGL::GLBuffer::usage_type GPlatesOpenGL::GLBuffer::USAGE_STREAM_COPY = GL_STREAM_COPY_ARB;

const GPlatesOpenGL::GLBuffer::access_type GPlatesOpenGL::GLBuffer::ACCESS_READ_ONLY = GL_READ_ONLY_ARB;
const GPlatesOpenGL::GLBuffer::access_type GPlatesOpenGL::GLBuffer::ACCESS_WRITE_ONLY = GL_WRITE_ONLY_ARB;
const GPlatesOpenGL::GLBuffer::access_type GPlatesOpenGL::GLBuffer::ACCESS_READ_WRITE = GL_READ_WRITE_ARB;


std::auto_ptr<GPlatesOpenGL::GLBuffer>
GPlatesOpenGL::GLBuffer::create_as_auto_ptr(
		GLRenderer &renderer)
{
	// Create an OpenGL buffer object if we can.
	if (GLEW_ARB_vertex_buffer_object)
	{
		return std::auto_ptr<GPlatesOpenGL::GLBuffer>(
				GLBufferObject::create_as_auto_ptr(renderer));
	}

	return std::auto_ptr<GPlatesOpenGL::GLBuffer>(
			GLBufferImpl::create_as_auto_ptr(renderer));
}


GPlatesOpenGL::GLBuffer::MapBufferScope::MapBufferScope(
		GLRenderer &renderer,
		GLBuffer &buffer,
		target_type target) :
	d_renderer(renderer),
	d_buffer(buffer),
	d_target(target),
	d_data(NULL)
{
}


GPlatesOpenGL::GLBuffer::MapBufferScope::~MapBufferScope()
{
	if (d_data)
	{
		// Since this is a destructor we cannot let any exceptions escape.
		// If one is thrown we just have to lump it and continue on.
		try
		{
			gl_unmap_buffer();
		}
		catch (...)
		{  }
	}
}


GLvoid *
GPlatesOpenGL::GLBuffer::MapBufferScope::gl_map_buffer_static(
		access_type access)
{
	// Make sure 'gl_unmap_buffer' was called, or this is first time called.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_data == NULL,
			GPLATES_ASSERTION_SOURCE);

	d_data = d_buffer.gl_map_buffer_static(d_renderer, d_target, access);

	return d_data;
}


GLvoid *
GPlatesOpenGL::GLBuffer::MapBufferScope::gl_map_buffer_dynamic()
{
	// Make sure 'gl_unmap_buffer' was called, or this is first time called.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_data == NULL,
			GPLATES_ASSERTION_SOURCE);

	d_data = d_buffer.gl_map_buffer_dynamic(d_renderer, d_target);

	return d_data;
}


GLvoid *
GPlatesOpenGL::GLBuffer::MapBufferScope::gl_map_buffer_stream(
		unsigned int minimum_bytes_to_stream,
		unsigned int stream_alignment,
		unsigned int &stream_offset,
		unsigned int &stream_bytes_available)
{
	// Make sure 'gl_unmap_buffer' was called, or this is first time called.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_data == NULL,
			GPLATES_ASSERTION_SOURCE);

	d_data = d_buffer.gl_map_buffer_stream(
			d_renderer,
			d_target,
			minimum_bytes_to_stream,
			stream_alignment,
			stream_offset,
			stream_bytes_available);

	return d_data;
}


void
GPlatesOpenGL::GLBuffer::MapBufferScope::gl_flush_buffer_dynamic(
		unsigned int offset,
		unsigned int length/*in bytes*/)
{
	// Make sure 'gl_map_buffer_dynamic' was called and was successful.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_data,
			GPLATES_ASSERTION_SOURCE);

	d_buffer.gl_flush_buffer_dynamic(d_renderer, d_target, offset, length);
}


void
GPlatesOpenGL::GLBuffer::MapBufferScope::gl_flush_buffer_stream(
		unsigned int bytes_written)
{
	// Make sure 'gl_map_buffer_stream' was called and was successful.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_data,
			GPLATES_ASSERTION_SOURCE);

	d_buffer.gl_flush_buffer_stream(d_renderer, d_target, bytes_written);
}


GLboolean
GPlatesOpenGL::GLBuffer::MapBufferScope::gl_unmap_buffer()
{
	// Make sure 'gl_map_buffer_static' was called and was successful.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_data,
			GPLATES_ASSERTION_SOURCE);

	d_data = NULL;

	return d_buffer.gl_unmap_buffer(d_renderer, d_target);
}
