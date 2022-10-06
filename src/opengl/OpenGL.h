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

#ifndef GPLATES_OPENGL_OPENGL_H
#define GPLATES_OPENGL_OPENGL_H

//
// Note: This header is called "OpenGL.h" instead of "GL.h" (it would normally be called "GL.h" since it
//       contains "class GL"). The reason is, on macOS, the OpenGL system header is called <OpenGL/gl.h>
//       which, when included by <qopengl.h>, ended up including this header instead (when it was called
//       "opengl/GL.h") since both are the same (when case is ignored).
//       To avoid this we now call this header "OpenGL.h" instead of "GL.h".
//       And "OpenGL.h" also includes the OpenGL constants/typedefs (via including <qopengl.h>).
//       So all clients of class GL should consider "OpenGL.h" the only OpenGL header needing inclusion
//       (all calls to OpenGL should go through class GL and all OpenGL constants/typedefs are included also).
//

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <qopengl.h>  // For OpenGL constants and typedefs.

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


		~GL();


		/**
		 * Returns the OpenGL implementation-dependent capabilities and parameters.
		 */
		const GLCapabilities &
		get_capabilities() const
		{
			return d_capabilities;
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// OpenGL methods.
		//
		// The following methods are equivalent to the native OpenGL functions with the same function name
		// (ie, with a 'gl' prefix). But we exclude the 'gl' prefix so that, for example, a function call to
		// 'BindTextureUnit()' on object 'gl' looks like 'gl.BindTextureUnit()' which looks very similar to the
		// native equivalent 'glBindTextureUnit()'.
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
		//    (such as 'Uniform4f()' affecting the currently bound @a GLProgram) and drawing commands (such as 'DrawArrays()').
		//    However some functionality has moved to the resource classes if was more convenient there (eg, @a GLProgram has a method to
		//    link a program and a method to get a uniform location since @a GLProgram wraps those with extra functionality for convenience,
		//    such as checking the link status after linking and caching the location of uniforms upon querying them).
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


		// OpenGL 2.0
		//
		// An alternative is to specify this in the shader instead (supported by our min requirement of OpenGL 3.3).
		// For example:
		//
		//   layout(location=0) in vec4 position;
		//
		void
		BindAttribLocation(
				GLProgram::shared_ptr_type program,
				GLuint index,
				const GLchar *name);

		// OpenGL 1.5
		void
		BindBuffer(
				GLenum target,
				boost::optional<GLBuffer::shared_ptr_type> buffer/*none means unbind*/);

		// OpenGL 3.0
		void
		BindBufferBase(
				GLenum target,
				GLuint index,
				boost::optional<GLBuffer::shared_ptr_type> buffer/*none means unbind*/);

		// OpenGL 3.0
		void
		BindBufferRange(
				GLenum target,
				GLuint index,
				boost::optional<GLBuffer::shared_ptr_type> buffer/*none means unbind*/,
				GLintptr offset,
				GLsizeiptr size);

		// OpenGL 3.0
		void
		BindFramebuffer(
				GLenum target,
				boost::optional<GLFramebuffer::shared_ptr_type> framebuffer/*none means bind to default framebuffer*/);

		void
		BindImageTexture(
				GLuint image_unit,
				boost::optional<GLTexture::shared_ptr_type> texture/*none means unbind*/,
				GLint level,
				GLboolean layered,
				GLint layer,
				GLenum access,
				GLenum format);

		// OpenGL 3.0
		void
		BindRenderbuffer(
				GLenum target,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer/*none means unbind*/);

		// OpenGL 3.3
		void
		BindSampler(
				GLuint unit,
				boost::optional<GLSampler::shared_ptr_type> sampler/*none means unbind*/);

		void
		BindTextureUnit(
				GLuint unit,
				boost::optional<GLTexture::shared_ptr_type> texture/*none means unbind*/);

		// OpenGL 3.0
		void
		BindVertexArray(
				boost::optional<GLVertexArray::shared_ptr_type> vertex_array/*none means unbind*/);

		// OpenGL 1.2
		void
		BlendColor(
				GLclampf red = GLclampf(0.0),
				GLclampf green = GLclampf(0.0),
				GLclampf blue = GLclampf(0.0),
				GLclampf alpha = GLclampf(0.0));

		// OpenGL 1.2
		void
		BlendEquation(
				GLenum mode);

		// OpenGL 2.0
		void
		BlendEquationSeparate(
				GLenum mode_RGB,
				GLenum mode_alpha);

		// OpenGL 1.0
		void
		BlendFunc(
				GLenum src,
				GLenum dst);

		// OpenGL 1.4
		void
		BlendFuncSeparate(
				GLenum src_RGB,
				GLenum dst_RGB,
				GLenum src_alpha,
				GLenum dst_alpha);
		
		// OpenGL 1.5
		void
		BufferData(
				GLenum target,
				GLsizeiptr size,
				const GLvoid *data,
				GLenum usage);

		void
		BufferStorage(
				GLenum target,
				GLsizeiptr size,
				const void *data,
				GLbitfield flags);

		// OpenGL 1.5
		void
		BufferSubData(
				GLenum target,
				GLintptr offset,
				GLsizeiptr size,
				const GLvoid *data);

		// OpenGL 3.0
		GLenum
		CheckFramebufferStatus(
				GLenum target);

		// OpenGL 3.0
		void
		ClampColor(
				GLenum target,
				GLenum clamp);

		// OpenGL 1.0
		void
		Clear(
				GLbitfield mask);

		void
		ClearBufferData(
				GLenum target,
				GLenum internalformat,
				GLenum format,
				GLenum type,
				const void *data);

		void
		ClearBufferSubData(
				GLenum target,
				GLenum internalformat,
				GLintptr offset,
				GLsizeiptr size,
				GLenum format,
				GLenum type,
				const void *data);

		// OpenGL 1.0
		void
		ClearColor(
				GLclampf red = GLclampf(0.0),
				GLclampf green = GLclampf(0.0),
				GLclampf blue = GLclampf(0.0),
				GLclampf alpha = GLclampf(0.0));

		// OpenGL 1.0
		void
		ClearDepth(
				GLclampd depth = GLclampd(1.0));

		void
		ClearNamedBufferData(
				GLBuffer::shared_ptr_type buffer,
				GLenum internalformat,
				GLenum format,
				GLenum type,
				const void *data);

		void
		ClearNamedBufferSubData(
				GLBuffer::shared_ptr_type buffer,
				GLenum internalformat,
				GLintptr offset,
				GLsizei size,
				GLenum format,
				GLenum type,
				const void *data);

		// OpenGL 1.0
		void
		ClearStencil(
				GLint stencil = 0);

		void
		ClearTexSubImage(
				GLTexture::shared_ptr_type texture,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLint zoffset,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLenum format,
				GLenum type,
				const void *data);

		void
		ClearTexImage(
				GLTexture::shared_ptr_type texture,
				GLint level,
				GLenum format,
				GLenum type,
				const void *data);

		// OpenGL 1.0
		void
		ColorMask(
				GLboolean red = GL_TRUE,
				GLboolean green = GL_TRUE,
				GLboolean blue = GL_TRUE,
				GLboolean alpha = GL_TRUE);

		// OpenGL 3.0
		void
		ColorMaski(
				GLuint buf,
				GLboolean red = GL_TRUE,
				GLboolean green = GL_TRUE,
				GLboolean blue = GL_TRUE,
				GLboolean alpha = GL_TRUE);

		// OpenGL 1.0
		void
		CullFace(
				GLenum mode = GL_BACK);

		// OpenGL 1.0
		void
		DepthFunc(
				GLenum func);

		// OpenGL 1.0
		void
		DepthMask(
				GLboolean flag = GL_TRUE);

		// OpenGL 1.0
		void
		DepthRange(
				GLclampd n = 0.0,
				GLclampd f = 1.0);

		// OpenGL 1.0
		void
		Disable(
				GLenum cap);

		// OpenGL 3.0
		void
		Disablei(
				GLenum cap,
				GLuint index);

		// OpenGL 2.0
		void
		DisableVertexAttribArray(
				GLuint index);

		// OpenGL 1.1
		void
		DrawArrays(
				GLenum mode,
				GLint first,
				GLsizei count);

		// OpenGL 1.0
		void
		DrawBuffer(
				GLenum buf);

		// OpenGL 2.0
		void
		DrawBuffers(
				const std::vector<GLenum> &bufs);

		// OpenGL 1.1
		void
		DrawElements(
				GLenum mode,
				GLsizei count,
				GLenum type,
				const GLvoid *indices);

		// OpenGL 1.2
		void
		DrawRangeElements(
				GLenum mode,
				GLuint start,
				GLuint end,
				GLsizei count,
				GLenum type,
				const GLvoid *indices);

		// OpenGL 1.0
		void
		Enable(
				GLenum cap);

		// OpenGL 3.0
		void
		Enablei(
				GLenum cap,
				GLuint index);

		// OpenGL 2.0
		void
		EnableVertexAttribArray(
				GLuint index);

		// OpenGL 3.0
		void
		FlushMappedBufferRange(
				GLenum target,
				GLintptr offset,
				GLsizeiptr length);

		// OpenGL 3.0
		void
		FramebufferRenderbuffer(
				GLenum target,
				GLenum attachment,
				GLenum renderbuffertarget,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer/*none means detach*/);

		// OpenGL 3.2
		void
		FramebufferTexture(
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture/*none means detach*/,
				GLint level);

		// OpenGL 3.0
		void
		FramebufferTexture1D(
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture/*none means detach*/,
				GLint level);

		// OpenGL 3.0
		void
		FramebufferTexture2D(
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture/*none means detach*/,
				GLint level);

		// OpenGL 3.0
		void
		FramebufferTexture3D(
				GLenum target,
				GLenum attachment,
				GLenum textarget,
				boost::optional<GLTexture::shared_ptr_type> texture/*none means detach*/,
				GLint level,
				GLint layer);

		// OpenGL 3.0
		void
		FramebufferTextureLayer(
				GLenum target,
				GLenum attachment,
				boost::optional<GLTexture::shared_ptr_type> texture/*none means detach*/,
				GLint level,
				GLint layer);

		// OpenGL 1.0
		void
		FrontFace(
				GLenum dir = GL_CCW);

		// OpenGL 1.0
		GLenum
		GetError();

		// OpenGL 1.0
		void
		GetTexImage(
				GLenum target,
				GLint level,
				GLenum format,
				GLenum type,
				GLvoid *pixels);

		// OpenGL 1.0
		void
		Hint(
				GLenum target,
				GLenum hint);

		// OpenGL 1.0
		void
		LineWidth(
				GLfloat width = GLfloat(1));

		// OpenGL 1.5
		GLvoid *
		MapBuffer(
				GLenum target,
				GLenum access);

		// OpenGL 3.0
		GLvoid *
		MapBufferRange(
				GLenum target,
				GLintptr offset,
				GLsizeiptr length,
				GLbitfield access);

		void
		MemoryBarrier(
				GLbitfield barriers);

		void
		MemoryBarrierByRegion(
				GLbitfield barriers);

		void
		NamedBufferStorage(
				GLBuffer::shared_ptr_type buffer,
				GLsizei size,
				const void *data,
				GLbitfield flags);

		void
		NamedBufferSubData(
				GLBuffer::shared_ptr_type buffer,
				GLintptr offset,
				GLsizei size,
				const void *data);

		// OpenGL 1.0
		void
		PixelStoref(
				GLenum pname,
				GLfloat param);

		// OpenGL 1.0
		void
		PixelStorei(
				GLenum pname,
				GLint param);

		// OpenGL 1.0
		void
		PointSize(
				GLfloat size = GLfloat(1));

		// OpenGL 1.0
		void
		PolygonMode(
				GLenum face = GL_FRONT_AND_BACK,
				GLenum mode = GL_FILL);

		// OpenGL 1.1
		void
		PolygonOffset(
				GLfloat factor = GLfloat(0),
				GLfloat units = GLfloat(0));

		// OpenGL 3.1
		void
		PrimitiveRestartIndex(
				GLuint index);

		// OpenGL 1.0
		void
		ReadBuffer(
				GLenum src);

		// OpenGL 1.0
		void
		ReadPixels(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				GLvoid *pixels);

		// OpenGL 3.0
		void
		RenderbufferStorage(
				GLenum target,
				GLenum internalformat,
				GLsizei width,
				GLsizei height);

		// OpenGL 1.3
		void
		SampleCoverage(
				GLclampf value,
				GLboolean invert);

		// OpenGL 3.2
		void
		SampleMaski(
				GLuint mask_number,
				GLbitfield mask);

		// OpenGL 3.3
		void
		SamplerParameterf(
				GLSampler::shared_ptr_type sampler,
				GLenum pname,
				GLfloat param);

		// OpenGL 3.3
		void
		SamplerParameterfv(
				GLSampler::shared_ptr_type sampler,
				GLenum pname,
				const GLfloat *param);

		// OpenGL 3.3
		void
		SamplerParameteri(
				GLSampler::shared_ptr_type sampler,
				GLenum pname,
				GLint param);

		// OpenGL 3.3
		void
		SamplerParameteriv(
				GLSampler::shared_ptr_type sampler,
				GLenum pname,
				const GLint *param);

		// OpenGL 3.3
		void
		SamplerParameterIiv(
				GLSampler::shared_ptr_type sampler,
				GLenum pname,
				const GLint *param);

		// OpenGL 3.3
		void
		SamplerParameterIuiv(
				GLSampler::shared_ptr_type sampler,
				GLenum pname,
				const GLuint *param);

		// OpenGL 1.0
		//
		// Note: The default scissor rectangle is the current dimensions (in device pixels) of the framebuffer
		//       (either main framebuffer or a framebuffer object) currently attached to the OpenGL context.
		void
		Scissor(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);

		void
		ShaderStorageBlockBinding(
				GLProgram::shared_ptr_type program,
				GLuint storageBlockIndex,
				GLuint storageBlockBinding);

		// OpenGL 1.0
		void
		StencilFunc(
				GLenum func,
				GLint ref,
				GLuint mask);

		// OpenGL 2.0
		void
		StencilFuncSeparate(
				GLenum face,
				GLenum func,
				GLint ref,
				GLuint mask);

		// OpenGL 1.0
		void
		StencilMask(
				GLuint mask = ~0/*all ones*/);

		// OpenGL 2.0
		void
		StencilMaskSeparate(
				GLenum face,
				GLuint mask = ~0/*all ones*/);

		// OpenGL 1.0
		void
		StencilOp(
				GLenum sfail,
				GLenum dpfail,
				GLenum dppass);

		// OpenGL 2.0
		void
		StencilOpSeparate(
				GLenum face,
				GLenum sfail,
				GLenum dpfail,
				GLenum dppass);

		// OpenGL 1.0
		void
		TexParameterf(
				GLenum target,
				GLenum pname,
				GLfloat param);

		// OpenGL 1.0
		void
		TexParameterfv(
				GLenum target,
				GLenum pname,
				const GLfloat *params);

		// OpenGL 1.0
		void
		TexParameteri(
				GLenum target,
				GLenum pname,
				GLint param);

		// OpenGL 1.0
		void
		TexParameteriv(
				GLenum target,
				GLenum pname,
				const GLint *params);

		// OpenGL 3.0
		void
		TexParameterIiv(
				GLenum target,
				GLenum pname,
				const GLint *params);

		// OpenGL 3.0
		void
		TexParameterIuiv(
				GLenum target,
				GLenum pname,
				const GLuint *params);

		void
		TexStorage1D(
				GLenum target,
				GLsizei levels,
				GLenum internalformat,
				GLsizei width);

		void
		TexStorage2D(
				GLenum target,
				GLsizei levels,
				GLenum internalformat,
				GLsizei width,
				GLsizei height);

		void
		TexStorage3D(
				GLenum target,
				GLsizei levels,
				GLenum internalformat,
				GLsizei width,
				GLsizei height,
				GLsizei depth);

		// OpenGL 1.1
		void
		TexSubImage1D(
				GLenum target,
				GLint level,
				GLint xoffset,
				GLsizei width,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		// OpenGL 1.1
		void
		TexSubImage2D(
				GLenum target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		// OpenGL 1.2
		void
		TexSubImage3D(
				GLenum target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLint zoffset,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		// OpenGL 1.0
		void
		TexImage1D(
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLint border,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		// OpenGL 1.0
		void
		TexImage2D(
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLint border,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		// OpenGL 1.2
		void
		TexImage3D(
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLint border,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		void
		TextureBuffer(
				GLuint texture,
				GLenum internalformat,
				GLuint buffer);

		void
		TextureBufferRange(
				GLuint texture,
				GLenum internalformat,
				GLuint buffer,
				GLintptr offset,
				GLsizei size);

		// OpenGL 2.0
		void
		Uniform1f(
				GLint location,
				GLfloat v0);

		// OpenGL 2.0
		void
		Uniform1fv(
				GLint location,
				GLsizei count,
				const GLfloat *value);

		// OpenGL 2.0
		void
		Uniform1i(
				GLint location,
				GLint v0);

		// OpenGL 2.0
		void
		Uniform1iv(
				GLint location,
				GLsizei count,
				const GLint *value);

		// OpenGL 3.0
		void
		Uniform1ui(
				GLint location,
				GLuint v0);

		// OpenGL 3.0
		void
		Uniform1uiv(
				GLint location,
				GLsizei count,
				const GLuint *value);

		// OpenGL 2.0
		void
		Uniform2f(
				GLint location,
				GLfloat v0,
				GLfloat v1);

		// OpenGL 2.0
		void
		Uniform2fv(
				GLint location,
				GLsizei count,
				const GLfloat *value);

		// OpenGL 2.0
		void
		Uniform2i(
				GLint location,
				GLint v0,
				GLint v1);

		// OpenGL 2.0
		void
		Uniform2iv(
				GLint location,
				GLsizei count,
				const GLint *value);

		// OpenGL 3.0
		void
		Uniform2ui(
				GLint location,
				GLuint v0,
				GLuint v1);

		// OpenGL 3.0
		void
		Uniform2uiv(
				GLint location,
				GLsizei count,
				const GLuint *value);

		// OpenGL 2.0
		void
		Uniform3f(
				GLint location,
				GLfloat v0,
				GLfloat v1,
				GLfloat v2);

		// OpenGL 2.0
		void
		Uniform3fv(
				GLint location,
				GLsizei count,
				const GLfloat *value);

		// OpenGL 2.0
		void
		Uniform3i(
				GLint location,
				GLint v0,
				GLint v1,
				GLint v2);

		// OpenGL 2.0
		void
		Uniform3iv(
				GLint location,
				GLsizei count,
				const GLint *value);

		// OpenGL 3.0
		void
		Uniform3ui(
				GLint location,
				GLuint v0,
				GLuint v1,
				GLuint v2);

		// OpenGL 3.0
		void
		Uniform3uiv(
				GLint location,
				GLsizei count,
				const GLuint *value);

		// OpenGL 2.0
		void
		Uniform4f(
				GLint location,
				GLfloat v0,
				GLfloat v1,
				GLfloat v2,
				GLfloat v3);

		// OpenGL 2.0
		void
		Uniform4fv(
				GLint location,
				GLsizei count,
				const GLfloat *value);

		// OpenGL 2.0
		void
		Uniform4i(
				GLint location,
				GLint v0,
				GLint v1,
				GLint v2,
				GLint v3);

		// OpenGL 2.0
		void
		Uniform4iv(
				GLint location,
				GLsizei count,
				const GLint *value);

		// OpenGL 3.0
		void
		Uniform4ui(
				GLint location,
				GLuint v0,
				GLuint v1,
				GLuint v2,
				GLuint v3);

		// OpenGL 3.0
		void
		Uniform4uiv(
				GLint location,
				GLsizei count,
				const GLuint *value);

		// OpenGL 3.1
		void
		UniformBlockBinding(
				GLProgram::shared_ptr_type program,
				GLuint uniformBlockIndex,
				GLuint uniformBlockBinding);

		// OpenGL 2.0
		void
		UniformMatrix2fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.1
		void
		UniformMatrix2x3fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.1
		void
		UniformMatrix2x4fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.0
		void
		UniformMatrix3fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.1
		void
		UniformMatrix3x2fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.1
		void
		UniformMatrix3x4fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.0
		void
		UniformMatrix4fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.1
		void
		UniformMatrix4x2fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 2.1
		void
		UniformMatrix4x3fv(
				GLint location,
				GLsizei count,
				GLboolean transpose,
				const GLfloat *value);

		// OpenGL 1.5
		GLboolean
		UnmapBuffer(
				GLenum target);

		// OpenGL 2.0
		void
		UseProgram(
				boost::optional<GLProgram::shared_ptr_type> program/*none means don't use any program*/);

		//
		// Note that we don't shadow globe state set by glVertexAttrib4f, glVertexAttribI4i, etc.
		//
		// This is generic vertex attribute state that only gets used if a vertex array is *not* enabled for
		// a generic attribute required by the vertex shader. However, according to the 3.3 core profile spec:
		//
		//   If an array corresponding to a generic attribute required by a vertex shader is enabled, the
		//   corresponding current generic attribute value is undefined after the execution of DrawElementsOneInstance.
		//
		// ...so essentially any state set with glVertexAttrib4f, glVertexAttribI4i, etc, prior to a draw call
		// is undefined after the draw call so we cannot track it (apparently this was rectified in OpenGL 4.2).
		//
		// A better approach is to instead set a uniform in the vertex shader (eg, a constant colour for the entire drawable).
		//
		// NOTE: The OpenGL functions glVertexAttrib4f, glVertexAttribI4i, etc, are not available for this class to use anyway.
		//       This is because they were not exposed by Qt until 4.4 core (ie, were not available until QOpenGLFunctions_4_4_Core),
		//       probably for a reason similar to above, and hence are not available in our @a OpenGLFunctions (used by this class).
		//

		// OpenGL 3.3
		void
		VertexAttribDivisor(
				GLuint index,
				GLuint divisor);

		// OpenGL 3.0
		void
		VertexAttribIPointer(
				GLuint index,
				GLint size,
				GLenum type,
				GLsizei stride,
				const GLvoid *pointer);

		// OpenGL 2.0
		void
		VertexAttribPointer(
				GLuint index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				const GLvoid *pointer);

		// OpenGL 1.0
		//
		// Note: The default viewport rectangle is the current dimensions (in device pixels) of the framebuffer
		//       (either main framebuffer or a framebuffer object) currently attached to the OpenGL context.
		void
		Viewport(
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height);



		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// GET *global* OpenGL state methods.
		//
		// OpenGL does natively support many 'get' functions to retrieve OpenGL state. However we have not generally
		// exposed those in this class because applications usually *set* global OpenGL state (rather than get it)
		// since retrieving state is typically much slower (requiring a round-trip to the driver/GPU).
		//
		// However this class does shadow/cache *global* state, so it's convenient to be able to query
		// some of that (especially since it doesn't require a slow round-trip to the driver/GPU).
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////


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
				const GLCapabilities &capabilities,
				OpenGLFunctions &opengl_functions,
				const GLStateStore::non_null_ptr_type &state_store,
				const GLViewport &default_viewport,
				const GLuint default_framebuffer_object)
		{
			return non_null_ptr_type(
					new GL(
							context, capabilities, opengl_functions, state_store,
							default_viewport, default_framebuffer_object));
		}

	private: // For use by OpenGL resource object classes (such as @a GLTexture)...

		//
		// Only resource object classes should be able to access the low-level OpenGL functions and OpenGL context.
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

	private:

		//! Constructor.
		GL(
				const GLContext::non_null_ptr_type &context,
				const GLCapabilities &capabilities,
				OpenGLFunctions &opengl_functions,
				const GLStateStore::non_null_ptr_type &state_store,
				const GLViewport &default_viewport,
				const GLuint default_framebuffer_object);


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
		 *       For example, each QOpenGLWindow has its own framebuffer object
		 *       (that we treat as our default framebuffer when rendering into it).
		 */
		GLuint d_default_framebuffer_resource;
	};
}

#endif // GPLATES_OPENGL_OPENGL_H
