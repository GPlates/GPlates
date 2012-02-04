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
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLBufferObject.h"

#include "OpenGLException.h"
#include "GLContext.h"
#include "GLRenderer.h"
#include "GLUtils.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLBufferObject::resource_handle_type
GPlatesOpenGL::GLBufferObject::Allocator::allocate()
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	resource_handle_type buffer_object;
	glGenBuffersARB(1, &buffer_object);
	return buffer_object;
}


void
GPlatesOpenGL::GLBufferObject::Allocator::deallocate(
		resource_handle_type buffer_object)
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	glDeleteBuffersARB(1, &buffer_object);
}


GPlatesOpenGL::GLBufferObject::GLBufferObject(
		GLRenderer &renderer) :
	d_resource(
			resource_type::create(
					renderer.get_context().get_shared_state()->get_buffer_object_resource_manager())),
	d_size(0),
	d_uninitialised_offset(0)
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLBufferObject::gl_buffer_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int size,
		const void* data,
		usage_type usage)
{
	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glBufferDataARB(target, size, data, usage);

	// Record the new buffer size.
	d_size = size;

	// Record usage of new buffer.
	d_usage = usage;

	// If the new memory allocation contains un-initialised data then we can write to it without
	// interfering with data the GPU might currently be reading (eg, due to a draw call).
	// Otherwise set the offset to the end of the buffer (equivalent to the inability to stream).
	d_uninitialised_offset = (data == NULL) ? 0 : size;

	// Notify clients that a buffer allocation has occurred.
	allocated_buffer();
}


void
GPlatesOpenGL::GLBufferObject::gl_buffer_sub_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int size,
		const void* data)
{
	// Range must fit within existing buffer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			offset + size <= d_size,
			GPLATES_ASSERTION_SOURCE);

	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glBufferSubDataARB(target, offset, size, data);

	// If we're loading data that overlaps with the un-initialised region of the buffer then
	// update the offset into un-initialised memory.
	// If the whole buffer is un-initialised and then 'gl_buffer_sub_data' initialises the middle
	// part then even though the first part of the buffer is still un-initialised it is considered
	// initialised and only the last part of the buffer is considered un-initialised.
	if (d_uninitialised_offset < offset + size)
	{
		d_uninitialised_offset = offset + size;
	}
}


void
GPlatesOpenGL::GLBufferObject::gl_get_buffer_sub_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int size,
		void* data) const
{
	// Range must fit within existing buffer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			offset + size <= d_size,
			GPLATES_ASSERTION_SOURCE);

	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glGetBufferSubDataARB(target, offset, size, data);
}


GLvoid *
GPlatesOpenGL::GLBufferObject::gl_map_buffer_static(
		GLRenderer &renderer,
		target_type target,
		access_type access)
{
	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	GLvoid *mapped_data = glMapBufferARB(target, access);

	// We have to assume the entire buffer will be written to.
	d_uninitialised_offset = d_size;

	// If there was an error during mapping then report it and throw exception.
	if (mapped_data == NULL)
	{
		GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

		// We shouldn't get there since a mapped data pointer of NULL should generate an OpenGL error.
		// But if we do then throw an exception since we promised the caller they wouldn't have to check for NULL.
#ifdef GPLATES_DEBUG
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#else
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLBufferObject::gl_map_buffer_static: failed to map OpenGL buffer object.");
#endif
	}

	return mapped_data;
}


bool
GPlatesOpenGL::GLBufferObject::asynchronous_map_buffer_dynamic_supported(
		GLRenderer &renderer) const
{
	return GLEW_ARB_map_buffer_range || GLEW_APPLE_flush_buffer_range;
}


