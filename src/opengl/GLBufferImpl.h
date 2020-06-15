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

#ifndef GPLATES_OPENGL_GLBUFFERIMPL_H
#define GPLATES_OPENGL_GLBUFFERIMPL_H

#include <vector>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLBuffer.h"


namespace GPlatesOpenGL
{
	/**
	 * An implementation of the OpenGL object that supports the buffer object OpenGL extension
	 * - well it's actually the GL_ARB_vertex_buffer_object extension because its first use was for
	 * vertex buffers but it has since been extended to other objects (such as pixel buffers, texture buffers).
	 *
	 * This implementation is used if the extension is not supported - in which case buffer objects
	 * are simulated by using client-side memory arrays in a base OpenGL 1.1 way.
	 */
	class GLBufferImpl :
			public GLBuffer
	{
	public:
		//! A convenience typedef for a shared pointer to a @a GLBufferImpl.
		typedef boost::shared_ptr<GLBufferImpl> shared_ptr_type;
		typedef boost::shared_ptr<const GLBufferImpl> shared_ptr_to_const_type;


		/**
		 * Creates a @a GLBufferImpl object with no array data.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer)
		{
			return shared_ptr_type(create_as_unique_ptr(renderer).release());
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLBufferImpl>
		create_as_unique_ptr(
				GLRenderer &renderer)
		{
			return std::unique_ptr<GLBufferImpl>(new GLBufferImpl(renderer));
		}


		/**
		 * Returns the size, in bytes, of the current buffer as allocated by @a gl_buffer_data.
		 *
		 * See base class interface for more details.
		 */
		virtual
		unsigned int
		get_buffer_size() const
		{
			return d_size;
		}


		/**
		 * Specifies a new buffer of data.
		 *
		 * See base class interface for more details.
		 */
		virtual
		void
		gl_buffer_data(
				GLRenderer &renderer,
				target_type target,
				unsigned int size,
				const void *data,
				usage_type usage);


		/**
		 * Specifies a new sub-section of data in the existing array.
		 *
		 * See base class interface for more details.
		 */
		virtual
		void
		gl_buffer_sub_data(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int size,
				const void* data);


		/**
		 * Retrieves a sub-section of data from the existing array and copies it into.
		 *
		 * See base class interface for more details.
		 */
		virtual
		void
		gl_get_buffer_sub_data(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int size,
				void* data) const;


		/**
		 * Maps the buffer for static access.
		 *
		 * See base class interface for more details.
		 */
		virtual
		GLvoid *
		gl_map_buffer_static(
				GLRenderer &renderer,
				target_type target,
				access_type access)
		{
			return d_data.get();
		}


		/**
		 * Returns true if @a gl_map_buffer_dynamic can be called without blocking.
		 *
		 * Calling @a gl_map_buffer_dynamic does not result in blocking.
		 *
		 * See base class interface for more details.
		 */
		virtual
		bool
		asynchronous_map_buffer_dynamic_supported(
				GLRenderer &renderer) const
		{
			return true;
		}

		/**
		 * Maps the buffer for dynamic *write* access.
		 *
		 * See base class interface for more details.
		 */
		virtual
		GLvoid *
		gl_map_buffer_dynamic(
				GLRenderer &renderer,
				target_type target)
		{
			return d_data.get();
		}

		/**
		 * Flushes a range of a currently mapped buffer (mapped via @a gl_map_buffer_dynamic).
		 *
		 * See base class interface for more details.
		 */
		virtual
		void
		gl_flush_buffer_dynamic(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int length/*in bytes*/)
		{
			// This is a no-op since it's just a system memory buffer and it's not being accessed by
			// the GPU (because any OpenGL calls referencing this memory will block and copy the array).
		}


		/**
		 * Returns true if @a gl_map_buffer_stream has fine-grained asynchronous support.
		 *
		 * Calling @a gl_map_buffer_stream does not result in blocking.
		 *
		 * See base class interface for more details.
		 */
		virtual
		bool
		asynchronous_map_buffer_stream_supported(
				GLRenderer &renderer) const
		{
			return true;
		}

		/**
		 * Maps the buffer for streaming *write* access.
		 *
		 * See base class interface for more details.
		 */
		virtual
		GLvoid *
		gl_map_buffer_stream(
				GLRenderer &renderer,
				target_type target,
				unsigned int minimum_bytes_to_stream,
				unsigned int stream_alignment,
				unsigned int &stream_offset,
				unsigned int &stream_bytes_available)
		{
			// OpenGL vertex arrays copy when dereferencing data during draw calls so no synchronisation issues.
			// The entire buffer is always available.
			stream_offset = 0;
			stream_bytes_available = d_size;

			return d_data.get();
		}

		/**
		 * Specifies the number of bytes streamed after calling @a gl_map_buffer_stream and
		 * writing data into the region mapped for streaming.
		 *
		 * See base class interface for more details.
		 */
		virtual
		void
		gl_flush_buffer_stream(
				GLRenderer &renderer,
				target_type target,
				unsigned int bytes_written)
		{
			// This is a no-op.
			// OpenGL vertex arrays copy when dereferencing data during draw calls so no synchronisation issues.
		}


		/**
		 * Unmaps the buffer mapped with @a gl_map_buffer_static, @a gl_map_buffer_dynamic or
		 * @a gl_map_buffer_stream.
		 *
		 * See base class interface for more details.
		 */
		virtual
		GLboolean
		gl_unmap_buffer(
				GLRenderer &renderer,
				target_type target)
		{
			// This is a no-op since it's just a system memory buffer and it's not being accessed by
			// the GPU (because any OpenGL calls referencing this memory will block and copy the array).
			return GL_TRUE;
		}


		/**
		 * Implementation function accessed by buffer implementation target types.
		 *
		 * Returns pointer to current internal buffer.
		 *
		 * NOTE: Even though a raw pointer is returned instead of a shared pointer the data is kept
		 * alive until the renderer has submitted it to the GPU because the renderer retains a
		 * shared reference to 'this'.
		 */
		const GLubyte *
		get_buffer_resource() const
		{
			return d_data.get();
		}

		/**
		 * The 'non-const' version of @a get_buffer_resource.
		 */
		GLubyte *
		get_buffer_resource()
		{
			return d_data.get();
		}

	protected:
		GLBufferImpl(
				GLRenderer &renderer) :
			d_size(0)
		{  }

		boost::shared_array<GLubyte> d_data;
		unsigned int d_size;
	};
}

#endif // GPLATES_OPENGL_GLBUFFERIMPL_H
