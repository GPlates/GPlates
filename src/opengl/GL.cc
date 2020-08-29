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

#include <opengl/OpenGL3.h>
#include <exception>
#include <QtGlobal>

#include "GL.h"

#include "OpenGLException.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GL::GL(
		const GLContext::non_null_ptr_type &context,
		const GLStateStore::non_null_ptr_type &state_store) :
	d_context(context),
	d_capabilities(context->get_capabilities()),
	d_current_state(GLState::create(context->get_capabilities(), state_store)),
	// Default viewport/scissor starts out as the initial window dimensions returned by context.
	// However it can change when the window (that context is attached to) is resized...
	d_default_viewport(0, 0, context->get_width(), context->get_height()),
	// Default draw/read buffer is GL_FRONT if there is no back buffer, otherwise GL_BACK...
	d_default_draw_read_buffer(context->get_qgl_format().doubleBuffer() ? GL_BACK : GL_FRONT)
{
}


void
GPlatesOpenGL::GL::ActiveTexture(
		GLenum active_texture_)
{
	d_current_state->active_texture(active_texture_);
}


void
GPlatesOpenGL::GL::BindBuffer(
		GLenum target,
		boost::optional<GLBuffer::shared_ptr_type> buffer)
{
	// Re-route binding of element array buffers to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).
	//
	// The element array buffer binding is not really global state in the OpenGL core profile.
	// The binding is stored in a vertex array object. And it's invalid to bind an element array buffer
	// when no vertex array object is bound. Which means it's not really global state in the core profile.
	if (target == GL_ELEMENT_ARRAY_BUFFER)
	{
		// Get the currently bound vertex array object.
		boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

		// Can only bind a vertex element buffer when a vertex array object is currently bound.
		GPlatesGlobal::Assert<OpenGLException>(
				vertex_array,
				GPLATES_ASSERTION_SOURCE,
				"Cannot bind GL_ELEMENT_ARRAY_BUFFER because a vertex array object is not currently bound.");

		vertex_array.get()->bind_element_array_buffer(*this, buffer);
	}
	else
	{
		d_current_state->bind_buffer(target, buffer);
	}
}


void
GPlatesOpenGL::GL::BindFramebuffer(
		GLenum target,
		boost::optional<GLFramebuffer::shared_ptr_type> framebuffer)
{
	if (framebuffer)
	{
		// Bind.
		d_current_state->bind_framebuffer(
				target,
				framebuffer.get(),
				// Framebuffer resource handle associated with the current OpenGL context...
				framebuffer.get()->get_resource_handle(*this));

		// Ensure the framebuffer's internal state is reflected in the current context.
		// Each 'GLFramebuffer' instance has one native framebuffer object per OpenGL context.
		//
		// NOTE: This must be done after binding to the framebuffer 'target'.
		framebuffer.get()->synchronise_current_context(*this, target);
	}
	else
	{
		// Unbind.
		d_current_state->bind_framebuffer(target, boost::none, 0);
	}
}


void
GPlatesOpenGL::GL::BindRenderbuffer(
		GLenum target,
		boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer)
{
	d_current_state->bind_renderbuffer(target, renderbuffer);
}


void
GPlatesOpenGL::GL::BindTexture(
		GLenum texture_target,
		boost::optional<GLTexture::shared_ptr_type> texture_object)
{
	d_current_state->bind_texture(
			texture_target,
			// Bind to the currently active texture unit (glActiveTexture)...
			d_current_state->get_active_texture()/*texture_unit*/,
			texture_object);
}


void
GPlatesOpenGL::GL::BindVertexArray(
		boost::optional<GLVertexArray::shared_ptr_type> vertex_array)
{
	if (vertex_array)
	{
		// Bind.
		d_current_state->bind_vertex_array(
				vertex_array.get(),
				// Array resource handle associated with the current OpenGL context...
				vertex_array.get()->get_resource_handle(*this));

		// Ensure the vertex array's internal state is reflected in the current context.
		// Each 'GLVertexArray' instance has one native vertex array object per OpenGL context.
		//
		// NOTE: This must be done after binding.
		vertex_array.get()->synchronise_current_context(*this);
	}
	else
	{
		// Unbind.
		d_current_state->bind_vertex_array(boost::none, 0);
	}
}


