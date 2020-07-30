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

#ifndef GPLATES_OPENGL_GL_H
#define GPLATES_OPENGL_GL_H

#include <opengl/OpenGL1.h>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLContext.h"
#include "GLState.h"
#include "GLStateStore.h"
#include "GLVertexArray.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Tracks OpenGL *global* state so it can be restored.
	 *
	 * Only those OpenGL calls that change the *global* state are routed through this class.
	 * All remaining OpenGL calls can be made directly through OpenGL.
	 *
	 * Some extra OpenGL calls (beyond tracking global state) are also routed through this class.
	 * For example, calls that set the state *inside* a vertex array object (hence not global state)
	 * are also routed through this class so that a single @a GLVertexArray instance can have one
	 * native vertex array object per OpenGL context. By tracking the state we can create a new
	 * native object when switching to another context and set its state to match. This is needed
	 * because vertex array objects (unlike buffer objects) cannot be shared across contexts.
	 */
	class GL :
			public GPlatesUtils::ReferenceCount<GL>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GL.
		typedef GPlatesUtils::non_null_intrusive_ptr<GL> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GL.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GL> non_null_ptr_to_const_type;


		/**
		 * RAII class to save the global state on entering a scope and restore on exiting the scope.
		 */
		class StateScope :
				private boost::noncopyable
		{
		public:
			explicit
			StateScope(
					GL &gl,
					bool reset_to_default_state = false);

			~StateScope();

			/**
			 * Opportunity to end the current state scope before the scope actually exits (when destructor is called).
			 */
			void
			end_scope();

		private:
			GL &d_gl;

			//! Snapshot of the OpenGL state on scope entry.
			GLState::shared_ptr_type d_entry_state;

			bool d_have_exited_scope;
		};


		/**
		 * Returns the @a GLContext passed into the constructor.
		 *
		 * Note that a shared pointer is not returned to avoid possibility of cycle shared references
		 * leading to memory leaks (@a GLContext owns a few resources which should not own it).
		 */
		GLContext &
		get_context() const
		{
			return *d_context;
		}


		/**
		 * Returns the OpenGL implementation-dependent capabilities and parameters.
		 */
		const GLCapabilities &
		get_capabilities() const
		{
			return d_context->get_capabilities();
		}


		//
		// The following methods are equivalent to the OpenGL functions with the same function name
		// (with a 'gl' prefix) and mostly the same function parameters.
		//
		// These methods are not documented.
		// To understand their usage please refer to the OpenGL 3.3 core profile specification.
		//
		// Any calls not provided below can be called with direct native OpenGL calls (with 'gl' prefix).
		//

		//! Bind vertex array object.
		void
		BindVertexArray(
				GPlatesOpenGL::GLVertexArray::shared_ptr_to_const_type array);

		//! Bind vertex array object to zero.
		void
		BindVertexArray();

		//! Bind buffer object target.
		void
		BindBuffer(
				GLenum target,
				GPlatesOpenGL::GLBuffer::shared_ptr_to_const_type buffer);

		//! Bind buffer object target to zero.
		void
		BindBuffer(
				GLenum target);

		void
		EnableVertexAttribArray(
				GLuint index);

		void
		DisableVertexAttribArray(
				GLuint index);


		void
		VertexAttribPointer(
				GLuint index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLvoid *pointer);

		void
		VertexAttribIPointer(
				GLuint index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLvoid *pointer);

	private:

		//! Constructor.
		GL(
				const GLContext::non_null_ptr_type &context,
				const GLStateStore::shared_ptr_type &state_store);


		/**
		 * Manages objects associated with the current OpenGL context.
		 */
		GLContext::non_null_ptr_type d_context;

		/**
		 * Used to efficiently allocate @a GLState objects.
		 */
		GLStateStore::shared_ptr_type d_state_store;

		/**
		 * Represents the default OpenGL state.
		 *
		 * Typically it won't have any states set which means it's the default state.
		 * One exception is the default viewport which represents the dimensions of the window
		 * currently attached to the OpenGL context.
		 *
		 * NOTE: This must be declared after @a d_state_set_store.
		 */
		GLState::shared_ptr_type d_default_state;
		
		/**
		 * Tracks the current OpenGL global state.
		 *
		 * NOTE: This must be declared after @a d_state_set_store.
		 */
		GLState::shared_ptr_type d_current_state;

	public: // For use by @a GLContext...

		/**
		 * Creates a @a GL object.
		 */
		static
		non_null_ptr_type
		create(
				const GLContext::non_null_ptr_type &context,
				const boost::shared_ptr<GLStateStore> &state_store)
		{
			return non_null_ptr_type(new GL(context, state_store));
		}
	};
}

#endif // GPLATES_OPENGL_GL_H
