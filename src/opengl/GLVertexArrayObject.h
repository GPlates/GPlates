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

#ifndef GPLATES_OPENGL_GLVERTEXARRAYOBJECT_H
#define GPLATES_OPENGL_GLVERTEXARRAYOBJECT_H

#include <bitset>
#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLVertexArray.h"
#include "GLVertexArrayImpl.h"
#include "GLVertexBuffer.h"
#include "GLVertexElementBuffer.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GLContext;
	class GLRenderer;

	/**
	 * An OpenGL object that encapsulates vertex array state.
	 *
	 * NOTE: Requires the GL_ARB_vertex_array_object extension.
	 *
	 * NOTE: While native vertex array objects in OpenGL cannot be shared across contexts,
	 * the @a GLVertexArrayObject wrapper can (because internally it creates a native vertex
	 * array object for each context that it encounters - that uses it).
	 * So you can freely use it (and the even higher-level wrapper @a GLVertexArray)
	 * in different OpenGL contexts.
	 */
	class GLVertexArrayObject :
			public GLVertexArray,
			public GLObject
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLVertexArrayObject.
		typedef boost::shared_ptr<GLVertexArrayObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexArrayObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexArrayObject.
		typedef boost::weak_ptr<GLVertexArrayObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexArrayObject> weak_ptr_to_const_type;


		//! Typedef for a resource handle.
		typedef GLuint resource_handle_type;

		/**
		 * Policy class to allocate and deallocate OpenGL vertex array objects.
		 */
		class Allocator
		{
		public:
			resource_handle_type
			allocate();

			void
			deallocate(
					resource_handle_type vertex_array_object);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<resource_handle_type, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<resource_handle_type, Allocator> resource_manager_type;


		/**
		 * Creates a shared pointer to a @a GLVertexArrayObject object.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer)
		{
			return shared_ptr_type(new GLVertexArrayObject(renderer));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLVertexArrayObject>
		create_as_auto_ptr(
				GLRenderer &renderer)
		{
			return std::auto_ptr<GLVertexArrayObject>(new GLVertexArrayObject(renderer));
		}


		/**
		 * Binds this vertex array so that all vertex data is sourced from the vertex buffers,
		 * specified in this interface, and vertex element data is sourced from the vertex element buffer.
		 *
		 * NOTE: Even if this vertex array object is currently bound when you call any of the
		 * 'set_...' methods below you'll still need to call @a gl_bind afterwards in order that
		 * those updates are reflected in the bound vertex array object.
		 */
		virtual
		void
		gl_bind(
				GLRenderer &renderer) const;


		/**
		 * Performs the equivalent of the OpenGL command 'glDrawRangeElements'.
		 *
		 * NOTE: If the GL_EXT_draw_range_elements extension is not present then reverts to
		 * using @a gl_draw_elements instead (ie, @a start and @a end are effectively ignored).
		 *
		 * @a indices_offset is a byte offset from the start of vertex element buffer set with
		 * @a set_vertex_element_buffer.
		 *
		 * This function can be more efficient for OpenGL than @a gl_draw_elements since
		 * you are guaranteeing that the range of vertex indices is bounded by [start, end].
		 *
		 * @throws @a PreconditionViolationError if a vertex element buffer has not been set
		 * with @a set_vertex_element_buffer.
		 */
		virtual
		void
		gl_draw_range_elements(
				GLRenderer &renderer,
				GLenum mode,
				GLuint start,
				GLuint end,
				GLsizei count,
				GLenum type,
				GLint indices_offset) const;


		/**
		 * Clears this vertex array.
		 */
		virtual
		void
		clear(
				GLRenderer &renderer);


		/**
		 * Specify the source of vertex element (vertex indices) data.
		 *
		 * NOTE: A vertex element buffer needs to be set before any drawing can occur.
		 */
		virtual
		void
		set_vertex_element_buffer(
				GLRenderer &renderer,
				const GLVertexElementBuffer::shared_ptr_to_const_type &vertex_element_buffer);


		/**
		 * Enables the specified (@a array) vertex array (in the fixed-function pipeline).
		 *
		 * These are the arrays selected for use by @a gl_bind.
		 * By default they are all disabled.
		 *
		 * @a array should be one of GL_VERTEX_ARRAY, GL_COLOR_ARRAY, or GL_NORMAL_ARRAY.
		 *
		 * NOTE: not including GL_INDEX_ARRAY since using RGBA mode (and not colour index mode)
		 * and not including GL_EDGE_FLAG_ARRAY for now.
		 */
		virtual
		void
		set_enable_client_state(
				GLRenderer &renderer,
				GLenum array,
				bool enable = true);

		/**
		 * Enables the vertex attribute array GL_TEXTURE_COORD_ARRAY (in the fixed-function pipeline)
		 * on the specified texture unit.
		 *
		 * By default the texture coordinate arrays for all texture units are disabled.
		 */
		virtual
		void
		set_enable_client_texture_state(
				GLRenderer &renderer,
				GLenum texture_unit,
				bool enable = true);


		/**
		 * Specify the source of vertex position data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_vertex_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Specify the source of vertex colour data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_color_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Specify the source of vertex normal data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_normal_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Specify the source of vertex texture coordinate data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_texture_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_tex_coord_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLenum texture_unit,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Enables the specified *generic* vertex attribute data at attribute index @a attribute_index.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' extension must be supported.
		 */
		virtual
		void
		set_enable_vertex_attrib_array(
				GLRenderer &renderer,
				GLuint attribute_index,
				bool enable);


		/**
		 * Specify the source of *generic* vertex attribute data at attribute index @a attribute_index.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' extension must be supported.
		 */
		virtual
		void
		set_vertex_attrib_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLint offset);


		/**
		 * Returns the vertex array resource handle (and current resource state) associated with the specified context.
		 *
		 * Since vertex array objects cannot be shared across OpenGL contexts a separate
		 * vertex array object resource is created for each context encountered.
		 *
		 * The returned resource state represents the current state stored in the
		 * vertex array resource as seen by the underlying OpenGL.
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		void
		get_vertex_array_resource(
				GLRenderer &renderer,
				resource_handle_type &resource_handle,
				boost::shared_ptr<GLState> &current_resource_state,
				boost::shared_ptr<const GLState> &target_resource_state) const;

	private:
		/**
		 * The vertex array object state as currently set in each OpenGL context.
		 *
		 * Since vertex array objects cannot be shared across OpenGL contexts, in contrast to
		 * vertex buffer object, we create a separate vertex array object for each context.
		 */
		struct ContextObjectState
		{
			/**
			 * Constructor creates a new vertex array object resource using the vertex array object
			 * manager of the specified context.
			 *
			 * If the vertex array object is destroyed then the resource will be queued for
			 * deallocation when this context is the active context and it is used for rendering.
			 */
			ContextObjectState(
					const GLContext &context_,
					GLRenderer &renderer_);

			/**
			 * The OpenGL context using our vertex array object.
			 *
			 * NOTE: This should *not* be a shared pointer otherwise it'll create a cyclic shared reference.
			 */
			const GLContext *context;

			//! The vertex array object resource created in a specific OpenGL context.
			resource_type::non_null_ptr_to_const_type resource;

			/**
			 * The current state of @a resource as currently known (or registered) in OpenGL.
			 *
			 * This is so when we bind the vertex array object (resource) in OpenGL we know what
			 * other buffer bindings and enable/disable client state it brings in with it.
			 * Vertex array objects are unlike other OpenGL objects in this way in that they are container objects.
			 */
			boost::shared_ptr<GLState> resource_state;
		};

		/**
		 * Typedef for a sequence of context object states.
		 *
		 * A 'vector' is fine since we're not expecting many OpenGL contexts so searches should be fast.
		 */
		typedef std::vector<ContextObjectState> context_object_state_seq_type;


		/**
		 * The vertex array object state for each context that we've encountered.
		 */
		mutable context_object_state_seq_type d_context_object_states;

		/**
		 * Object state as last set for the OpenGL context that @a resource was created in.
		 *
		 * Easiest way to do this is to re-use @a GLVertexArrayImpl.
		 */
		GLVertexArrayImpl::shared_ptr_type d_object_state;


		//! Constructor.
		explicit
		GLVertexArrayObject(
				GLRenderer &renderer);

		const ContextObjectState &
		get_object_state_for_current_context(
				GLRenderer &renderer) const;
	};
}

#endif // GPLATES_OPENGL_GLVERTEXARRAYOBJECT_H