void
GPlatesOpenGL::GL::BlendColor(
		GLclampf red,
		GLclampf green,
		GLclampf blue,
		GLclampf alpha)
{
	d_current_state->blend_color(red, green, blue, alpha);
}


void
GPlatesOpenGL::GL::BlendEquation(
		GLenum mode)
{
	d_current_state->blend_equation(mode);
}


void
GPlatesOpenGL::GL::BlendEquationSeparate(
		GLenum mode_RGB,
		GLenum mode_alpha)
{
	d_current_state->blend_equation_separate(mode_RGB, mode_alpha);
}


void
GPlatesOpenGL::GL::BlendFunc(
		GLenum src,
		GLenum dst)
{
	d_current_state->blend_func(src, dst);
}


void
GPlatesOpenGL::GL::BlendFuncSeparate(
		GLenum src_RGB,
		GLenum dst_RGB,
		GLenum src_alpha,
		GLenum dst_alpha)
{
	d_current_state->blend_func_separate(src_RGB, dst_RGB, src_alpha, dst_alpha);
}


void
GPlatesOpenGL::GL::ClampColor(
		GLenum target,
		GLenum clamp)
{
	d_current_state->clamp_color(target, clamp);
}


void
GPlatesOpenGL::GL::ClearColor(
		GLclampf red,
		GLclampf green,
		GLclampf blue,
		GLclampf alpha)
{
	d_current_state->clear_color(red, green, blue, alpha);
}


void
GPlatesOpenGL::GL::ClearDepth(
		GLclampd depth)
{
	d_current_state->clear_depth(depth);
}


void
GPlatesOpenGL::GL::ColorMask(
		GLboolean red,
		GLboolean green,
		GLboolean blue,
		GLboolean alpha)
{
	d_current_state->color_mask(red, green, blue, alpha);
}


void
GPlatesOpenGL::GL::ColorMaski(
		GLuint buf,
		GLboolean red,
		GLboolean green,
		GLboolean blue,
		GLboolean alpha)
{
	d_current_state->color_maski(buf, red, green, blue, alpha);
}


void
GPlatesOpenGL::GL::ClearStencil(
		GLint stencil)
{
	d_current_state->clear_stencil(stencil);
}


void
GPlatesOpenGL::GL::CullFace(
		GLenum mode)
{
	d_current_state->cull_face(mode);
}


void
GPlatesOpenGL::GL::DepthFunc(
		GLenum func)
{
	d_current_state->depth_func(func);
}


void
GPlatesOpenGL::GL::DepthMask(
		GLboolean flag)
{
	d_current_state->depth_mask(flag);
}


void
GPlatesOpenGL::GL::DepthRange(
		GLclampd n,
		GLclampd f)
{
	d_current_state->depth_range(n, f);
}


void
GPlatesOpenGL::GL::Disable(
		GLenum cap)
{
	d_current_state->enable(cap, false);
}


void
GPlatesOpenGL::GL::Disablei(
		GLenum cap,
		GLuint index)
{
	d_current_state->enablei(cap, index, false);
}


void
GPlatesOpenGL::GL::DisableVertexAttribArray(
		GLuint index)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only disable an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot disable vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->disable_vertex_attrib_array(*this, index);
}


void
GPlatesOpenGL::GL::DrawBuffer(
		GLenum buf)
{
	// Get the framebuffer object currently bound to 'GL_DRAW_FRAMEBUFFER', if any.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer =
			d_current_state->get_bind_framebuffer(GL_DRAW_FRAMEBUFFER);

	if (framebuffer)
	{
		// Re-route to the framebuffer object (which needs to track its internal state across contexts
		// since cannot be shared across contexts).
		framebuffer.get()->draw_buffer(*this, buf);
	}
	else
	{
		// Drawing to default framebuffer, which is global context state (not framebuffer object state).
		d_current_state->draw_buffer(buf, d_default_draw_read_buffer);
	}
}


