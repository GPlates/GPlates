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

#ifndef GPLATES_OPENGL_GLBUFFER_H
#define GPLATES_OPENGL_GLBUFFER_H

#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An abstraction based on an OpenGL object that supports the buffer object OpenGL extension
	 * - well it's actually the GL_ARB_vertex_buffer_object extension because its first use was for
	 * vertex buffers but it has since been extended to other objects (such as pixel buffers, texture buffers).
	 *
	 * If the extension is not supported then buffer objects are simulated.
	 */
	class GLBuffer :
			public boost::enable_shared_from_this<GLBuffer>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLBuffer.
		typedef boost::shared_ptr<GLBuffer> shared_ptr_type;
		typedef boost::shared_ptr<const GLBuffer> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLBuffer.
		typedef boost::weak_ptr<GLBuffer> weak_ptr_type;
		typedef boost::weak_ptr<const GLBuffer> weak_ptr_to_const_type;


		//! Typedef for a target of this buffer.
		typedef unsigned int target_type;

		//
		// The supported targets.
		//
		static const target_type TARGET_ARRAY_BUFFER;          // When reading/writing vertex attribute data (vertices).
		static const target_type TARGET_ELEMENT_ARRAY_BUFFER;  // When reading/writing vertex element data (indices).


		//! Typedef for the usage of the buffer.
		typedef unsigned int usage_type;

		//
		// STATIC - You will specify the data only once,
		// then use it many times without modifying it.
		//
		static const usage_type USAGE_STATIC_DRAW; // Application -> GL (data sent to the GPU)
		static const usage_type USAGE_STATIC_READ; // GL -> Application (data read from the GPU)
		static const usage_type USAGE_STATIC_COPY; // GL -> GL          (data copied within GPU)
		//
		// DYNAMIC - You will specify or modify the data repeatedly,
		// and use it repeatedly after each time you do this.
		//
		static const usage_type USAGE_DYNAMIC_DRAW; // Application -> GL (data sent to the GPU)
		static const usage_type USAGE_DYNAMIC_READ; // GL -> Application (data read from the GPU)
		static const usage_type USAGE_DYNAMIC_COPY; // GL -> GL          (data copied within GPU)
		//
		// STREAM - You will modify the data once, then use it once,
		// and repeat this process many times.
		//
		static const usage_type USAGE_STREAM_DRAW; // Application -> GL (data sent to the GPU)
		static const usage_type USAGE_STREAM_READ; // GL -> Application (data read from the GPU)
		static const usage_type USAGE_STREAM_COPY; // GL -> GL          (data copied within GPU)


		//! Typedef for regular mapped access to the buffer.
		typedef unsigned int access_type;

		static const access_type ACCESS_READ_ONLY;  // Indicates read-only access
		static const access_type ACCESS_WRITE_ONLY; // Indicates write-only access
		static const access_type ACCESS_READ_WRITE; // Indicates read-write access


		//! Typedef for mapped range access to the buffer.
		typedef unsigned int range_access_type;

		static const range_access_type RANGE_ACCESS_READ_BIT;
		static const range_access_type RANGE_ACCESS_WRITE_BIT;
		static const range_access_type RANGE_ACCESS_INVALIDATE_RANGE_BIT;
		static const range_access_type RANGE_ACCESS_INVALIDATE_BUFFER_BIT;
		static const range_access_type RANGE_ACCESS_FLUSH_EXPLICIT_BIT;
		static const range_access_type RANGE_ACCESS_UNSYNCHRONIZED_BIT;


		/**
		 * Creates a @a GLBuffer object with no array data.
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
		std::auto_ptr<GLBuffer>
		create_as_auto_ptr(
				GLRenderer &renderer);


		virtual
		~GLBuffer()
		{  }


		/**
		 * Returns the size, in bytes, of the current buffer as allocated by @a gl_buffer_data.
		 *
		 * NOTE: Initially the size is zero (until @a gl_buffer_data is first called).
		 */
		virtual
		unsigned int
		get_buffer_size() const = 0;


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
				usage_type usage) = 0;

		/**
		 * Similar to the other overload of @a gl_buffer_data but using a std::vector.
		 */
		template <typename ElementType>
		void
		gl_buffer_data(
				GLRenderer &renderer,
				target_type target,
				const std::vector<ElementType> &data,
				usage_type usage)
		{
			gl_buffer_data(renderer, target, data.size() * sizeof(ElementType), &data[0], usage);
		}


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
				const void* data) = 0;

		/**
		 * Similar to the other overload of @a gl_buffer_sub_data but using a std::vector.
		 */
		template <typename ElementType>
		void
		gl_buffer_sub_data(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				const std::vector<ElementType> &data)
		{
			gl_buffer_sub_data(renderer, target, offset, data.size() * sizeof(ElementType), &data[0]);
		}


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
				void* data) const = 0;


		/**
		 * Maps the buffer for the specified access.
		 *
		 * This mirrors the behaviour of glMapBuffer.
		 * Returns NULL if buffer cannot be mapped or is already mapped.
		 */
		virtual
		void *
		gl_map_buffer(
				GLRenderer &renderer,
				target_type target,
				access_type access) = 0;


		/**
		 * Returns true if a sub-range of the buffer can be mapped.
		 *
		 * This is only supported if the GL_ARB_map_buffer_range extension is available
		 * (or if GL_ARB_vertex_buffer_objects are *not* supported and instead simulated with
		 * system memory that OpenGL always blocks on).
		 *
		 * Only then can the methods @a gl_map_buffer_range and @a gl_flush_mapped_buffer_range be called.
		 */
		virtual
		bool
		is_map_buffer_range_supported() const = 0;

		/**
		 * Maps the specified range of the buffer for the specified access.
		 *
		 * This mirrors the behaviour of glMapBufferRange.
		 * Returns NULL if buffer cannot be mapped or is already mapped.
		 *
		 * NOTE: This method is only supported if the GL_ARB_map_buffer_range extension is available
		 * (or if GL_ARB_vertex_buffer_objects are *not* supported and instead simulated with
		 * system memory that OpenGL always blocks on).
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
				range_access_type range_access) = 0;

		/**
		 * Maps the specified range of the buffers.
		 *
		 * This mirrors the behaviour of glFlushMappedBufferRange.
		 *
		 * NOTE: This method is only supported if the GL_ARB_map_buffer_range extension is available
		 * (or if GL_ARB_vertex_buffer_objects are *not* supported and instead simulated with
		 * system memory that OpenGL always blocks on).
		 *
		 * Check with @a is_map_buffer_range_supported to see if this method can be used.
		 */
		virtual
		void
		gl_flush_mapped_buffer_range(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int length) = 0;


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
				target_type target) = 0;

		/**
		 * RAII class to call @a gl_map_buffer and @a gl_unmap_buffer over a scope.
		 */
		class MapBufferScope :
				private boost::noncopyable
		{
		public:
			/**
			 * Constructor - NOTE that it doesn't map the buffer - call @a gl_map_buffer for that.
			 */
			MapBufferScope(
					GLRenderer &renderer,
					GLBuffer &buffer,
					target_type target,
					access_type access);

			//! Destructor - unmaps buffer if @a gl_unmap_buffer needs to be called but was not called.
			~MapBufferScope();

			//
			// @a gl_map_buffer and @a gl_unmap_buffer can be called multiple times (in matched non-nested pairs).
			//

			//! Maps buffer using parameters passed into constructor.
			void *
			gl_map_buffer();

			//! Unmaps buffer using parameters passed into constructor.
			void
			gl_unmap_buffer();

		private:
			GLRenderer &d_renderer;
			GLBuffer &d_buffer;
			target_type d_target;
			access_type d_access;
			void *d_data;
		};


		/////////////////////////////////////////////////////////////
		//                                                         //
		// The following are support for the render framework only //
		//                                                         //
		/////////////////////////////////////////////////////////////


		//! Typedef for an observer of buffer allocations.
		typedef GPlatesUtils::ObserverToken buffer_allocation_observer_type;


		/**
		 * Returns true if a buffer has been allocated (ie, is @a gl_buffer_data has been called)
		 * since @a buffer_allocation_observer was last passed to this method.
		 *
		 * NOTE: Only buffer allocations are considered here (this does *not* include)
		 * @a gl_buffer_sub_data or the buffer mapping methods since they operate on an existing
		 * (already allocated) buffer. The only method to allocate a new buffer is @a gl_buffer_data.
		 */
		bool
		has_buffer_been_allocated_since(
				const buffer_allocation_observer_type &buffer_allocation_observer) const
		{
			return !d_buffer_allocation_subject.is_observer_up_to_date(buffer_allocation_observer);
		}

		/**
		 * Updates the specified buffer allocation observer so that a call to
		 * @a has_buffer_been_allocated_since will subsequently return false.
		 */
		void
		update_buffer_allocation_observer(
				buffer_allocation_observer_type &buffer_allocation_observer) const
		{
			d_buffer_allocation_subject.update_observer(buffer_allocation_observer);
		}

	protected:
		//! Derived classes can notify clients that a buffer allocation has occurred.
		void
		allocated_buffer()
		{
			d_buffer_allocation_subject.invalidate();
		}

	private:
		//! Typedef for a subject of buffer allocations.
		typedef GPlatesUtils::SubjectToken buffer_allocation_subject_type;

		//! Keeps track of buffer allocations (ie, calls to 'gl_buffer_data').
		buffer_allocation_subject_type d_buffer_allocation_subject;
	};
}

#endif // GPLATES_OPENGL_GLBUFFER_H
