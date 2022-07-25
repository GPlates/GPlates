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

#include <vector>
#include <opengl/OpenGL1.h>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLContext.h"
#include "GLFramebuffer.h"
#include "GLProgram.h"
#include "GLRenderbuffer.h"
#include "GLSampler.h"
#include "GLState.h"
#include "GLStateStore.h"
#include "GLTexture.h"
#include "GLVertexArray.h"
#include "GLViewport.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class OpenGLFunctions;

	/**
	 * Public interface for interacting with OpenGL, and tracking common OpenGL *global* state in an OpenGL context.
	 *
	 * Global OpenGL state differs from the state of OpenGL *objects* (such as vertex arrays, buffers, textures,
	 * programs, framebuffers, etc). Global state is things like what textures are bound to which texture units.
	 *
	 * The benefit to tracking *global* context state is that it can be automatically restored
	 * (see nested class @a StateScope below) without having to explicitly do it (eg, unbinding textures
	 * when finished drawing with them). This helps avoid bugs where the global state is not what is expected.
	 * The internal state of resource *objects* must still be explicitly managed but that is usually easier
	 * since a single resource object is typically managed by a single class/module whereas the global OpenGL state
	 * is used across the entire application (and therefore harder to manage explicitly).
	 *
	 * The main difference between this class and the lower-level class @a OpenGLFunctions is the latter
	 * only provides access to the native OpenGL functions (via Qt) and is only used to help implement the
	 * machinery that supports this class (which includes the resource classes like @a GLTexture).
	 * Hence functions in @a OpenGLFunctions should not be called by users. They should instead use this
	 * class @a GL along with the OpenGL resource classes that manage OpenGL resources (such as using
	 * @a GLTexture to manage a texture object).
	 *
	 * Note: The only OpenGL function calls that are catered for here (and in the resource classes @a GLProgram, etc)
	 *       are those that are currently used by GPlates. If you need to call an OpenGL function not catered for here
	 *       (and not in a resource class) then it'll need to be added. And if it sets global state then it will
	 *       need to include global state tracking (in this class).
	 *
	 * Note: This class also tracks some resource *object* state (ie, not just *global* context state) but
	 *       only for the purpose described next (ie, not for the purpose of automatically saving/restoring it).
	 *       For example, calls that set the state *inside* a vertex array object (hence not global state)
	 *       are tracked by this class so that a single @a GLVertexArray instance can have one native
	 *       vertex array object per OpenGL context. By tracking the *object* state we can create a new
	 *       native object when switching to another OpenGL context (ie, when using an instance of @a GL
	 *       that refers to a different context) and set its object state to match. This is needed
	 *       because vertex array objects (unlike buffer objects) cannot be shared across contexts.
	 *       Another example, similar to @a GLVertexArray, is @a GLFramebuffer.
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
			bool d_have_ended;
		};


		/**
		 * RAII class to save the *global* state on entering a scope and restore on exiting the scope.
		 *
		 * Note: The *internal* state of resource objects is not saved/restored.
		 */
		class StateScope :
				private boost::noncopyable
		{
		public:
			/**
			 * Save the current OpenGL global state (so it can be restored on exiting the current scope).
			 *
			 * If @a reset_to_default_state is true then reset to the default OpenGL global state after saving.
			 * This results in the default OpenGL global state when entering the current scope.
			 * Note that this does not affect the global state that is saved (and hence restored).
			 * By default it is not reset (to the default OpenGL global state).
			 */
			explicit
			StateScope(
					GL &gl,
					bool reset_to_default_state = false);

			/**
			 * Restores the OpenGL global state to what it was on entering the current scope
			 * (unless @a restore has been called).
			 */
			~StateScope();

			/**
			 * Opportunity to restore the OpenGL global state before the scope actually exits (when destructor is called).
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
		 * Note that a shared pointer is not returned to avoid possibility of cyclic shared references
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


		//! Version 3.3 core is always supported (it's our minimum requirement).
		bool
		supports_3_3_core() const
		{
			return true;
		}

		//! Returns true if we have OpenGL 4.0 or greater.
		bool
		supports_4_0_core() const;

		//! Returns true if we have OpenGL 4.1 or greater.
		bool
		supports_4_1_core() const;

		//! Returns true if we have OpenGL 4.2 or greater.
		bool
		supports_4_2_core() const;

		//! Returns true if we have OpenGL 4.3 or greater.
		bool
		supports_4_3_core() const;


		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Set OpenGL state methods.
		//
		// The following methods are equivalent to the native OpenGL functions with the same function name
		// (ie, with a 'gl' prefix). But we exclude the 'gl' prefix so that, for example, a function call to
		// 'ActiveTexture()' on object 'gl' looks like 'gl.ActiveTexture()' which looks very similar to the
		// native equivalent 'glActiveTexture()'.
		//
		// There are three categories of methods that set OpenGL state:
		// 1) Methods that create/delete OpenGL resources (such as textures).
		//    That functionality is handled *outside* this class using our own OpenGL resource classes that
		//    manage those resources (such as using @a GLTexture to manage a texture object).
		// 2) Methods that manipulate *global* state not related to the *internal* state of resources *objects* (or their creation).
		//    That functionality is handled by this class, and results in some of those methods having slightly different parameters
		//    than their equivalent native OpenGL functions to account for resource classes (such as @a GLTexture),
		//    usually accepting the resource *class* as an argument rather than accepting an *integer* handle (native resource).
		// 3) All other methods (that do NOT set *global* state and do NOT create/delete OpenGL resources).
		//    That functionality is handled by this class and includes methods that manipulate the *internal* state of resource objects
		//    (such as 'Uniform4f()') and drawing commands (such as 'DrawArrays()'). However some functionality has moved to the
		//    resource classes if was more convenient there (eg, @a GLProgram has methods to link a program and get a uniform location
		//    since it wraps those with extra functionality for convenience, such as caching the location of uniforms upon querying them).
		//
		// Note: OpenGL calls for item (1) should use resource classes (like @a GLTexture).
		//       All other OpenGL calls (ie, items (2) and (3) above) should go through this class (and in some cases the resource classes).
		//       The lower-level class @a OpenGLFunctions only exists to support the implementation of this class (and the resource classes)
		//       and should not be used outside of that implementation.
		//
		// Note: The *internal* state of resource objects is not saved/restored by @a StateScope.
		//       Only the *global* state for item (2) above is saved/restored.
		//
		// Note: These methods are not documented here, to understand their usage please refer to the
		//       core profile specifications for OpenGL 3.3 (and above).
		//
		// As mentioned above, some extra OpenGL calls (beyond tracking global context state) are also
		// routed through this class. For example, calls that set the state *inside* a vertex array object
		// (or a framebuffer object) are object state (not global state) but are nevertheless routed through
		// this class so that a single @a GLVertexArray instance (or a @a GLFramebuffer instance) can have
		// one native vertex array object (or framebuffer object) per OpenGL context.
		////////////////////////////////////////////////////////////////////////////////////////////////////////


		////////////////////////////////////////
		//                                    //
		// Functions in OpenGL 1.0 - 3.3 core //
		//                                    //
		////////////////////////////////////////


		//
		//
		// GLOBAL state (saved/restored by @a StateScope)
		//
		//

		void
		ActiveTexture(
				GLenum active_texture);

		//! Bind buffer target to buffer object (none means unbind).
		void
		BindBuffer(
				GLenum target,
				boost::optional<GLBuffer::shared_ptr_type> buffer);

		//! Bind entire buffer object to indexed target (none means unbind).
		void
		BindBufferBase(
				GLenum target,
				GLuint index,
				boost::optional<GLBuffer::shared_ptr_type> buffer);

		//! Bind sub-range of buffer object to indexed target (none means unbind).
		void
		BindBufferRange(
				GLenum target,
				GLuint index,
				boost::optional<GLBuffer::shared_ptr_type> buffer,
				GLintptr offset,
				GLsizeiptr size);

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

		//! Bind texture unit to sampler object (none means unbind).
		void
		BindSampler(
				GLuint unit,
				boost::optional<GLSampler::shared_ptr_type> sampler);

		//! Bind texture target and active texture unit to texture object (none means unbind).
		void
		BindTexture(
				GLenum texture_target,
				boost::optional<GLTexture::shared_ptr_type> texture);

		//! Bind vertex array object (none means unbind).
		void
		BindVertexArray(
				boost::optional<GLVertexArray::shared_ptr_type> vertex_array);

		void
		BlendColor(
				GLclampf red = GLclampf(0.0),
				GLclampf green = GLclampf(0.0),
				GLclampf blue = GLclampf(0.0),
				GLclampf alpha = GLclampf(0.0));

		void
		BlendEquation(
				GLenum mode);

		void
		BlendEquationSeparate(
				GLenum mode_RGB,
				GLenum mode_alpha);

		void
		BlendFunc(
				GLenum src,
				GLenum dst);

		void
		BlendFuncSeparate(
				GLenum src_RGB,
				GLenum dst_RGB,
				GLenum src_alpha,
				GLenum dst_alpha);

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
		DepthFunc(
				GLenum func);

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
		Disablei(
				GLenum cap,
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
		Enablei(
				GLenum cap,
				GLuint index);

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
		PixelStoref(
				GLenum pname,
				GLfloat param);

		void
		PixelStorei(
				GLenum pname,
				GLint param);

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
		SampleCoverage(
				GLclampf value,
				GLboolean invert);

		void
		SampleMaski(
				GLuint mask_number,
				GLbitfield mask);

		/**
		 * Note that the default scissor rectangle is the current dimensions (in device pixels) of the framebuffer
		 * (either main framebuffer or a framebuffer object) currently attached to the OpenGL context.
		 */
		void
		Scissor(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);

		void
		StencilFunc(
				GLenum func,
				GLint ref,
				GLuint mask);

		void
		StencilFuncSeparate(
				GLenum face,
				GLenum func,
				GLint ref,
				GLuint mask);

		void
		StencilMask(
				GLuint mask = ~0/*all ones*/);

		void
		StencilMaskSeparate(
				GLenum face,
				GLuint mask = ~0/*all ones*/);

		void
		StencilOp(
				GLenum sfail,
				GLenum dpfail,
				GLenum dppass);

		void
		StencilOpSeparate(
				GLenum face,
				GLenum sfail,
				GLenum dpfail,
				GLenum dppass);

		//! Use the program object (none means don't use any program).
		void
		UseProgram(
				boost::optional<GLProgram::shared_ptr_type> program);

		/**
		 * Note that the default viewport rectangle is the current dimensions (in device pixels) of the framebuffer
		 * (either main framebuffer or a framebuffer object) currently attached to the OpenGL context.
		 */
		void
		Viewport(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);


		//
		//
		// OBJECT (resource) state (NOT saved/restored by @a StateScope)
		//
		//


		void
		DisableVertexAttribArray(
				GLuint index);

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
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		void
		FramebufferTexture1D(
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		void
		FramebufferTexture2D(
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level);

		void
		FramebufferTexture3D(
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level,
				GLint layer);

		void
		FramebufferTextureLayer(
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture,
				GLint level,
				GLint layer);

		/**
		 * Note that we don't shadow globe state set by glVertexAttrib4f, glVertexAttribI4i, etc.
		 *
		 * This is generic vertex attribute state that only gets used if a vertex array is *not* enabled for
		 * a generic attribute required by the vertex shader. However, according to the 3.3 core profile spec:
		 *
		 *   If an array corresponding to a generic attribute required by a vertex shader is enabled, the
		 *   corresponding current generic attribute value is undefined after the execution of DrawElementsOneInstance.
		 *
		 * ...so essentially any state set with glVertexAttrib4f, glVertexAttribI4i, etc, prior to a draw call
		 * is undefined after the draw call so we cannot track it (apparently this was rectified in OpenGL 4.2).
		 *
		 * A better approach is to instead set a uniform in the vertex shader (eg, a constant colour for the entire drawable).
		 *
		 * NOTE: The OpenGL functions glVertexAttrib4f, glVertexAttribI4i, etc, are not available for this class to use anyway.
		 *       This is because they were not exposed by Qt until 4.4 core (ie, were not available until QOpenGLFunctions_4_4_Core),
		 *       probably for a reason similar to above, and hence are not available in our @a OpenGLFunctions (used by this class).
		 */

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


		/////////////////////////////////////////////
		//                                         //
		// Functions introduced in OpenGL 4.0 core //
		//                                         //
		/////////////////////////////////////////////


		/////////////////////////////////////////////
		//                                         //
		// Functions introduced in OpenGL 4.1 core //
		//                                         //
		/////////////////////////////////////////////


		/////////////////////////////////////////////
		//                                         //
		// Functions introduced in OpenGL 4.2 core //
		//                                         //
		/////////////////////////////////////////////


		/////////////////////////////////////////////
		//                                         //
		// Functions introduced in OpenGL 4.3 core //
		//                                         //
		/////////////////////////////////////////////


		///////////////////////////////////////////////////////////////////////////////////////////////
		// Get OpenGL state methods.
		//
		// Note: Unlike the 'set' methods, only a handful of 'get' methods are typically needed.
		//       Applications usually set OpenGL state (rather than get it) since retrieving state
		//       is typically much slower (requiring a round-trip to the driver/GPU).
		//       However we've shadowed/cached some global state so it's convenient to be able to
		//       query some of that.
		///////////////////////////////////////////////////////////////////////////////////////////////


		/**
		 * Returns the current scissor rectangle.
		 *
		 * Note that the default scissor rectangle is the current dimensions (in device pixels) of the framebuffer
		 * (either main framebuffer or a framebuffer object) currently attached to the OpenGL context.
		 */
		const GLViewport &
		get_scissor() const;

		/**
		 * Returns the current viewport rectangle.
		 *
		 * Note that the default viewport rectangle is the current dimensions (in device pixels) of the framebuffer
		 * (either main framebuffer or a framebuffer object) currently attached to the OpenGL context.
		 */
		const GLViewport &
		get_viewport() const;

		/**
		 * Returns true if the specified capability is currently enabled
		 * (via @a Enable/Disable or @a Enablei/Disablei).
		 *
		 * If the capability is indexed (eg, GL_BLEND) then @a index can be non-zero.
		 */
		bool
		is_capability_enabled(
				GLenum cap,
				GLuint index = 0) const;


	private: // For use by @a GLContext...

		//
		// Only @a GLContext should be able to create us.
		//
		friend class GLContext;

		/**
		 * Creates a @a GL object.
		 */
		static
		non_null_ptr_type
		create(
				const GLContext::non_null_ptr_type &context,
				OpenGLFunctions &opengl_functions,
				const GLStateStore::non_null_ptr_type &state_store)
		{
			return non_null_ptr_type(new GL(context, opengl_functions, state_store));
		}

	private: // For use by OpenGL resource object classes (such as @a GLTexture)...

		//
		// Only resource object classes should be able to access the low-level OpenGL functions.
		//
		friend class GLBuffer;
		friend class GLFramebuffer;
		friend class GLProgram;
		friend class GLRenderbuffer;
		friend class GLSampler;
		friend class GLShader;
		friend class GLTexture;
		friend class GLVertexArray;

		OpenGLFunctions &
		get_opengl_functions()
		{
			return d_opengl_functions;
		}

	private:

		//! Constructor.
		GL(
				const GLContext::non_null_ptr_type &context,
				OpenGLFunctions &opengl_functions,
				const GLStateStore::non_null_ptr_type &state_store);


		/**
		 * Manages objects associated with the current OpenGL context.
		 */
		GLContext::non_null_ptr_type d_context;

		/**
		 * The OpenGL functions.
		 */
		OpenGLFunctions &d_opengl_functions;

		/**
		 * Context capabilities.
		 */
		const GLCapabilities &d_capabilities;
		
		/**
		 * Tracks the current OpenGL global state.
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

		/**
		 * Default framebuffer resource.
		 *
		 * Note: This might not be zero.
		 *       For example, each QOpenGLWidget has its own framebuffer object
		 *       (that we treat as our default framebuffer when rendering into it).
		 */
		GLuint d_default_framebuffer_resource;
	};
}

#endif // GPLATES_OPENGL_GL_H