GLvoid *
GPlatesOpenGL::GLBufferObject::gl_map_buffer_dynamic(
		GLRenderer &renderer,
		target_type target)
{
	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	GLvoid *mapped_data = NULL;

	if (GLEW_ARB_map_buffer_range)
	{
		// We always map the entire buffer.
		// Only used for write access - otherwise caller should be using 'gl_map_buffer_static'.
		// 'GL_MAP_FLUSH_EXPLICIT_BIT' means one or more ranges of the buffer will need to be
		// explicitly flushed (using 'gl_flush_buffer_dynamic()').
		mapped_data = glMapBufferRange(target, 0, d_size, (GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
	}
	// Apple use a different (although similar) API.
	else if (GLEW_APPLE_flush_buffer_range)
	{
		// Prevent OpenGL from flushing the entire buffer.
		// One or more ranges of the buffer will need to be explicitly flushed
		// (using 'gl_flush_buffer_dynamic()').
		// NOTE: This is buffer object state (not global state) so it applies to the currently bound buffer object.
		glBufferParameteriAPPLE(target, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_FALSE);

		// Map the entire buffer.
		// Only used for write access - otherwise caller should be using 'gl_map_buffer_static'.
		mapped_data = glMapBufferARB(target, GL_WRITE_ONLY_ARB);

		// If the mapping failed for some reason then 'gl_unmap_buffer()' won't get called so
		// we should restore the default flushing behaviour before we return.
		if (mapped_data == NULL)
		{
			// Restore default flushing behaviour - which is to flush the entire buffer.
			// NOTE: This is buffer object state (not global state) so it applies to the currently bound buffer object.
			glBufferParameteriAPPLE(target, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_TRUE);
		}
	}
	else // We have no asynchronous API ...
	{
		// We have no way of telling OpenGL not to flush the entire buffer at 'unmap' so the OpenGL
		// driver might decide to block (eg, until the GPU finished reading the buffer) because
		// it thinks the entire buffer is getting modified and doesn't want to make a copy for us
		// to avoid blocking.
		// But the caller knows this because they checked with 'asynchronous_map_buffer_dynamic_supported()'.

		// Map the entire buffer.
		// Only used for write access - otherwise caller should be using 'gl_map_buffer_static'.
		mapped_data = glMapBufferARB(target, GL_WRITE_ONLY_ARB);
	}

	// If there was an error during mapping then report it and throw exception.
	if (mapped_data == NULL)
	{
		GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

		// We shouldn't get there since a mapped data pointer of NULL should generate an OpenGL error.
		// But if we do then throw an exception since we promised the caller they wouldn't have to check for NULL.
#ifdef GPLATES_DEBUG
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#else
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLBufferObject::gl_map_buffer_dynamic: failed to map OpenGL buffer object.");
#endif
	}

	return mapped_data;
}


void
GPlatesOpenGL::GLBufferObject::gl_flush_buffer_dynamic(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int length/*in bytes*/)
{
	// Range must fit within existing buffer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			offset + length <= d_size,
			GPLATES_ASSERTION_SOURCE);

	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	if (GLEW_ARB_map_buffer_range)
	{
		// Only flush the requested range.
		glFlushMappedBufferRange(target, offset, length);
	}
	// Apple use a different (although similar) API.
	else if (GLEW_APPLE_flush_buffer_range)
	{
		// Only flush the requested range.
		glFlushMappedBufferRangeAPPLE(target, offset, length);
	}
	else // We have no asynchronous API ...
	{
		// Nothing to do.
		// There was no API to disable flushing of the entire buffer which means the entire
		// buffer will get flushed at 'unmap' which means no explicitly flushing is necessary.
	}

	// If we're flushing/loading data that overlaps with the un-initialised region of the buffer then
	// update the offset into un-initialised memory.
	// There might be un-initialised memory before the flushed buffer range but we're ignoring that
	// and only considering un-initialised memory after all flushed ranges.
	if (d_uninitialised_offset < offset + length)
	{
		d_uninitialised_offset = offset + length;
	}
}


bool
GPlatesOpenGL::GLBufferObject::asynchronous_map_buffer_stream_supported(
		GLRenderer &renderer) const
{
	return GLEW_ARB_map_buffer_range || GLEW_APPLE_flush_buffer_range;
}


GLvoid *
GPlatesOpenGL::GLBufferObject::gl_map_buffer_stream(
		GLRenderer &renderer,
		target_type target,
		unsigned int minimum_bytes_to_stream,
		unsigned int &stream_offset,
		unsigned int &stream_bytes_available)
{
	//PROFILE_FUNC();

	// 'minimum_bytes_to_stream' must be in the half-open range (0, d_size].
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			0 < minimum_bytes_to_stream && minimum_bytes_to_stream <= d_size,
			GPLATES_ASSERTION_SOURCE);

	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	// Discard the current buffer allocation if there's not enough un-initialised memory
	// at the end of the buffer.
	bool discard = false;
	if (d_uninitialised_offset + minimum_bytes_to_stream > d_size)
	{
		discard = true;
	}

	GLvoid *mapped_data = NULL;

	if (GLEW_ARB_map_buffer_range)
	{
		// Only used for write access - otherwise caller should be using 'gl_map_buffer_static'.
		// 'GL_MAP_FLUSH_EXPLICIT_BIT' means the buffer will need to be explicitly flushed
		// (using 'gl_flush_buffer_stream').
		GLbitfield range_access = (GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

		// We're either:
		//  1) discarding/orphaning the buffer to get a new buffer allocation of same size, or
		//  2) forgoing synchronisation because the caller is promising not to overwrite current buffer data.
		if (discard)
		{
			range_access |= GL_MAP_INVALIDATE_BUFFER_BIT;

			// Since we're invalidating the buffer we can consider the entire buffer un-initialised.
			d_uninitialised_offset = 0;

			// Notify clients that a buffer allocation has occurred.
			// We haven't really allocated a new buffer, like 'gl_buffer_data()', but we tell
			// clients we have in case some hardware needs to rebind the buffer objects.
			// This might be required for ATI hardware which seems to require a rebing when
			// 'gl_buffer_data()' is called (nVidia doesn't seem to require it).
			allocated_buffer();
		}
		else // client is going to write to un-initialised memory in current buffer...
		{
			// This stops OpenGL from blocking (otherwise the GPU might block until it is finished
			// using any data currently in the buffer).
			range_access |= GL_MAP_UNSYNCHRONIZED_BIT;
		}

		// We only need to map the un-initialised region at the end of the buffer.
		mapped_data = glMapBufferRange(target, d_uninitialised_offset, d_size - d_uninitialised_offset, range_access);
	}
	// Apple use a different (although similar) API.
	else if (GLEW_APPLE_flush_buffer_range)
	{
		if (discard)
		{
			// If the usage is 'none' then 'gl_buffer_data' has never been called on this buffer
			// so it doesn't make sense to be mapping the buffer (there's no buffer allocation).
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					d_usage,
					GPLATES_ASSERTION_SOURCE);

			// Discard/orphan the current buffer to get a new buffer allocation of same size.
			// There's no way to do this using GL_APPLE_flush_buffer_range so we use the old way
			// of specifying a NULL data pointer (and use the same buffer size).
			//
			// NOTE: This will also set 'd_uninitialised_offset' to zero (because the data pointer is NULL).
			gl_buffer_data(renderer, target, d_size, NULL, d_usage.get());
		}
		else // client is going to write to un-initialised memory in current buffer...
		{
			// Forgo synchronisation because the client is going to write to un-initialised memory in
			// the buffer which does not require any synchronisation (because GPU can't be reading it).
			// Disable synchronisation temporarily while we map the current buffer - this prevents
			// OpenGL from blocking (until the GPU is finished using any data currently in the buffer).
			// NOTE: This is buffer object state (not global state) so it applies to the currently bound buffer object.
			glBufferParameteriAPPLE(target, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_FALSE);
		}

		// Prevent OpenGL from flushing the entire buffer.
		// The client will call 'gl_flush_buffer_stream' to flush the data they stream.
		// NOTE: This is buffer object state (not global state) so it applies to the currently bound buffer object.
		glBufferParameteriAPPLE(target, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_FALSE);

		// With the APPLE API we can only map the entire buffer.
		// Use write access - otherwise caller should be using 'gl_map_buffer_static'.
		mapped_data = glMapBufferARB(target, GL_WRITE_ONLY_ARB);
		if (mapped_data)
		{
			// Return pointer to start of un-initialised memory.
			mapped_data = static_cast<GLubyte *>(mapped_data) + d_uninitialised_offset;
		}

		if (!discard)
		{
			// Re-enable synchronisation so subsequent regular mapping operations (see 'gl_map_buffer_static')
			// will block until the GPU is finished processing previously submitted draw call (if necessary).
			// This is the default state.
			// NOTE: This is buffer object state (not global state) so it applies to the currently bound buffer object.
			glBufferParameteriAPPLE(target, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_TRUE);
		}
	}
	else // We have no asynchronous API ...
	{
		// The client is going to write to un-initialised memory in the current buffer but
		// we have no fine-grained API to tell OpenGL this - so instead of taking the blocking hit
		// we'll discard the current buffer if it contains *any* initialised memory (because
		// initialised memory means the GPU could still be reading it).
		if (d_uninitialised_offset != 0)
		{
			// If the usage is 'none' then 'gl_buffer_data' has never been called on this buffer
			// so it doesn't make sense to be mapping the buffer (there's no buffer allocation).
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					d_usage,
					GPLATES_ASSERTION_SOURCE);

			// Discard/orphan the current buffer to get a new buffer allocation of same size.
			// There's no way to do this so we use the old way of specifying a NULL data pointer
			// (and use the same buffer size).
			//
			// NOTE: This will also set 'd_uninitialised_offset' to zero (because the data pointer is NULL).
			gl_buffer_data(renderer, target, d_size, NULL, d_usage.get());
		}

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_uninitialised_offset == 0,
				GPLATES_ASSERTION_SOURCE);

		// We can only map the entire buffer.
		// Use write access - otherwise caller should be using 'gl_map_buffer_static'.
		//
		// NOTE: 'mapped_data' always points to the beginning of the buffer (because
		// 'd_uninitialised_offset' is always zero).
		mapped_data = glMapBufferARB(target, GL_WRITE_ONLY_ARB);
	}

	// If there was an error during mapping then report it and throw exception.
	if (mapped_data == NULL)
	{
		GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

		// We shouldn't get there since a mapped data pointer of NULL should generate an OpenGL error.
		// But if we do then throw an exception since we promised the caller they wouldn't have to check for NULL.
#ifdef GPLATES_DEBUG
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#else
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLBufferObject::gl_map_buffer_stream: failed to map OpenGL buffer object.");
#endif
	}

	// Initialise the caller's parameters.
	stream_offset = d_uninitialised_offset;
	stream_bytes_available = d_size - d_uninitialised_offset;

	return mapped_data;
}


