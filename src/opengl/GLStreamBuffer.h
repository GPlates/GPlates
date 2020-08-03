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

#ifndef GPLATES_OPENGL_GLSTREAMBUFFER_H
#define GPLATES_OPENGL_GLSTREAMBUFFER_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "opengl/OpenGL1.h"

#include "GLBuffer.h"


namespace GPlatesOpenGL
{
	/**
	 * Wrapper around @a GLBuffer for streaming data from application to buffer object (from CPU to GPU).
	 */
	class GLStreamBuffer
	{
	public:

		/**
		 * RAII class to map and unmap a buffer over a scope.
		 */
		class MapScope :
				private boost::noncopyable
		{
		public:
			/**
			 * Constructor - NOTE that it doesn't map the buffer - call @a map for that.
			 *
			 * @a minimum_bytes_to_stream is the minimum number of bytes that @a map should return.
			 * It should be in the half-open range (0, <buffer size>].
			 * @a stream_alignment is typically the size of vertex (or vertex index for element array buffers).
			 *
			 * NOTE: The buffer, returned by 'stream_buffer.get_buffer()', should currently be bound on the
			 *       specified target (and remain bound for the duration of this scope).
			 *       If the buffer contains vertex elements (GL_ELEMENT_ARRAY_BUFFER target) then
			 *       this means the vertex array containing it should currently be bound.
			 */
			MapScope(
					GLenum target,
					GLStreamBuffer &stream_buffer,
					unsigned int minimum_bytes_to_stream,
					unsigned int stream_alignment);

			/**
			 * Destructor - unmaps buffer if @a unmap needs to be called but was not called.
			 */
			~MapScope();


			//
			// @a map and @a unmap can be called multiple times (in matched non-nested pairs).
			//

			/**
			 * Maps buffer and returns offset in buffer at start of mapped region and number of bytes mapped.
			 *
			 * Note: The returned pointer is non-NULL.
			 *       @throws OpenGLException if unable to map buffer.
			 */
			GLvoid *
			map(
					unsigned int &stream_offset,
					unsigned int &stream_bytes_available);

			/**
			 * Flush the specified number of bytes written after calling @a map and unmap the buffer.
			 *
			 * Note that number of bytes written can be less than mapped with @a map (and can be zero).
			 *
			 * Returns GL_FALSE if 'glUnmapBuffer()' returned false (internally), which usually
			 * happens when buffer contents become corrupted (due to a windowing event).
			 */
			GLboolean
			unmap(
					unsigned int bytes_written);

		private:
			GLenum d_target;
			GLStreamBuffer &d_stream_buffer;
			unsigned int d_minimum_bytes_to_stream;
			unsigned int d_stream_alignment;
			bool d_is_mapped;
		};


		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLStreamBuffer.
		typedef boost::shared_ptr<GLStreamBuffer> shared_ptr_type;
		typedef boost::shared_ptr<const GLStreamBuffer> shared_ptr_to_const_type;


		/**
		 * Creates a @a GLStreamBuffer object for streaming the specified buffer object (from CPU to GPU).
		 *
		 * A data store of @a buffer_size bytes is created (via glBufferData) in the specified buffer
		 * when it is first mapped.
		 *
		 * NOTE: The specified buffer should not be used for other purposes while it is being used here for streaming.
		 */
		static
		shared_ptr_type
		create(
				GLBuffer::shared_ptr_type buffer,
				unsigned int buffer_size)
		{
			return shared_ptr_type(new GLStreamBuffer(buffer, buffer_size));
		}


		/**
		 * Return the buffer we're streaming into.
		 */
		GLBuffer::shared_ptr_type
		get_buffer() const
		{
			return d_buffer;
		}

	private:

		GLBuffer::shared_ptr_type d_buffer;

		/**
		 * Number of bytes to allocate in buffer each time its data store is discarded.
		 */
		unsigned int d_buffer_size;

		/**
		 * Current offset into buffer where uninitialised memory is (memory that hasn't been yet
		 * been written to by the client).
		 *
		 * Streamed data is written into uninitialised memory (to avoid synchronisation issues with GPU).
		 *
		 * This is the first part of the current buffer that contains unwritten data.
		 * This is data that can be written to without interfering with data that the GPU
		 * might currently be reading (eg, due to a previous draw call).
		 */
		unsigned int d_uninitialised_offset;

		/**
		 * Is true when buffer data store has been created (via glBufferData).
		 */
		bool d_created_buffer_data_store;


		GLStreamBuffer(
				GLBuffer::shared_ptr_type buffer,
				unsigned int buffer_size);
	};
}

#endif // GPLATES_OPENGL_GLSTREAMBUFFER_H
