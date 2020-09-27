/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include <QtGlobal>

#include "GLStreamBuffer.h"

#include "GLUtils.h"
#include "OpenGLException.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLStreamBuffer::GLStreamBuffer(
		GLBuffer::shared_ptr_type buffer,
		unsigned int buffer_size) :
	d_buffer(buffer),
	d_buffer_size(buffer_size),
	d_uninitialised_offset(0),
	d_created_buffer_data_store(false)
{
}


GPlatesOpenGL::GLStreamBuffer::MapScope::MapScope(
		GLenum target,
		GLStreamBuffer &stream_buffer,
		unsigned int d_minimum_bytes_to_stream,
		unsigned int stream_alignment) :
	d_target(target),
	d_stream_buffer(stream_buffer),
	d_minimum_bytes_to_stream(d_minimum_bytes_to_stream),
	d_stream_alignment(stream_alignment),
	d_is_mapped(false)
{
}


GPlatesOpenGL::GLStreamBuffer::MapScope::~MapScope()
{
	if (d_is_mapped)
	{
		// Flushes entire mapped range.
		glUnmapBuffer(d_target);
	}
}


GLvoid *
GPlatesOpenGL::GLStreamBuffer::MapScope::map(
		unsigned int &stream_offset,
		unsigned int &stream_bytes_available)
{
	// 'd_minimum_bytes_to_stream' must be in the half-open range (0, d_buffer_size].
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			0 < d_minimum_bytes_to_stream &&
				d_minimum_bytes_to_stream <= d_stream_buffer.d_buffer_size,
			GPLATES_ASSERTION_SOURCE);

	// Create the buffer data store if we haven't already (this is only done once).
	if (!d_stream_buffer.d_created_buffer_data_store)
	{
		// Allocate the (uninitialised) data store.
		//
		// Data will be specified by the application and used at most a few times (hence 'GL_STREAM_DRAW').
		glBufferData(d_target, d_stream_buffer.d_buffer_size, nullptr, GL_STREAM_DRAW);

		d_stream_buffer.d_created_buffer_data_store = true;
	}

	// The stream offset must be multiple of 'd_stream_alignment'.
	// Note that this does nothing if 'd_uninitialised_offset' is zero (ie, contains no initialised data).
	const unsigned int stream_offset_adjust = d_stream_buffer.d_uninitialised_offset % d_stream_alignment;
	if (stream_offset_adjust)
	{
		d_stream_buffer.d_uninitialised_offset += d_stream_alignment - stream_offset_adjust;
	}

	// Discard the current buffer allocation if there's not enough uninitialised memory
	// at the end of the buffer.
	bool discard = false;
	if (d_stream_buffer.d_uninitialised_offset + d_minimum_bytes_to_stream > d_stream_buffer.d_buffer_size)
	{
		discard = true;
	}

	// 'GL_MAP_FLUSH_EXPLICIT_BIT' means the buffer will need to be explicitly flushed
	// (using glFlushMappedBufferRange).
	GLbitfield range_access = (GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

	// We're either:
	//  1) discarding/orphaning the buffer to get a new buffer allocation (internally by OpenGL) of same size, or
	//  2) forgoing synchronisation because we are promising not to overwrite current buffer data.
	if (discard)
	{
		range_access |= GL_MAP_INVALIDATE_BUFFER_BIT;

		// Since we're invalidating the buffer we can consider the entire buffer uninitialised.
		d_stream_buffer.d_uninitialised_offset = 0;
	}
	else // client is going to write to uninitialised memory in current buffer...
	{
		// 'GL_MAP_UNSYNCHRONIZED_BIT' stops OpenGL from blocking, otherwise the GPU might block
		// until it is finished using any data currently in the buffer.
		// Note that we don't specify 'GL_MAP_UNSYNCHRONIZED_BIT' when discarding (with
		// 'GL_MAP_INVALIDATE_BUFFER_BIT') because it's possible OpenGL could return the
		// same buffer (rather than internally keeping the existing buffer for its pending GPU
		// operations and returning a fresh new buffer to us) and hence we could be overwriting
		// data that hasn't been consumed by the GPU yet. I've seen this happen when specifying
		// 'GL_MAP_UNSYNCHRONIZED_BIT' with 'GL_MAP_INVALIDATE_BUFFER_BIT' (on nVidia 780Ti using
		// 364.96 driver) - manifested as flickering cross-sections and surface masks when using
		// them with 3D scalar fields.
		range_access |= (GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	}

	// We only need to map the uninitialised region at the end of the buffer.
	GLvoid *mapped_data = glMapBufferRange(
			d_target,
			d_stream_buffer.d_uninitialised_offset,
			d_stream_buffer.d_buffer_size - d_stream_buffer.d_uninitialised_offset,
			range_access);

	// If there was an error during mapping then report it and throw exception.
	if (mapped_data == NULL)
	{
		// Log OpenGL error - a mapped data pointer of NULL should generate an OpenGL error.
		GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);

		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLBufferStream::MapScope::map: failed to map OpenGL buffer object.");
	}

	// Initialise the caller's parameters.
	stream_offset = d_stream_buffer.d_uninitialised_offset;
	stream_bytes_available = d_stream_buffer.d_buffer_size - d_stream_buffer.d_uninitialised_offset;

	// Buffer is now mapped.
	d_is_mapped = true;

	return mapped_data;
}


GLboolean
GPlatesOpenGL::GLStreamBuffer::MapScope::unmap(
		unsigned int bytes_written)
{
	if (bytes_written > 0)
	{
		// Bytes written must fit within existing buffer.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_stream_buffer.d_uninitialised_offset + bytes_written <= d_stream_buffer.d_buffer_size,
				GPLATES_ASSERTION_SOURCE);

		// Only flush the requested range.
		//
		// Note that the offset is zero and not 'd_uninitialised_offset' since the mapped region
		// was not the entire buffer (only the uninitialised region at the end of the buffer).
		glFlushMappedBufferRange(d_target, 0, bytes_written);
	}

	if (!glUnmapBuffer(d_target))
	{
		// Check OpenGL errors in case glUnmapBuffer used incorrectly.
		GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);

		// Otherwise the buffer contents have been corrupted.
		qWarning() << "GLBufferStream::MapScope::unmap: "
				"OpenGL buffer object contents have been corrupted (such as an ALT+TAB switch between applications).";

		return GL_FALSE;
	}

	// Buffer is no longer mapped.
	d_is_mapped = false;

	return GL_TRUE;
}