void
GPlatesOpenGL::GL::DrawBuffers(
		const std::vector<GLenum> &bufs)
{
	// Get the framebuffer object currently bound to 'GL_DRAW_FRAMEBUFFER', if any.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer =
			d_current_state->get_bind_framebuffer(GL_DRAW_FRAMEBUFFER);

	if (framebuffer)
	{
		// Re-route to the framebuffer object (which needs to track its internal state across contexts
		// since cannot be shared across contexts).
		framebuffer.get()->draw_buffers(*this, bufs);
	}
	else
	{
		// Drawing to default framebuffer, which is global context state (not framebuffer object state).
		d_current_state->draw_buffers(bufs, d_default_draw_read_buffer);
	}
}


void
GPlatesOpenGL::GL::Enable(
		GLenum cap)
{
	d_current_state->enable(cap, true);
}


void
GPlatesOpenGL::GL::Enablei(
		GLenum cap,
		GLuint index)
{
	d_current_state->enablei(cap, index, true);
}


void
GPlatesOpenGL::GL::EnableVertexAttribArray(
		GLuint index)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only enable an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot enable vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->enable_vertex_attrib_array(*this, index);
}


void
GPlatesOpenGL::GL::FramebufferRenderbuffer(
		GLenum target,
		GLenum attachment,
		GLenum renderbuffertarget,
		boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer)
{
	// Re-route framebuffer attachments to the framebuffer object currently bound to 'target'
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the framebuffer object currently bound to 'target'.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer = d_current_state->get_bind_framebuffer(target);

	// Can only attach to framebuffer when a framebuffer object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			framebuffer,
			GPLATES_ASSERTION_SOURCE,
			"Cannot attach to framebuffer because a framebuffer object is not currently bound.");

	framebuffer.get()->framebuffer_renderbuffer(*this, target, attachment, renderbuffertarget, renderbuffer);
}


void
GPlatesOpenGL::GL::FramebufferTexture(
		GL &gl,
		GLenum target,
		GLenum attachment,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Re-route framebuffer attachments to the framebuffer object currently bound to 'target'
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the framebuffer object currently bound to 'target'.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer = d_current_state->get_bind_framebuffer(target);

	// Can only attach to framebuffer when a framebuffer object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			framebuffer,
			GPLATES_ASSERTION_SOURCE,
			"Cannot attach to framebuffer because a framebuffer object is not currently bound.");

	framebuffer.get()->framebuffer_texture(*this, target, attachment, texture, level);
}


void
GPlatesOpenGL::GL::FramebufferTexture1D(
		GL &gl,
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Re-route framebuffer attachments to the framebuffer object currently bound to 'target'
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the framebuffer object currently bound to 'target'.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer = d_current_state->get_bind_framebuffer(target);

	// Can only attach to framebuffer when a framebuffer object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			framebuffer,
			GPLATES_ASSERTION_SOURCE,
			"Cannot attach to framebuffer because a framebuffer object is not currently bound.");

	framebuffer.get()->framebuffer_texture_1D(*this, target, attachment, textarget, texture, level);
}


void
GPlatesOpenGL::GL::FramebufferTexture2D(
		GL &gl,
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Re-route framebuffer attachments to the framebuffer object currently bound to 'target'
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the framebuffer object currently bound to 'target'.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer = d_current_state->get_bind_framebuffer(target);

	// Can only attach to framebuffer when a framebuffer object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			framebuffer,
			GPLATES_ASSERTION_SOURCE,
			"Cannot attach to framebuffer because a framebuffer object is not currently bound.");

	framebuffer.get()->framebuffer_texture_2D(*this, target, attachment, textarget, texture, level);
}


void
GPlatesOpenGL::GL::FramebufferTexture3D(
		GL &gl,
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLint layer)
{
	// Re-route framebuffer attachments to the framebuffer object currently bound to 'target'
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the framebuffer object currently bound to 'target'.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer = d_current_state->get_bind_framebuffer(target);

	// Can only attach to framebuffer when a framebuffer object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			framebuffer,
			GPLATES_ASSERTION_SOURCE,
			"Cannot attach to framebuffer because a framebuffer object is not currently bound.");

	framebuffer.get()->framebuffer_texture_3D(*this, target, attachment, textarget, texture, level, layer);
}