void
GPlatesOpenGL::GLBufferObject::gl_flush_buffer_stream(
		GLRenderer &renderer,
		target_type target,
		unsigned int bytes_written)
{
	//PROFILE_FUNC();

	// If no data was written then return early.
	if (bytes_written == 0)
	{
		return;
	}

	// Bytes written must fit within existing buffer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_uninitialised_offset + bytes_written <= d_size,
			GPLATES_ASSERTION_SOURCE);

	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	if (GLEW_ARB_map_buffer_range)
	{
		// Only flush the requested range.
		//
		// Note that the offset is zero and not 'd_uninitialised_offset' since the mapped region
		// was not the entire buffer (only the un-initialised region at the end of the buffer).
		glFlushMappedBufferRange(target, 0, bytes_written);
	}
	// Apple use a different (although similar) API.
	else if (GLEW_APPLE_flush_buffer_range)
	{
		// Only flush the requested range.
		//
		// Note that the offset is 'd_uninitialised_offset' and not zero since the mapped region
		// was the entire buffer (and not just the un-initialised region at the end of the buffer).
		glFlushMappedBufferRangeAPPLE(target, d_uninitialised_offset, bytes_written);
	}
	else // We have no asynchronous API ...
	{
		// Nothing to do.
		// There was no API to disable flushing of the entire buffer which means the entire
		// buffer will get flushed at 'unmap' which means no explicitly flushing is necessary.
	}

	// Advance the offset into un-initialised memory.
	d_uninitialised_offset += bytes_written;
}


