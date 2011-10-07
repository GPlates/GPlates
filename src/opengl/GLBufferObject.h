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
			allocate();

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
		 * Note that @a gl_buffer_data must have been called and the sub-range must fit within it.
		 *
		 * This mirrors the behaviour of glBufferSubData.
		 * In particular note that NULL is a valid value for @a data (in which case it
		 * allocates storage but leaves it uninitialised).
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
		 * Maps the buffer for the specified access.
		 *
		 * This mirrors the behaviour of glMapBuffer.
		 * Returns NULL if buffer cannot be mapped or is already mapped.
		 */
		virtual
		GLvoid *
		gl_map_buffer(
				GLRenderer &renderer,
				target_type target,
				access_type access);


		/**
		 * Returns true if a sub-range of the buffer can be mapped.
		 *
		 * This is only supported if the GL_ARB_map_buffer_range extension is available.
		 *
		 * Only then can the methods @a gl_map_buffer_range and @a gl_flush_mapped_buffer_range be called.
		 */
		virtual
		bool
		is_map_buffer_range_supported() const;

		/**
		 * Maps the specified range of the buffer for the specified access.
		 *
		 * This mirrors the behaviour of glMapBufferRange.
		 * Returns NULL if buffer cannot be mapped or is already mapped.
		 *
		 * NOTE: This method is only supported if the GL_ARB_map_buffer_range extension is available.
		 *
		 * Check with @a is_map_buffer_range_supported to see if this method can be used.
		 */
		virtual
		void *
		gl_map_buffer_range(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int length,
				range_access_type access);

		/**
		 * Maps the specified range of the buffers.
		 *
		 * This mirrors the behaviour of glFlushMappedBufferRange.
		 *
		 * NOTE: This method is only supported if the GL_ARB_map_buffer_range extension is available.
		 *
		 * Check with @a is_map_buffer_range_supported to see if this method can be used.
		 */
		virtual
		void
		gl_flush_mapped_buffer_range(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int length);


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
	};
}

#endif // GPLATES_OPENGL_GLBUFFEROBJECT_H