void
GPlatesOpenGL::GL::FramebufferTextureLayer(
		GL &gl,
		GLenum target,
		GLenum attachment,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLint layer)
{
	// Re-route framebuffer attachments to the framebuffer object currently bound to 'target'
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the framebuffer object currently bound to 'target'.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer = d_current_state->get_bind_framebuffer(target);

	// Can only attach to framebuffer when a framebuffer object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			framebuffer,
			GPLATES_ASSERTION_SOURCE,
			"Cannot attach to framebuffer because a framebuffer object is not currently bound.");

	framebuffer.get()->framebuffer_texture_layer(*this, target, attachment, texture, level, layer);
}


void
GPlatesOpenGL::GL::FrontFace(
		GLenum dir)
{
	d_current_state->front_face(dir);
}


void
GPlatesOpenGL::GL::Hint(
		GLenum target,
		GLenum hint)
{
	d_current_state->hint(target, hint);
}


void
GPlatesOpenGL::GL::LineWidth(
		GLfloat width)
{
	d_current_state->line_width(width);
}


void
GPlatesOpenGL::GL::PointSize(
		GLfloat size)
{
	d_current_state->point_size(size);
}


void
GPlatesOpenGL::GL::PolygonMode(
		GLenum face,
		GLenum mode)
{
	// OpenGL 3.3 core requires 'face' to be 'GL_FRONT_AND_BACK'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			face == GL_FRONT_AND_BACK,
			GPLATES_ASSERTION_SOURCE);

	d_current_state->polygon_mode(mode);
}


void
GPlatesOpenGL::GL::PolygonOffset(
		GLfloat factor,
		GLfloat units)
{
	d_current_state->polygon_offset(factor, units);
}


void
GPlatesOpenGL::GL::PrimitiveRestartIndex(
		GLuint index)
{
	d_current_state->primitive_restart_index(index);
}


void
GPlatesOpenGL::GL::ReadBuffer(
		GLenum src)
{
	// Get the framebuffer object currently bound to 'GL_READ_FRAMEBUFFER', if any.
	boost::optional<GLFramebuffer::shared_ptr_type> framebuffer =
			d_current_state->get_bind_framebuffer(GL_READ_FRAMEBUFFER);

	if (framebuffer)
	{
		// Re-route to the framebuffer object (which needs to track its internal state across contexts
		// since cannot be shared across contexts).
		framebuffer.get()->read_buffer(*this, src);
	}
	else
	{
		// Reading from the default framebuffer, which is global context state (not framebuffer object state).
		d_current_state->read_buffer(src, d_default_draw_read_buffer);
	}
}


void
GPlatesOpenGL::GL::SampleCoverage(
		GLclampf value,
		GLboolean invert)
{
	d_current_state->sample_coverage(value, invert);
}


void
GPlatesOpenGL::GL::SampleMaski(
		GLuint mask_number,
		GLbitfield mask)
{
	d_current_state->sample_maski(mask_number, mask);
}


void
GPlatesOpenGL::GL::Scissor(
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height)
{
	d_current_state->scissor(
			GLViewport(x, y, width, height),
			d_default_viewport);
}


void
GPlatesOpenGL::GL::StencilFunc(
		GLenum func,
		GLint ref,
		GLuint mask)
{
	d_current_state->stencil_func(func, ref, mask);
}


void
GPlatesOpenGL::GL::StencilFuncSeparate(
		GLenum face,
		GLenum func,
		GLint ref,
		GLuint mask)
{
	d_current_state->stencil_func_separate(face, func, ref, mask);
}


void
GPlatesOpenGL::GL::StencilMask(
		GLuint mask)
{
	d_current_state->stencil_mask(mask);
}


void
GPlatesOpenGL::GL::StencilMaskSeparate(
		GLenum face,
		GLuint mask)
{
	d_current_state->stencil_mask_separate(face, mask);
}


void
GPlatesOpenGL::GL::StencilOp(
		GLenum sfail,
		GLenum dpfail,
		GLenum dppass)
{
	d_current_state->stencil_op(sfail, dpfail, dppass);
}


void
GPlatesOpenGL::GL::StencilOpSeparate(
		GLenum face,
		GLenum sfail,
		GLenum dpfail,
		GLenum dppass)
{
	d_current_state->stencil_op_separate(face, sfail, dpfail, dppass);
}


void
GPlatesOpenGL::GL::VertexAttribDivisor(
		GLuint index,
		GLuint divisor)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only disable an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot set vertex attribute divisor because a vertex array object is not currently bound.");

	vertex_array.get()->vertex_attrib_divisor(*this, index, divisor);
}


