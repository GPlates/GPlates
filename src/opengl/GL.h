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

#include <memory>
#include <vector>
#include <opengl/OpenGL1.h>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLContext.h"
#include "GLFramebuffer.h"
#include "GLRenderbuffer.h"
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
	 * Another example, similar to @a GLVertexArray, is @a GLFramebuffer.
	 */
	class GL :
			public GPlatesUtils::ReferenceCount<GL>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GL.
		typedef GPlatesUtils::non_null_intrusive_ptr<GL> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GL.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GL> non_null_ptr_to_const_type;


		class StateScope;

		/**
		 * RAII class to scope all rendering calls (in this class).
		 *
		 * All @a GL calls should be done inside this scope.
		 *
		 * NOTE: OpenGL must be in the default state before entering this scope.
		 *       On exiting this scope the default OpenGL state is restored.
		 *
		 * On entering this scope the viewport/scissor rectangle is set to the current dimensions
		 * (in device pixels) of the framebuffer currently attached to the OpenGL context.
		 * And this is then considered the default viewport for the current rendering scope.
		 * Note that the viewport dimensions can change when the window (attached to context) is resized,
		 * so the default viewport can be different from one render scope to the next.
		 */
		class RenderScope :
				private boost::noncopyable
		{
		public:
			explicit
			RenderScope(
					GL &gl);

			/**
			 * Exit the current rendering scope (unless @a end has been called).
			 */
			~RenderScope();

			/**
			 * Opportunity to exit the current scope (before destructor is called).
			 */
			void
			end();

		private:
			GL &d_gl;
			std::unique_ptr<StateScope> d_state_scope;
			bool d_have_ended;
		};


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
			return d_capabilities;
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

		//! Bind framebuffer target to framebuffer object (none means bind to default framebuffer).
		void
		BindFramebuffer(
				GLenum target,
				boost::optional<GLFramebuffer::shared_ptr_type> framebuffer);

		//! Bind renderbuffer target (must be GL_RENDERBUFFER) to renderbuffer object (none means unbind).
		void
		BindRenderbuffer(
				GLenum target,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer);

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
		ClampColor(
				GLenum target,
				GLenum clamp);

		void
		ClearColor(
				GLclampf red = GLclampf(0.0),
				GLclampf green = GLclampf(0.0),
				GLclampf blue = GLclampf(0.0),
				GLclampf alpha = GLclampf(0.0));

		void
		ClearDepth(
				GLclampd depth = GLclampd(1.0));

		void
		ColorMask(
				GLboolean red = GL_TRUE,
				GLboolean green = GL_TRUE,
				GLboolean blue = GL_TRUE,
				GLboolean alpha = GL_TRUE);

		void
		ColorMaski(
				GLuint buf,
				GLboolean red = GL_TRUE,
				GLboolean green = GL_TRUE,
				GLboolean blue = GL_TRUE,
				GLboolean alpha = GL_TRUE);

		void
		ClearStencil(
				GLint stencil = 0);

		void
		CullFace(
				GLenum mode = GL_BACK);

		void
		DepthMask(
				GLboolean flag = GL_TRUE);

		void
		DepthRange(
				GLclampd n = 0.0,
				GLclampd f = 1.0);

		void
		Disable(
				GLenum cap);

		void
		DisableVertexAttribArray(
				GLuint index);

		void
		DrawBuffer(
				GLenum buf);

		void
		DrawBuffers(
				const std::vector<GLenum> &bufs);

		void
		Enable(
				GLenum cap);

		void
		EnableVertexAttribArray(
				GLuint index);

		void
		FramebufferRenderbuffer(
				GLenum target,
				GLenum attachment,
				GLenum renderbuffertarget,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer);

		void
		FramebufferTexture(
				GL &gl,
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		void
		FramebufferTexture1D(
				GL &gl,
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		void
		FramebufferTexture2D(
				GL &gl,
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		void
		FramebufferTexture3D(
				GL &gl,
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level,
				GLint layer);

		void
		FramebufferTextureLayer(
				GL &gl,
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level,
				GLint layer);

		void
		FrontFace(
				GLenum dir = GL_CCW);

		void
		Hint(
				GLenum target,
				GLenum hint);

		void
		LineWidth(
				GLfloat width = GLfloat(1));

		void
		PointSize(
				GLfloat size = GLfloat(1));

		void
		PolygonMode(
				GLenum face = GL_FRONT_AND_BACK,
				GLenum mode = GL_FILL);

		void
		PolygonOffset(
				GLfloat factor = GLfloat(0),
				GLfloat units = GLfloat(0));

		void
		PrimitiveRestartIndex(
				GLuint index);

		void
		ReadBuffer(
				GLenum src);

		void
		Scissor(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);

		void
		StencilMask(
				GLuint mask = ~0/*all ones*/);

		void
		StencilMaskSeparate(
				GLenum face,
				GLuint mask = ~0/*all ones*/);

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
		 * Context capabilities.
		 */
		const GLCapabilities &d_capabilities;
		
		/**
		 * Tracks the current OpenGL global state.
		 *
		 * NOTE: This must be declared after @a d_state_set_store.
		 */
		GLState::non_null_ptr_type d_current_state;

		/**
		 * The default viewport can change when the window (that context is attached to) is resized.
		 */
		GLViewport d_default_viewport;

		/**
		 * The default read and draw buffer in default framebuffer
		 * (GL_FRONT if there is no back buffer, otherwise GL_BACK).
		 */
		GLenum d_default_draw_read_buffer;
	};
}

#endif // GPLATES_OPENGL_GL_H