GLboolean
GPlatesOpenGL::GLBufferObject::gl_unmap_buffer(
		GLRenderer &renderer,
		target_type target)
{
	//PROFILE_FUNC();

	// Bind this buffer object.
	// Revert our buffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindBufferObjectAndApply save_restore_bind(
			renderer,
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	const GLboolean unmap_result = glUnmapBufferARB(target);

	if (GLEW_ARB_map_buffer_range)
	{
		// Nothing to do.
	}
	// Reset to the default flushing behaviour in case 'gl_map_buffer_dynamic' or 'gl_map_buffer_stream'
	// were called (and we're using the 'GL_APPLE_flush_buffer_range' extension).
	else if (GLEW_APPLE_flush_buffer_range)
	{
		// Restore default flushing behaviour - which is to flush the entire buffer.
		// NOTE: This is buffer object state (not global state) so it applies to the currently bound buffer object.
		glBufferParameteriAPPLE(target, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_TRUE);
	}

	// If the unmapping was unsuccessful...
	if (!unmap_result)
	{
		// Check OpenGL errors in case glUnmapBuffer used incorrectly - this will throw exception if so.
		GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

		// Otherwise the buffer contents have been corrupted.
		qWarning() << "GLBufferObject::gl_unmap_buffer: "
				"OpenGL buffer object contents have been corrupted (such as an ALT+TAB switch between applications).";
	}

	return unmap_result;
}