void
GPlatesOpenGL::GL::VertexAttribIPointer(
		GLuint index,
		GLint size,
		GLenum type,
		GLsizei stride,
		const GLvoid *pointer)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).
	//
	// The currently bound array buffer, along with the attribute array parameters, are stored in the
	// currently bound vertex array object. And it's invalid to specify an attribute array when no
	// vertex array object is bound.

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only specify an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot specify vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->vertex_attrib_i_pointer(
			*this,
			index, size, type, stride, pointer,
			// The currently bound array buffer...
			d_current_state->get_bind_buffer(GL_ARRAY_BUFFER));
}


void
GPlatesOpenGL::GL::VertexAttribPointer(
		GLuint index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		const GLvoid *pointer)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).
	//
	// The currently bound array buffer, along with the attribute array parameters, are stored in the
	// currently bound vertex array object. And it's invalid to specify an attribute array when no
	// vertex array object is bound.

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only specify an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot specify vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->vertex_attrib_pointer(
			*this,
			index, size, type, normalized, stride, pointer,
			// The currently bound array buffer...
			d_current_state->get_bind_buffer(GL_ARRAY_BUFFER));
}


void
GPlatesOpenGL::GL::Viewport(
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height)
{
	d_current_state->viewport(
			GLViewport(x, y, width, height),
			d_default_viewport);
}


GPlatesOpenGL::GL::RenderScope::RenderScope(
		GL &gl) :
	d_gl(gl),
	d_have_ended(false)
{
	// On entering this scope set the default viewport/scissor rectangle to the current dimensions
	// (in device pixels) of the framebuffer currently attached to the OpenGL context.
	// This is then considered the default viewport for the current rendering scope.
	// Note that the viewport dimensions can change when the window (attached to context) is resized,
	// so the default viewport can be different from one render scope to the next.
	d_gl.d_default_viewport = GLViewport(0, 0, d_gl.d_context->get_width(), d_gl.d_context->get_height());

	// We explicitly set the viewport/scissor OpenGL state here. This is unusual since it's all meant
	// to be wrapped by GLState and the GLStateSet derivations. We do this because whenever GL::Viewport()
	// or GL::Scissor() are called, we pass the default viewport to GLState (which shadows the actual OpenGL
	// state) and hence our default viewport should represent the actual OpenGL state (as seen by OpenGL).
	glViewport(
			d_gl.d_default_viewport.x(), d_gl.d_default_viewport.y(),
			d_gl.d_default_viewport.width(), d_gl.d_default_viewport.height());
	glScissor(
			d_gl.d_default_viewport.x(), d_gl.d_default_viewport.y(),
			d_gl.d_default_viewport.width(), d_gl.d_default_viewport.height());

	// Begin render scope.
	d_gl.d_context->begin_render();

	// Note that we're expecting the current OpenGL state to be the *default* OpenGL state.
}


GPlatesOpenGL::GL::RenderScope::~RenderScope()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		end();
	}
	catch (std::exception &exc)
	{
		qWarning() << "GL: exception thrown during render scope: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GL: exception thrown during render scope: Unknown error";
	}
}


void
GPlatesOpenGL::GL::RenderScope::end()
{
	if (!d_have_ended)
	{
		// Restore the default state.
		d_gl.d_current_state->reset_to_default();

		// End render scope.
		d_gl.d_context->end_render();

		d_have_ended = true;
	}
}


GPlatesOpenGL::GL::StateScope::StateScope(
		GL &gl,
		bool reset_to_default_state) :
	d_gl(gl),
	d_have_restored(false)
{
	gl.d_current_state->save();

	if (reset_to_default_state)
	{
		gl.d_current_state->reset_to_default();
	}
}


GPlatesOpenGL::GL::StateScope::~StateScope()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		restore();
	}
	catch (std::exception &exc)
	{
		qWarning() << "GL: exception thrown during state scope: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GL: exception thrown during state scope: Unknown error";
	}
}


void
GPlatesOpenGL::GL::StateScope::restore()
{
	if (!d_have_restored)
	{
		// Restore the global state to what it was on scope entry.
		d_gl.d_current_state->restore();

		d_have_restored = true;
	}
}
