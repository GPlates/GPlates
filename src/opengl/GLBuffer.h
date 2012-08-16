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
		static const target_type TARGET_PIXEL_UNPACK_BUFFER;   // When reading pixel data.
		static const target_type TARGET_PIXEL_PACK_BUFFER;     // When writing pixel data.


		//! Typedef for the usage of the buffer.
		typedef unsigned int usage_type;

		//
		// STATIC - You will specify the data only once (or possibly very rarely),
		// then use it many times without modifying it.
		//
		static const usage_type USAGE_STATIC_DRAW; // Application -> GL (data sent to the GPU)
		static const usage_type USAGE_STATIC_READ; // GL -> Application (data read from the GPU)
		static const usage_type USAGE_STATIC_COPY; // GL -> GL          (data copied within GPU)
		//
		// DYNAMIC - You will specify or modify the data repeatedly,
		// and use it repeatedly after each time you do this.
		// Such as modifying the data every few frames or so.
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
		 * Retrieves a sub-section of data from the existing array and copies it into @a data.
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
		 * Maps the buffer for static access.
		 *
		 * The returned pointer points to the beginning of the buffer.
		 *
		 * The reason it's called 'static' is the mapping operation will block in OpenGL if
		 * the GPU is currently accessing the buffer data (eg, for a draw call).
		 * To avoid the block you would first need to call 'glBufferData()' with a NULL data pointer
		 * to get a new buffer allocation (allows GPU to continue accessing the previous buffer contents
		 * while you write data to the new buffer allocation).
		 *
		 * So this method is probably best used if you're writing buffer data only once or
		 * infrequently enough that blocking is not a concern or first calling 'glBufferData()'
		 * with a NULL data pointer to avoid blocking (and then write the entire buffer).
		 *
		 * NOTE: Unlike @a gl_map_buffer_dynamic and @a gl_map_buffer_stream,
		 * @a gl_map_buffer_static is always supported.
		 *
		 * This mirrors the behaviour of glMapBuffer except that this method throws exception
		 * on error (instead of returning NULL) so you don't need to test for NULL.
		 * It also emits the relevant OpenGL warning message to the console.
		 */
		virtual
		GLvoid *
		gl_map_buffer_static(
				GLRenderer &renderer,
				target_type target,
				access_type access) = 0;


		/**
		 * Returns true if @a gl_map_buffer_dynamic can be called without blocking.
		 *
		 * Asynchronous mapping is supported if the GL_ARB_map_buffer_range or
		 * GL_APPLE_flush_buffer_range extension is available.
		 *
		 * Without asynchronous mapping support blocking can happen if the GPU is reading from the buffer when you map it.
		 * In this case @a gl_map_buffer_dynamic is the same as @a gl_map_buffer_static and you might
		 * be better off modifying your own system memory copy of the buffer data and re-submitting
		 * the entire data via @a gl_buffer_data each time you want to modify the data.
		 *
		 * NOTE: A return value of false does not prevent @a gl_map_buffer_dynamic from being called.
		 */
		virtual
		bool
		asynchronous_map_buffer_dynamic_supported(
				GLRenderer &renderer) const = 0;

		/**
		 * Maps the buffer for dynamic *write* access.
		 *
		 * The returned pointer points to the beginning of the buffer.
		 *
		 * The reason it's called 'dynamic' is it allows modification of *existing* buffer data without
		 * causing OpenGL to block (if the GPU is currently accessing the buffer data, eg, for a draw call).
		 * This is done by specifying sub-ranges within the mapped buffer that you have modified.
		 * This is useful if you want to retain the same data and only modify parts of it.
		 *
		 * Like @a gl_map_buffer_static this method throws exception on error so you don't need to test for NULL.
		 *
		 * NOTE: Check with @a asynchronous_map_buffer_dynamic_supported - if it returns false then
		 * @a gl_map_buffer_dynamic is the same as @a gl_map_buffer_static and you might be better off
		 * modifying your own system memory copy of the buffer data and re-submitting the entire
		 * via @a gl_buffer_data each time you want to modify the data.
		 * NOTE: If @a asynchronous_map_buffer_dynamic_supported returns false and you still decide
		 * to use @a gl_map_buffer_dynamic then you also still need to call @a gl_flush_buffer_dynamic.
		 */
		virtual
		GLvoid *
		gl_map_buffer_dynamic(
				GLRenderer &renderer,
				target_type target) = 0;

		/**
		 * Flushes a range of a currently mapped buffer (mapped via @a gl_map_buffer_dynamic).
		 *
		 * There can be multiple calls to @a gl_flush_buffer_dynamic between a
		 * @a gl_map_buffer_dynamic / @a gl_unmap_buffer pair where each call corresponds to
		 * modification of a different range within the buffer.
		 *
		 * NOTE: Call @a gl_flush_buffer_dynamic *after* each modification of a specified range.
		 * If a range of data is modified but not flushed then that data becomes undefined.
		 *
		 * NOTE: The buffer must currently be mapped with @a gl_map_buffer_dynamic before this can be called.
		 */
		virtual
		void
		gl_flush_buffer_dynamic(
				GLRenderer &renderer,
				target_type target,
				unsigned int offset,
				unsigned int length/*in bytes*/) = 0;


		/**
		 * Returns true if @a gl_map_buffer_stream has fine-grained asynchronous support.
		 *
		 * Asynchronous mapping is supported if the GL_ARB_map_buffer_range or
		 * GL_APPLE_flush_buffer_range extension is available.
		 *
		 * Without asynchronous mapping support each call to @a gl_map_buffer_stream will allocate
		 * a new buffer (of size determined by @a gl_buffer_data) in order to avoid blocking that
		 * can happen if the GPU is reading from the buffer when you map it.
		 * So there's still a coarse-grained level of asynchronous streaming support regardless.
		 * However if you have a large size buffer and only write a small amount of data during
		 * each mapping then this can use more memory (since the OpenGL driver allocates new buffers,
		 * behind the scenes, at each discard).
		 *
		 * NOTE: A return value of false does not prevent @a gl_map_buffer_stream from being called.
		 */
		virtual
		bool
		asynchronous_map_buffer_stream_supported(
				GLRenderer &renderer) const = 0;

		/**
		 * Maps the buffer for streaming *write* access.
		 *
		 * The returned pointer points to the area in the buffer at which streaming can begin -
		 * this is @a stream_offset bytes from the beginning of the buffer.
		 * @a stream_bytes_available is the number of bytes available for streaming.
		 *
		 * @a minimum_bytes_to_stream is the smallest amount of bytes that you will want to stream.
		 * If the internal buffer is too full to satisfy this request then the current buffer allocation
		 * is discarded and a new one of the same size (see @a gl_buffer_data) is allocated.
		 * @a minimum_bytes_to_stream must be in the half-open range (0, buffer_size] where
		 * 'buffer_size' is determined by @a gl_buffer_data.
		 *
		 * @a stream_alignment ensures the returned pointer is an integer multiple of @a stream_alignment
		 * bytes from the beginning of the internal buffer. This also means @a stream_offset will
		 * be an integer multiple of @a stream_alignment. This is useful for ensuring vertices are
		 * indexed correctly - in which case the alignment should be set to the size of the vertex.
		 *
		 * This call must be followed up with a single call to @a gl_flush_buffer_stream
		 * (before @a gl_unmap_buffer is called but after modifying the buffer) to specify the
		 * number of bytes that were streamed.
		 *
		 * This is a more flexible alternative to @a gl_map_buffer_static when streaming small amounts
		 * of data - normally with a large size buffer you need to orphan the previously rendered
		 * buffer (by calling 'glBufferData()' with a NULL data pointer) in order to avoid blocking
		 * to wait for the GPU to finish with the previous rendering call - however doing this frequently
		 * for a large buffer when only a small portion of it is actually used could lead to the
		 * OpenGL driver using excessive amounts of memory (or perhaps even just blocking) - actually
		 * this is what happens if @a asynchronous_map_buffer_stream_supported returns false.
		 * Internally this is implemented by allowing clients to modify only unused parts of the
		 * buffer (since the last time the buffer was orphaned) - this is done by incrementally filling
		 * up the buffer (and the client issuing draw calls) and then, when the buffer, is full simply
		 * orphaning/discarding it (the driver returns a new underlying buffer allocation) and do the
		 * same thing again.
		 * This is the way Direct3D does it (using D3DLOCK_NOOVERWRITE and D3DLOCK_DISCARD) -
		 * see http://msdn.microsoft.com/en-us/library/windows/desktop/bb147263%28v=vs.85%29.aspx#Using_Dynamic_Vertex_and_Index_Buffers
		 *
		 * Like @a gl_map_buffer_static this method throws exception on error.
		 */
		virtual
		GLvoid *
		gl_map_buffer_stream(
				GLRenderer &renderer,
				target_type target,
				unsigned int minimum_bytes_to_stream,
				unsigned int stream_alignment,
				unsigned int &stream_offset,
				unsigned int &stream_bytes_available) = 0;

		/**
		 * Specifies the number of bytes streamed after calling @a gl_map_buffer_stream and
		 * writing data into the region mapped for streaming.
		 *
		 * @a bytes_written is the number of bytes streamed into the region mapped by @a gl_map_buffer_stream.
		 *
		 * There should be only a single call to @a gl_flush_buffer_stream between a
		 * @a gl_map_buffer_stream / @a gl_unmap_buffer pair.
		 *
		 * NOTE: Call @a gl_flush_buffer_stream *after* modifying the region mapped for streaming.
		 * If the region mapped for streaming is modified but not flushed then that data becomes undefined.
		 *
		 * NOTE: The buffer must currently be mapped with @a gl_map_buffer_stream before this can be called.
		 *
		 * NOTE: If no data was streamed (written) after calling @a gl_map_buffer_stream then
		 * specifying zero for @a bytes_written is allowed since it makes it easier for the client code
		 * (@a gl_flush_buffer_stream effectively gets ignored in this case).
		 */
		virtual
		void
		gl_flush_buffer_stream(
				GLRenderer &renderer,
				target_type target,
				unsigned int bytes_written) = 0;


		/**
		 * Unmaps the buffer mapped with @a gl_map_buffer_static, @a gl_map_buffer_dynamic or
		 * @a gl_map_buffer_stream.
		 *
		 * This mirrors the behaviour of glUnmapBuffer (included returning GL_FALSE if the buffer
		 * contents were corrupted during the mapping, eg, video memory corruption during ALT+TAB).
		 * Also emits OpenGL error message as a warning to the console.
		 */
		virtual
		GLboolean
		gl_unmap_buffer(
				GLRenderer &renderer,
				target_type target) = 0;


		/**
		 * RAII class to call @a gl_map_buffer_static and @a gl_unmap_buffer over a scope.
		 */
		class MapBufferScope :
				private boost::noncopyable
		{
		public:
			/**
			 * Constructor - NOTE that it doesn't map the buffer - call @a gl_map_buffer_static for that.
			 */
			MapBufferScope(
					GLRenderer &renderer,
					GLBuffer &buffer,
					target_type target);

			/**
			 * Destructor - unmaps buffer if @a gl_unmap_buffer needs to be called but was not called -
			 * in which case the return code from @a gl_unmap_buffer is ignored.
			 */
			~MapBufferScope();

			//
			// @a gl_map_buffer_static and @a gl_unmap_buffer can be called multiple times (in matched non-nested pairs).
			//

			//! Maps buffer.
			GLvoid *
			gl_map_buffer_static(
					access_type access);

			//! Maps buffer for dynamic *write* access.
			GLvoid *
			gl_map_buffer_dynamic();

			//! Maps buffer for streaming *write* access.
			GLvoid *
			gl_map_buffer_stream(
					unsigned int minimum_bytes_to_stream,
					unsigned int stream_alignment,
					unsigned int &stream_offset,
					unsigned int &stream_bytes_available);


			//! Flushes a range of a currently mapped buffer (mapped via @a gl_map_buffer_dynamic).
			void
			gl_flush_buffer_dynamic(
					unsigned int offset,
					unsigned int length/*in bytes*/);

			/**
			 * Specifies the number of bytes streamed after calling @a gl_map_buffer_stream and
			 * writing data into the region mapped for streaming.
			 */
			void
			gl_flush_buffer_stream(
					unsigned int bytes_written);


			//! Unmaps buffer using parameters passed into constructor.
			GLboolean
			gl_unmap_buffer();

		private:
			GLRenderer &d_renderer;
			GLBuffer &d_buffer;
			target_type d_target;
			GLvoid *d_data;
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
