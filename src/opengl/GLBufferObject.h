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

#ifndef GPLATES_OPENGL_GLBUFFEROBJECT_H
#define GPLATES_OPENGL_GLBUFFEROBJECT_H

#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLBuffer.h"
#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An OpenGL object that supports the buffer object OpenGL extension - well it's actually
	 * the GL_ARB_vertex_buffer_object extension because its first use was for vertex buffers but
	 * it has since been extended to other objects (such as pixel buffers, texture buffers).
	 *
	 * NOTE: @a GLFrameBufferObject is not a type of buffer object, despite its name, since it
	 * doesn't have the API or interface of a buffer object.
	 */
	class GLBufferObject :
			public GLBuffer,
			public GLObject
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLBufferObject.
		typedef boost::shared_ptr<GLBufferObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLBufferObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLBufferObject.
		typedef boost::weak_ptr<GLBufferObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLBufferObject> weak_ptr_to_const_type;


		//! Typedef for a resource handle.
		typedef GLuint resource_handle_type;

		/**
		 * Policy class to allocate and deallocate OpenGL buffer objects.
		 */
		class Allocator
		{
		public:
			resource_handle_type
			allocate(
					const GLCapabilities &capabilities);

			void
			deallocate(
					resource_handle_type buffer_object);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<resource_handle_type, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<resource_handle_type, Allocator> resource_manager_type;


		/**
		 * Creates a @a GLBufferObject object with no array data.
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
		std::auto_ptr<GLBufferObject>
		create_as_auto_ptr(
				GLRenderer &renderer)
		{
			return std::auto_ptr<GLBufferObject>(new GLBufferObject(renderer));
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
				const void* data,
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
		 * Maps the buffer for the specified access.
		 *
		 * See base class interface for more details.
		 */
		virtual
		GLvoid *
		gl_map_buffer_static(
				GLRenderer &renderer,
				target_type target,
				access_type access);


		/**
		 * Returns true if @a gl_map_buffer_dynamic can be called without blocking.
		 *
		 * See base class interface for more details.
		 */
		virtual
		bool
		asynchronous_map_buffer_dynamic_supported(
				GLRenderer &renderer) const;

		/**
		 * Maps the buffer for dynamic *write* access.
		 *
		 * See base class interface for more details.
		 */
		virtual
		GLvoid *
		gl_map_buffer_dynamic(
				GLRenderer &renderer,
				target_type target);

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
				unsigned int length/*in bytes*/);


		/**
		 * Returns true if @a gl_map_buffer_stream has fine-grained asynchronous support.
		 *
		 * See base class interface for more details.
		 */
		virtual
		bool
		asynchronous_map_buffer_stream_supported(
				GLRenderer &renderer) const;

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
				unsigned int &stream_bytes_available);

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
				unsigned int bytes_written);


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
				target_type target);


		/**
		 * Returns the buffer resource handle.
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		resource_handle_type
		get_buffer_resource_handle() const
		{
			return d_resource->get_resource_handle();
		}

	protected:
		//! Constructor.
		explicit
		GLBufferObject(
				GLRenderer &renderer);

	private:
		resource_type::non_null_ptr_to_const_type d_resource;
		unsigned int d_size;
		boost::optional<usage_type> d_usage;

		/**
		 * Current offset into buffer where uninitialised memory is (memory that hasn't been yet
		 * been written to by the client).
		 *
		 * This is used when streaming (using @a gl_map_buffer_stream) since streaming is
		 * written into uninitialised memory (to avoid synchronisation issues with GPU).
		 *
		 * This is the first part of the current buffer that contains unwritten data.
		 * This is data that can be written to without interfering with data that the GPU
		 * might currently be reading (eg, due to a previous draw call).
		 */
		unsigned int d_uninitialised_offset;
	};
}

#endif // GPLATES_OPENGL_GLBUFFEROBJECT_H
