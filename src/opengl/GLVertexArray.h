/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_OPENGL_GLVERTEXARRAY_H
#define GPLATES_OPENGL_GLVERTEXARRAY_H

#include <memory> // For std::unique_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <opengl/OpenGL1.h>

#include "GLBuffer.h"
#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GL;
	class GLCapabilities;
	class GLContext;

	/**
	 * Wrapper around an OpenGL vertex array object.
	 *
	 * You can use an instance of this class freely across different OpenGL contexts (eg, globe and map views).
	 * Normally a vertex array object cannot be shared across OpenGL contexts, so this class internally
	 * creates a native vertex array object for each context that it encounters. It also remembers the
	 * vertex array state (such as vertex attribute arrays and buffer bindings) and sets it on each
	 * new native vertex array object (for each context encountered).
	 */
	class GLVertexArray :
			public boost::enable_shared_from_this<GLVertexArray>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLVertexArray.
		typedef boost::shared_ptr<GLVertexArray> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexArray> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexArray.
		typedef boost::weak_ptr<GLVertexArray> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexArray> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexArray object.
		 */
		static
		shared_ptr_type
		create(
				GL &gl)
		{
			return shared_ptr_type(new GLVertexArray(gl));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLVertexArray>
		create_unique(
				GL &gl)
		{
			return std::unique_ptr<GLVertexArray>(new GLVertexArray(gl));
		}


		/**
		 * Clears this vertex array.
		 *
		 * This includes the following:
		 *  - disable all attribute arrays
		 *  - set attribute divisor to zero
		 *  - unbind element array buffer
		 *  - unbind all attribute array buffers
		 *  - reset all attribute array parameters
		 *
		 * This method is useful when reusing a vertex array and you don't know what attribute
		 * arrays were previously enabled on the vertex array for example.
		 */
		void
		clear(
				GL &gl);


		/**
		 * Returns the vertex array resource handle associated with the current OpenGL context.
		 *
		 * Since vertex array objects cannot be shared across OpenGL contexts a separate
		 * vertex array object resource is created for each context encountered.
		 */
		GLuint
		get_resource_handle(
				GL &gl) const;

	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL vertex array objects.
		 */
		class Allocator
		{
		public:
			GLuint
			allocate(
					const GLCapabilities &capabilities);

			void
			deallocate(
					GLuint vertex_array_object);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<GLuint, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<GLuint, Allocator> resource_manager_type;


		/**
		 * Ensure the native vertex array object associated with the current OpenGL context has
		 * up-to-date internal state.
		 *
		 * It's possible the state of this vertex array was modified in a different context and
		 * hence a different native vertex array object was modified (there's a separate one for
		 * each context since they cannot be shared across contexts) and now we're in a different
		 * context so the native vertex array object of the current context must be updated to match.
		 *
		 * NOTE: This vertex array must currently be bound.
		 */
		void
		synchronise_current_context(
				GL &gl);

		//! Equivalent to glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, <buffer>) where none corresponds to zero.
		void
		bind_element_array_buffer(
				GL &gl,
				boost::optional<GLBuffer::shared_ptr_type> buffer);

		//! Equivalent to glEnableVertexAttribArray.
		void
		enable_vertex_attrib_array(
				GL &gl,
				GLuint index);

		//! Equivalent to glDisableVertexAttribArray.
		void
		disable_vertex_attrib_array(
				GL &gl,
				GLuint index);

		//! Equivalent to glVertexAttribDivisor.
		void
		vertex_attrib_divisor(
				GL &gl,
				GLuint index,
				GLuint divisor);

		//! Equivalent to glVertexAttribPointer.
		void
		vertex_attrib_pointer(
				GL &gl,
				GLuint index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				const GLvoid *pointer,
				boost::optional<GLBuffer::shared_ptr_type> array_buffer);

		//! Equivalent to glVertexAttribIPointer.
		void
		vertex_attrib_i_pointer(
				GL &gl,
				GLuint index,
				GLint size,
				GLenum type,
				GLsizei stride,
				const GLvoid *pointer,
				boost::optional<GLBuffer::shared_ptr_type> array_buffer);

	private:

		/**
		 * Keep track of the vertex array object state.
		 */
		struct ObjectState
		{
			struct AttributeArray
			{
				// Default state.
				AttributeArray() :
					enabled(GL_FALSE),
					divisor(0),
					size(4),
					type(GL_FLOAT),
					normalized(GL_FALSE),
					integer(GL_FALSE),
					stride(0),
					pointer(nullptr)
				{  }

				GLboolean enabled;  // True/false for glEnableVertexAttribArray/glDisableVertexAttribArray.
				GLuint divisor;
				GLint size;
				GLenum type;
				GLboolean normalized;  // Only applies to glVertexAttribPointer.
				GLboolean integer;  // True if applies to glVertexAttribIPointer (otherwise glVertexAttribPointer).
				GLsizei stride;
				const GLvoid *pointer;

				//! The array buffer bound to the attribute array (if any).
				boost::optional<GLBuffer::shared_ptr_type> array_buffer;
			};

			explicit
			ObjectState(
					GLuint max_attrib_arrays)
			{
				attribute_arrays.resize(max_attrib_arrays);
			}

			//! The vertex element buffer bound to this vertex array (if any).
			boost::optional<GLBuffer::shared_ptr_type> element_array_buffer;

			std::vector<AttributeArray> attribute_arrays;
		};

		/**
		 * The vertex array object state as currently set in each OpenGL context.
		 *
		 * Since vertex array objects cannot be shared across OpenGL contexts, in contrast to
		 * vertex buffer objects, we create a separate vertex array object for each context.
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
					GL &gl);

			/**
			 * The OpenGL context using our vertex array object.
			 *
			 * NOTE: This should *not* be a shared pointer otherwise it'll create a cyclic shared reference.
			 */
			const GLContext *context;

			//! The vertex array object resource created in a specific OpenGL context.
			resource_type::non_null_ptr_to_const_type resource;

			/**
			 * The current state of the native vertex array object in this OpenGL context.
			 *
			 * Note that this might be out-of-date if the native vertex array in another context has been
			 * updated and then we switched to this context (required this native object to be updated).
			 */
			ObjectState object_state;

			/**
			 * Determines if our context state needs updating.
			 */
			GPlatesUtils::ObserverToken object_state_observer;
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
		 * The vertex array object state set by the client.
		 *
		 * Before a native vertex array object can be used in a particular OpenGL context the state
		 * in that native object must match this state.
		 */
		ObjectState d_object_state;

		/**
		 * Subject token is invalidated when object state is updated, meaning all contexts need updating.
		 */
		GPlatesUtils::SubjectToken d_object_state_subject;


		//! Constructor.
		explicit
		GLVertexArray(
				GL &gl);

		ContextObjectState &
		get_object_state_for_current_context(
				GL &gl) const;

		ObjectState::AttributeArray &
		get_attribute_array(
				GLuint index);

		void
		update_attribute_array(
				GL &gl,
				GLuint index,
				const ObjectState::AttributeArray &attribute_array);
	};
}

#endif // GPLATES_OPENGL_GLVERTEXARRAY_H
