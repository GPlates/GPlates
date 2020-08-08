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
#include "GLTexture.h"
#include "GLVertexArray.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Tracks common OpenGL global context state so it can be automatically restored.
	 *
	 * Only those OpenGL calls that change the global context state are routed through this class.
	 * All remaining OpenGL calls should be made directly to OpenGL, including calls that change the
	 * state of OpenGL objects (such as vertex arrays, buffers, textures, programs, framebuffers, etc).
	 *
	 * Note that only those global context state calls that are commonly used are catered for here.
	 * Uncatered global context state calls will need to be made directly to OpenGL, and hence will
	 * have no save/restore ability provided by this class.
	 *
	 * Some extra OpenGL calls (beyond tracking global context state) are also routed through this class.
	 * For example, calls that set the state *inside* a vertex array object (hence not global state)
	 * are also routed through this class so that a single @a GLVertexArray instance can have one
	 * native vertex array object per OpenGL context. By tracking the state we can create a new
	 * native object when switching to another context and set its state to match. This is needed
	 * because vertex array objects (unlike buffer objects) cannot be shared across contexts.
	 * Note that the default attribute state set by glVertexAttrib* (such as glVertexAttrib4f) are
	 * actually global context state (they're not part of vertex array object state). Despite this they
	 * are an example of context state not catered for in this class (so will need direct calls to OpenGL).
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
			/**
			 * Save the current OpenGL state (so it can be restored on exiting the current scope).
			 *
			 * If @a reset_to_default_state is true then reset to the default OpenGL state after saving.
			 * This results in the default OpenGL state when entering the current scope.
			 * Note that this does not affect the state that is saved (and hence restored).
			 * By default it is not reset (to the default OpenGL state).
			 */
			explicit
			StateScope(
					GL &gl,
					bool reset_to_default_state = false);

			/**
			 * Restores the OpenGL state to what it was on entering the current scope
			 * (unless @a restore has been called).
			 */
			~StateScope();

			/**
			 * Opportunity to restore the OpenGL state before the scope actually exits (when destructor is called).
			 */
			void
			restore();

		private:
			GL &d_gl;
			bool d_have_restored;
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

		void
		ActiveTexture(
				GLenum active_texture);

		//! Bind buffer target to buffer object (none means unbind).
		void
		BindBuffer(
				GLenum target,
				boost::optional<GLBuffer::shared_ptr_type> buffer);

		//! Bind texture target and active texture unit to texture object (none means unbind).
		void
		BindTexture(
				GLenum texture_target,
				boost::optional<GLTexture::shared_ptr_type> texture_object);

		//! Bind vertex array object (none means unbind).
		void
		BindVertexArray(
				boost::optional<GLVertexArray::shared_ptr_type> vertex_array);

		void
		ClearColor(
				GLclampf red,
				GLclampf green,
				GLclampf blue,
				GLclampf alpha);

		void
		ClearDepth(
				GLclampd depth);

		void
		ColorMask(
				GLboolean red,
				GLboolean green,
				GLboolean blue,
				GLboolean alpha);

		void
		ClearStencil(
				GLint stencil);

		void
		CullFace(
				GLenum mode);

		void
		DepthMask(
				GLboolean flag);

		void
		DepthRange(
				GLclampd n,
				GLclampd f);

		void
		Disable(
				GLenum cap);

		void
		DisableVertexAttribArray(
				GLuint index);

		void
		Enable(
				GLenum cap);

		void
		EnableVertexAttribArray(
				GLuint index);

		void
		FrontFace(
				GLenum dir);

		void
		Hint(
				GLenum target,
				GLenum hint);

		void
		LineWidth(
				GLfloat width);

		void
		PointSize(
				GLfloat size);

		void
		PolygonMode(
				GLenum face,
				GLenum mode);

		void
		Scissor(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);

		void
		StencilMask(
				GLuint mask);

		void
		VertexAttribDivisor(
				GLuint index,
				GLuint divisor);

		void
		VertexAttribIPointer(
				GLuint index,
				GLint size,
				GLenum type,
				GLsizei stride,
				const GLvoid *pointer);

		void
		VertexAttribPointer(
				GLuint index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				const GLvoid *pointer);

		void
		Viewport(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);


	public: // For use by @a GLContext...

		/**
		 * Creates a @a GL object.
		 */
		static
		non_null_ptr_type
		create(
				const GLContext::non_null_ptr_type &context,
				const GLStateStore::non_null_ptr_type &state_store)
		{
			return non_null_ptr_type(new GL(context, state_store));
		}

	private:

		//! Constructor.
		GL(
				const GLContext::non_null_ptr_type &context,
				const GLStateStore::non_null_ptr_type &state_store);


		/**
		 * Manages objects associated with the current OpenGL context.
		 */
		GLContext::non_null_ptr_type d_context;
		
		/**
		 * Tracks the current OpenGL global state.
		 *
		 * NOTE: This must be declared after @a d_state_set_store.
		 */
		GLState::non_null_ptr_type d_current_state;
	};
}

#endif // GPLATES_OPENGL_GL_H
