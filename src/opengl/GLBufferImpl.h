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
			return shared_ptr_type(create_as_auto_ptr(renderer).release());
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLBufferImpl>
		create_as_auto_ptr(
				GLRenderer &renderer)
		{
			return std::auto_ptr<GLBufferImpl>(new GLBufferImpl(renderer));
		}


		/**
		 * Returns the size, in bytes, of the current buffer as allocated by @a gl_buffer_data.
		 *
		 * NOTE: Initially the size is zero (until @a gl_buffer_data is first called).
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
		 * This mirrors the behaviour of glBufferData.
		 * In particular note that NULL is a valid value for @a data (in which case it
		 * allocates storage but leaves it uninitialised).
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
		 * Note that @a gl_buffer_data must have been called and the sub-range must fit within it.
		 *
		 * This mirrors the behaviour of glBufferSubData.
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
		 * Note that @a gl_buffer_data must have been called and the sub-range must fit within it.
		 *
		 * NOTE: @a data must be an existing array with a size of at least @a size bytes.
		 *
		 * This mirrors the behaviour of glGetBufferSubData.
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
		 * Maps the buffer for reading or writing.
		 *
		 * This mirrors the behaviour of glMapBuffer (except for access mode).
		 * Returns NULL if buffer cannot be mapped or is already mapped.
		 */
		virtual
		void *
		gl_map_buffer(
				GLRenderer &renderer,
				target_type target,
				access_type access)
		{
			return d_data.get();
		}


		/**
		 * Range access is supported.
		 *
		 * It's system memory that OpenGL blocks on access so there's no synchronisation or access issues.
		 */
		virtual
		bool
		is_map_buffer_range_supported() const
		{
			return true;
		}

		/**
		 * Maps the specified range of the buffer for the specified access.
		 *
		 * This mirrors the behaviour of glMapBufferRange.
		 * Returns NULL if buffer cannot be mapped or is already mapped.
		 */
		virtual
		void *
		gl_map_buffer_range(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int length,
				range_access_type range_access);

		/**
		 * Maps the specified range of the buffers.
		 *
		 * This mirrors the behaviour of glFlushMappedBufferRange.
		 */
		virtual
		void
		gl_flush_mapped_buffer_range(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int length)
		{
			// This is a no-op for system memory buffers that OpenGL blocks on access so
			// there's no synchronisation or access issues.
		}


		/**
		 * Unmaps the buffer mapped with @a gl_map_buffer - indicates updates or reading is complete.
		 *
		 * This mirrors the behaviour of glUnmapBuffer.
		 * Returns NULL if buffer cannot be mapped or is already mapped.
		 */
		virtual
		void
		gl_unmap_buffer(
				GLRenderer &renderer,
				target_type target)
		{
			// This is a no-op since it's just a system memory buffer and it's not being accessed by
			// the GPU (because any OpenGL calls referencing this memory will block and copy the array).
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
