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

#include <exception>
#include <QtGlobal>

#include "OpenGL.h"  // For Class GL and the OpenGL constants/typedefs

#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GL::GL(
		Vulkan &vulkan,
		const GLContext::non_null_ptr_type &context,
		const GLCapabilities &capabilities,
		OpenGLFunctions &opengl_functions,
		const GLStateStore::non_null_ptr_type &state_store,
		const GLViewport &default_viewport,
		const GLuint default_framebuffer_object) :
	d_vulkan(vulkan),
	d_context(context),
	d_opengl_functions(opengl_functions),
	d_capabilities(capabilities),
	d_current_state(GLState::create(opengl_functions, capabilities, state_store)),
	// Default viewport/scissor starts out as the initial window dimensions returned by context.
	// However it can change when the window (that context is attached to) is resized...
	d_default_viewport(default_viewport),
	// Default draw/read buffer is the back buffer (GL_BACK)...
	d_default_draw_read_buffer(GL_BACK),
	d_default_framebuffer_resource(default_framebuffer_object)
{
#if 0
	// On entering this scope set the default viewport/scissor rectangle to the dimensions
	// (in device pixels) of the framebuffer currently attached to the OpenGL context.
	// This is then considered the default viewport for the current rendering scope.
	//
	// We explicitly set the viewport/scissor OpenGL state here. This is unusual since it's all meant
	// to be wrapped by GLState and the GLStateSet derivations. We do this because whenever GL::Viewport()
	// or GL::Scissor() are called, we pass the default viewport to GLState (which shadows the actual OpenGL
	// state) and hence our default viewport should represent the actual OpenGL state (as seen by OpenGL).
	d_opengl_functions.glViewport(
			d_default_viewport.x(), d_default_viewport.y(),
			d_default_viewport.width(), d_default_viewport.height());
	d_opengl_functions.glScissor(
			d_default_viewport.x(), d_default_viewport.y(),
			d_default_viewport.width(), d_default_viewport.height());
#endif

	// Note that we're expecting the current OpenGL state to be the *default* OpenGL state.
}


GPlatesOpenGL::GL::~GL()
{
	// Restore the default state.
	d_current_state->reset_to_default();
}


void
GPlatesOpenGL::GL::BindAttribLocation(
		GLProgram::shared_ptr_type program,
		GLuint index,
		const GLchar *name)
{
	d_opengl_functions.glBindAttribLocation(program->get_resource_handle(), index, name);
}


void
GPlatesOpenGL::GL::BindBuffer(
		GLenum target,
		boost::optional<GLBuffer::shared_ptr_type> buffer)
{
	// The element array buffer binding is not global state in the OpenGL core profile.
	// The binding is stored in a vertex array object.
	// And it's invalid to bind an element array buffer when no vertex array object is bound.
	// Which means it's not really global state in the core profile.
	if (target == GL_ELEMENT_ARRAY_BUFFER)
	{
		// Can only bind a vertex element buffer when a vertex array object is currently bound.
		GPlatesGlobal::Assert<OpenGLException>(
				d_current_state->get_bind_vertex_array(),
				GPLATES_ASSERTION_SOURCE,
				"Cannot bind GL_ELEMENT_ARRAY_BUFFER because a vertex array object is not currently bound.");

		d_opengl_functions.glBindBuffer(
				GL_ELEMENT_ARRAY_BUFFER,
				// The buffer resource to bind (or 0 to unbind)...
				buffer ? buffer.get()->get_resource_handle() : 0);
	}
	else
	{
		d_current_state->bind_buffer(target, buffer);
	}
}


void
GPlatesOpenGL::GL::BindBufferBase(
		GLenum target,
		GLuint index,
		boost::optional<GLBuffer::shared_ptr_type> buffer)
{
	// Only used for targets GL_UNIFORM_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER, GL_SHADER_STORAGE_BUFFER and GL_ATOMIC_COUNTER_BUFFER.
	d_current_state->bind_buffer_base(target, index, buffer);
}


void
GPlatesOpenGL::GL::BindBufferRange(
		GLenum target,
		GLuint index,
		boost::optional<GLBuffer::shared_ptr_type> buffer,
		GLintptr offset,
		GLsizeiptr size)
{
	// Only used for targets GL_UNIFORM_BUFFER, GL_TRANSFORM_FEEDBACK_BUFFER, GL_SHADER_STORAGE_BUFFER and GL_ATOMIC_COUNTER_BUFFER.
	d_current_state->bind_buffer_range(target, index, buffer, offset, size);
}


void
GPlatesOpenGL::GL::BindFramebuffer(
		GLenum target,
		boost::optional<GLFramebuffer::shared_ptr_type> framebuffer)
{
	d_current_state->bind_framebuffer(
			target,
			framebuffer.get(),
			// Default framebuffer resource (might not be zero, eg, each QOpenGLWindow has its own framebuffer object)...
			d_default_framebuffer_resource);
}


void
GPlatesOpenGL::GL::BindImageTexture(
		GLuint image_unit,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLboolean layered,
		GLint layer,
		GLenum access,
		GLenum format)
{
	d_current_state->bind_image_texture(image_unit, texture, level, layered, layer, access, format);
}


void
GPlatesOpenGL::GL::BindRenderbuffer(
		GLenum target,
		boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer)
{
	d_current_state->bind_renderbuffer(target, renderbuffer);
}


void
GPlatesOpenGL::GL::BindSampler(
		GLuint unit,
		boost::optional<GLSampler::shared_ptr_type> sampler)
{
	d_current_state->bind_sampler(unit, sampler);
}


void
GPlatesOpenGL::GL::BindTextureUnit(
		GLuint unit,
		boost::optional<GLTexture::shared_ptr_type> texture)
{
	d_current_state->bind_texture_unit(unit, texture);
}


void
GPlatesOpenGL::GL::BindVertexArray(
		boost::optional<GLVertexArray::shared_ptr_type> vertex_array)
{
	d_current_state->bind_vertex_array(vertex_array);
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
GPlatesOpenGL::GL::BufferData(
		GLenum target,
		GLsizeiptr size,
		const GLvoid *data,
		GLenum usage)
{
	d_opengl_functions.glBufferData(target, size, data, usage);
}


GLenum
GPlatesOpenGL::GL::CheckFramebufferStatus(
		GLenum target)
{
	return d_opengl_functions.glCheckFramebufferStatus(target);
}

void
GPlatesOpenGL::GL::ClampColor(
		GLenum target,
		GLenum clamp)
{
	d_current_state->clamp_color(target, clamp);
}


void
GPlatesOpenGL::GL::Clear(
		GLbitfield mask)
{
	d_opengl_functions.glClear(mask);
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
GPlatesOpenGL::GL::ClearNamedBufferData(
		GLBuffer::shared_ptr_type buffer,
		GLenum internalformat,
		GLenum format,
		GLenum type,
		const void *data)
{
	d_opengl_functions.glClearNamedBufferData(buffer->get_resource_handle(), internalformat, format, type, data);
}


void
GPlatesOpenGL::GL::ClearNamedBufferSubData(
		GLBuffer::shared_ptr_type buffer,
		GLenum internalformat,
		GLintptr offset,
		GLsizei size,
		GLenum format,
		GLenum type,
		const void *data)
{
	d_opengl_functions.glClearNamedBufferSubData(buffer->get_resource_handle(), internalformat, offset, size, format, type, data);
}


void
GPlatesOpenGL::GL::ClearStencil(
		GLint stencil)
{
	d_current_state->clear_stencil(stencil);
}


void
GPlatesOpenGL::GL::ClearTexSubImage(
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
		const void *data)
{
	d_opengl_functions.glClearTexSubImage(texture->get_resource_handle(), level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}


void
GPlatesOpenGL::GL::ClearTexImage(
		GLTexture::shared_ptr_type texture,
		GLint level,
		GLenum format,
		GLenum type,
		const void *data)
{
	d_opengl_functions.glClearTexImage(texture->get_resource_handle(), level, format, type, data);
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
GPlatesOpenGL::GL::DisableVertexArrayAttrib(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint index)
{
	d_opengl_functions.glDisableVertexArrayAttrib(vertex_array->get_resource_handle(), index);
}


void
GPlatesOpenGL::GL::DrawArrays(
		GLenum mode,
		GLint first,
		GLsizei count)
{
	d_opengl_functions.glDrawArrays(mode, first, count);
}


void
GPlatesOpenGL::GL::DrawBuffer(
		GLenum buf)
{
	d_current_state->draw_buffer(buf, d_default_draw_read_buffer);
}


void
GPlatesOpenGL::GL::DrawBuffers(
		const std::vector<GLenum> &bufs)
{
	d_current_state->draw_buffers(bufs, d_default_draw_read_buffer);
}


void
GPlatesOpenGL::GL::DrawElements(
		GLenum mode,
		GLsizei count,
		GLenum type,
		const GLvoid *indices)
{
	d_opengl_functions.glDrawElements(mode, count, type, indices);
}


void
GPlatesOpenGL::GL::DrawRangeElements(
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		const GLvoid *indices)
{
	d_opengl_functions.glDrawRangeElements(mode, start, end, count, type, indices);
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
GPlatesOpenGL::GL::EnableVertexArrayAttrib(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint index)
{
	d_opengl_functions.glEnableVertexArrayAttrib(vertex_array->get_resource_handle(), index);
}


void
GPlatesOpenGL::GL::FlushMappedBufferRange(
		GLenum target,
		GLintptr offset,
		GLsizeiptr length)
{
	d_opengl_functions.glFlushMappedBufferRange(target, offset, length);
}


void
GPlatesOpenGL::GL::FramebufferRenderbuffer(
		GLenum target,
		GLenum attachment,
		GLenum renderbuffertarget,
		boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer)
{
	// Either attach the specified renderbuffer or detach.
	GLuint renderbuffer_resource = 0;
	if (renderbuffer)
	{
		renderbuffer_resource = renderbuffer.get()->get_resource_handle();
	}

	d_opengl_functions.glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer_resource);
}


void
GPlatesOpenGL::GL::FramebufferTexture(
		GLenum target,
		GLenum attachment,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	d_opengl_functions.glFramebufferTexture(target, attachment, texture_resource, level);
}


void
GPlatesOpenGL::GL::FramebufferTexture1D(
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	d_opengl_functions.glFramebufferTexture1D(target, attachment, textarget, texture_resource, level);
}


void
GPlatesOpenGL::GL::FramebufferTexture2D(
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	d_opengl_functions.glFramebufferTexture2D(target, attachment, textarget, texture_resource, level);
}


void
GPlatesOpenGL::GL::FramebufferTexture3D(
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLint layer)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	d_opengl_functions.glFramebufferTexture3D(target, attachment, textarget, texture_resource, level, layer);
}


void
GPlatesOpenGL::GL::FramebufferTextureLayer(
		GLenum target,
		GLenum attachment,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLint layer)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	d_opengl_functions.glFramebufferTextureLayer(target, attachment, texture_resource, level, layer);
}


void
GPlatesOpenGL::GL::FrontFace(
		GLenum dir)
{
	d_current_state->front_face(dir);
}


GLenum
GPlatesOpenGL::GL::GetError()
{
	return d_opengl_functions.glGetError();
}


void
GPlatesOpenGL::GL::GetTextureImage(
		GLTexture::shared_ptr_type texture,
		GLint level,
		GLenum format,
		GLenum type,
		GLsizei bufSize,
		void *pixels)
{
	d_opengl_functions.glGetTextureImage(texture->get_resource_handle(), level, format, type, bufSize, pixels);
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
	// We use OpenGL 4.5 core profile, with "forward compatibility" which removes support for deprecated wide lines.
	// They're not well-supported on macOS anyway (even in Vulkan).
	// So just set the line width to 1.0 for now (to avoid an OpenGL error getting flagged).
	//
	// TODO: Replace wide lines altogether - with rectangle geometry and an anti-aliasing fragment shader.
	d_current_state->line_width(1.0f/*width*/);
}


GLvoid *
GPlatesOpenGL::GL::MapBuffer(
		GLenum target,
		GLenum access)
{
	return d_opengl_functions.glMapBuffer(target, access);
}


GLvoid *
GPlatesOpenGL::GL::MapBufferRange(
		GLenum target,
		GLintptr offset,
		GLsizeiptr length,
		GLbitfield access)
{
	return d_opengl_functions.glMapBufferRange(target, offset, length, access);
}


void
GPlatesOpenGL::GL::MemoryBarrier(
		GLbitfield barriers)
{
	d_opengl_functions.glMemoryBarrier(barriers);
}


void
GPlatesOpenGL::GL::MemoryBarrierByRegion(
		GLbitfield barriers)
{
	d_opengl_functions.glMemoryBarrierByRegion(barriers);
}


void
GPlatesOpenGL::GL::NamedBufferStorage(
		GLBuffer::shared_ptr_type buffer,
		GLsizei size,
		const void *data,
		GLbitfield flags)
{
	d_opengl_functions.glNamedBufferStorage(buffer->get_resource_handle(), size, data, flags);
}


void
GPlatesOpenGL::GL::NamedBufferData(
		GLBuffer::shared_ptr_type buffer,
		GLsizei size,
		const void *data,
		GLenum usage)
{
	d_opengl_functions.glNamedBufferData(buffer->get_resource_handle(), size, data, usage);
}


void
GPlatesOpenGL::GL::NamedBufferSubData(
		GLBuffer::shared_ptr_type buffer,
		GLintptr offset,
		GLsizei size,
		const void *data)
{
	d_opengl_functions.glNamedBufferSubData(buffer->get_resource_handle(), offset, size, data);
}


void
GPlatesOpenGL::GL::PixelStoref(
		GLenum pname,
		GLfloat param)
{
	d_current_state->pixel_storei(pname, param);
}

void
GPlatesOpenGL::GL::PixelStorei(
		GLenum pname,
		GLint param)
{
	d_current_state->pixel_storef(pname, param);
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
	// Modern OpenGL requires 'face' to be 'GL_FRONT_AND_BACK'.
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
	d_current_state->read_buffer(src, d_default_draw_read_buffer);
}


void
GPlatesOpenGL::GL::ReadPixels(
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		GLvoid *pixels)
{
	d_opengl_functions.glReadPixels(x, y, width, height, format, type, pixels);
}


void
GPlatesOpenGL::GL::RenderbufferStorage(
		GLenum target,
		GLenum internalformat,
		GLsizei width,
		GLsizei height)
{
	d_opengl_functions.glRenderbufferStorage(target, internalformat, width, height);
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
GPlatesOpenGL::GL::SamplerParameterf(
		GLSampler::shared_ptr_type sampler,
		GLenum pname,
		GLfloat param)
{
	d_opengl_functions.glSamplerParameterf(sampler->get_resource_handle(), pname, param);
}


void
GPlatesOpenGL::GL::SamplerParameterfv(
		GLSampler::shared_ptr_type sampler,
		GLenum pname,
		const GLfloat *param)
{
	d_opengl_functions.glSamplerParameterfv(sampler->get_resource_handle(), pname, param);
}


void
GPlatesOpenGL::GL::SamplerParameteri(
		GLSampler::shared_ptr_type sampler,
		GLenum pname,
		GLint param)
{
	d_opengl_functions.glSamplerParameteri(sampler->get_resource_handle(), pname, param);
}


void
GPlatesOpenGL::GL::SamplerParameteriv(
		GLSampler::shared_ptr_type sampler,
		GLenum pname,
		const GLint *param)
{
	d_opengl_functions.glSamplerParameteriv(sampler->get_resource_handle(), pname, param);
}


void
GPlatesOpenGL::GL::SamplerParameterIiv(
		GLSampler::shared_ptr_type sampler,
		GLenum pname,
		const GLint *param)
{
	d_opengl_functions.glSamplerParameterIiv(sampler->get_resource_handle(), pname, param);
}


void
GPlatesOpenGL::GL::SamplerParameterIuiv(
		GLSampler::shared_ptr_type sampler,
		GLenum pname,
		const GLuint *param)
{
	d_opengl_functions.glSamplerParameterIuiv(sampler->get_resource_handle(), pname, param);
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
GPlatesOpenGL::GL::ShaderStorageBlockBinding(
		GLProgram::shared_ptr_type program,
		GLuint storageBlockIndex,
		GLuint storageBlockBinding)
{
	d_opengl_functions.glShaderStorageBlockBinding(program->get_resource_handle(), storageBlockIndex, storageBlockBinding);
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
GPlatesOpenGL::GL::TextureParameterf(
		GLTexture::shared_ptr_type texture,
		GLenum pname,
		GLfloat param)
{
	d_opengl_functions.glTextureParameterf(texture->get_resource_handle(), pname, param);
}


void
GPlatesOpenGL::GL::TextureParameterfv(
		GLTexture::shared_ptr_type texture,
		GLenum pname,
		const GLfloat *params)
{
	d_opengl_functions.glTextureParameterfv(texture->get_resource_handle(), pname, params);
}


void
GPlatesOpenGL::GL::TextureParameteri(
		GLTexture::shared_ptr_type texture,
		GLenum pname,
		GLint param)
{
	d_opengl_functions.glTextureParameteri(texture->get_resource_handle(), pname, param);
}


void
GPlatesOpenGL::GL::TextureParameteriv(
		GLTexture::shared_ptr_type texture,
		GLenum pname,
		const GLint *params)
{
	d_opengl_functions.glTextureParameteriv(texture->get_resource_handle(), pname, params);
}


void
GPlatesOpenGL::GL::TextureParameterIiv(
		GLTexture::shared_ptr_type texture,
		GLenum pname,
		const GLint *params)
{
	d_opengl_functions.glTextureParameterIiv(texture->get_resource_handle(), pname, params);
}


void
GPlatesOpenGL::GL::TextureParameterIuiv(
		GLTexture::shared_ptr_type texture,
		GLenum pname,
		const GLuint *params)
{
	d_opengl_functions.glTextureParameterIuiv(texture->get_resource_handle(), pname, params);
}


void
GPlatesOpenGL::GL::TextureStorage1D(
		GLTexture::shared_ptr_type texture,
		GLsizei levels,
		GLenum internalformat,
		GLsizei width)
{
	d_opengl_functions.glTextureStorage1D(texture->get_resource_handle(), levels, internalformat, width);
}


void
GPlatesOpenGL::GL::TextureStorage2D(
		GLTexture::shared_ptr_type texture,
		GLsizei levels,
		GLenum internalformat,
		GLsizei width,
		GLsizei height)
{
	d_opengl_functions.glTextureStorage2D(texture->get_resource_handle(), levels, internalformat, width, height);
}


void
GPlatesOpenGL::GL::TextureStorage3D(
		GLTexture::shared_ptr_type texture,
		GLsizei levels,
		GLenum internalformat,
		GLsizei width,
		GLsizei height,
		GLsizei depth)
{
	d_opengl_functions.glTextureStorage3D(texture->get_resource_handle(), levels, internalformat, width, height, depth);
}


void
GPlatesOpenGL::GL::TextureSubImage1D(
		GLTexture::shared_ptr_type texture,
		GLint level,
		GLint xoffset,
		GLsizei width,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	d_opengl_functions.glTextureSubImage1D(texture->get_resource_handle(), level, xoffset, width, format, type, pixels);
}


void
GPlatesOpenGL::GL::TextureSubImage2D(
		GLTexture::shared_ptr_type texture,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	d_opengl_functions.glTextureSubImage2D(texture->get_resource_handle(), level, xoffset, yoffset, width, height, format, type, pixels);
}


void
GPlatesOpenGL::GL::TextureSubImage3D(
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
		const GLvoid *pixels)
{
	d_opengl_functions.glTextureSubImage3D(texture->get_resource_handle(), level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}


void
GPlatesOpenGL::GL::TextureBuffer(
		GLTexture::shared_ptr_type texture,
		GLenum internalformat,
		GLBuffer::shared_ptr_type buffer)
{
	d_opengl_functions.glTextureBuffer(texture->get_resource_handle(), internalformat, buffer->get_resource_handle());
}


void
GPlatesOpenGL::GL::TextureBufferRange(
		GLTexture::shared_ptr_type texture,
		GLenum internalformat,
		GLBuffer::shared_ptr_type buffer,
		GLintptr offset,
		GLsizei size)
{
	d_opengl_functions.glTextureBufferRange(texture->get_resource_handle(), internalformat, buffer->get_resource_handle(), offset, size);
}


void
GPlatesOpenGL::GL::Uniform1f(
		GLint location,
		GLfloat v0)
{
	d_opengl_functions.glUniform1f(location, v0);
}


void
GPlatesOpenGL::GL::Uniform1fv(
		GLint location,
		GLsizei count,
		const GLfloat *value)
{
	d_opengl_functions.glUniform1fv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform1i(
		GLint location,
		GLint v0)
{
	d_opengl_functions.glUniform1i(location, v0);
}


void
GPlatesOpenGL::GL::Uniform1iv(
		GLint location,
		GLsizei count,
		const GLint *value)
{
	d_opengl_functions.glUniform1iv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform1ui(
		GLint location,
		GLuint v0)
{
	d_opengl_functions.glUniform1ui(location, v0);
}


void
GPlatesOpenGL::GL::Uniform1uiv(
		GLint location,
		GLsizei count,
		const GLuint *value)
{
	d_opengl_functions.glUniform1uiv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform2f(
		GLint location,
		GLfloat v0,
		GLfloat v1)
{
	d_opengl_functions.glUniform2f(location, v0, v1);
}


void
GPlatesOpenGL::GL::Uniform2fv(
		GLint location,
		GLsizei count,
		const GLfloat *value)
{
	d_opengl_functions.glUniform2fv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform2i(
		GLint location,
		GLint v0,
		GLint v1)
{
	d_opengl_functions.glUniform2i(location, v0, v1);
}


void
GPlatesOpenGL::GL::Uniform2iv(
		GLint location,
		GLsizei count,
		const GLint *value)
{
	d_opengl_functions.glUniform2iv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform2ui(
		GLint location,
		GLuint v0,
		GLuint v1)
{
	d_opengl_functions.glUniform2ui(location, v0, v1);
}


void
GPlatesOpenGL::GL::Uniform2uiv(
		GLint location,
		GLsizei count,
		const GLuint *value)
{
	d_opengl_functions.glUniform2uiv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform3f(
		GLint location,
		GLfloat v0,
		GLfloat v1,
		GLfloat v2)
{
	d_opengl_functions.glUniform3f(location, v0, v1, v2);
}


void
GPlatesOpenGL::GL::Uniform3fv(
		GLint location,
		GLsizei count,
		const GLfloat *value)
{
	d_opengl_functions.glUniform3fv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform3i(
		GLint location,
		GLint v0,
		GLint v1,
		GLint v2)
{
	d_opengl_functions.glUniform3i(location, v0, v1, v2);
}


void
GPlatesOpenGL::GL::Uniform3iv(
		GLint location,
		GLsizei count,
		const GLint *value)
{
	d_opengl_functions.glUniform3iv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform3ui(
		GLint location,
		GLuint v0,
		GLuint v1,
		GLuint v2)
{
	d_opengl_functions.glUniform3ui(location, v0, v1, v2);
}


void
GPlatesOpenGL::GL::Uniform3uiv(
		GLint location,
		GLsizei count,
		const GLuint *value)
{
	d_opengl_functions.glUniform3uiv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform4f(
		GLint location,
		GLfloat v0,
		GLfloat v1,
		GLfloat v2,
		GLfloat v3)
{
	d_opengl_functions.glUniform4f(location, v0, v1, v2, v3);
}


void
GPlatesOpenGL::GL::Uniform4fv(
		GLint location,
		GLsizei count,
		const GLfloat *value)
{
	d_opengl_functions.glUniform4fv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform4i(
		GLint location,
		GLint v0,
		GLint v1,
		GLint v2,
		GLint v3)
{
	d_opengl_functions.glUniform4i(location, v0, v1, v2, v3);
}


void
GPlatesOpenGL::GL::Uniform4iv(
		GLint location,
		GLsizei count,
		const GLint *value)
{
	d_opengl_functions.glUniform4iv(location, count, value);
}


void
GPlatesOpenGL::GL::Uniform4ui(
		GLint location,
		GLuint v0,
		GLuint v1,
		GLuint v2,
		GLuint v3)
{
	d_opengl_functions.glUniform4ui(location, v0, v1, v2, v3);
}


void
GPlatesOpenGL::GL::Uniform4uiv(
		GLint location,
		GLsizei count,
		const GLuint *value)
{
	d_opengl_functions.glUniform4uiv(location, count, value);
}


void
GPlatesOpenGL::GL::UniformBlockBinding(
		GLProgram::shared_ptr_type program,
		GLuint uniformBlockIndex,
		GLuint uniformBlockBinding)
{
	d_opengl_functions.glUniformBlockBinding(program->get_resource_handle(), uniformBlockIndex, uniformBlockBinding);
}


void
GPlatesOpenGL::GL::UniformMatrix2fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix2fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix2x3fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix2x3fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix2x4fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix2x4fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix3fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix3fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix3x2fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix3x2fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix3x4fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix3x4fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix4fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix4fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix4x2fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix4x2fv(location, count, transpose, value);
}


void
GPlatesOpenGL::GL::UniformMatrix4x3fv(
		GLint location,
		GLsizei count,
		GLboolean transpose,
		const GLfloat *value)
{
	d_opengl_functions.glUniformMatrix4x3fv(location, count, transpose, value);
}


GLboolean
GPlatesOpenGL::GL::UnmapBuffer(
		GLenum target)
{
	return d_opengl_functions.glUnmapBuffer(target);
}


void
GPlatesOpenGL::GL::UseProgram(
		boost::optional<GLProgram::shared_ptr_type> program)
{
	d_current_state->use_program(program);
}


void
GPlatesOpenGL::GL::VertexArrayAttribBinding(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint attribindex,
		GLuint bindingindex)
{
	d_opengl_functions.glVertexArrayAttribBinding(vertex_array->get_resource_handle(), attribindex, bindingindex);
}


void
GPlatesOpenGL::GL::VertexArrayAttribFormat(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint attribindex,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLuint relativeoffset)
{
	d_opengl_functions.glVertexArrayAttribFormat(vertex_array->get_resource_handle(), attribindex, size, type, normalized, relativeoffset);
}


void
GPlatesOpenGL::GL::VertexArrayAttribIFormat(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint attribindex,
		GLint size,
		GLenum type,
		GLuint relativeoffset)
{
	d_opengl_functions.glVertexArrayAttribIFormat(vertex_array->get_resource_handle(), attribindex, size, type, relativeoffset);
}


void
GPlatesOpenGL::GL::VertexArrayAttribLFormat(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint attribindex,
		GLint size,
		GLenum type,
		GLuint relativeoffset)
{
	d_opengl_functions.glVertexArrayAttribLFormat(vertex_array->get_resource_handle(), attribindex, size, type, relativeoffset);
}


void
GPlatesOpenGL::GL::VertexArrayBindingDivisor(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint bindingindex,
		GLuint divisor)
{
	d_opengl_functions.glVertexArrayBindingDivisor(vertex_array->get_resource_handle(), bindingindex, divisor);
}


void
GPlatesOpenGL::GL::VertexArrayElementBuffer(
		GLVertexArray::shared_ptr_type vertex_array,
		GLBuffer::shared_ptr_type buffer)
{
	d_opengl_functions.glVertexArrayElementBuffer(vertex_array->get_resource_handle(), buffer->get_resource_handle());
}


void
GPlatesOpenGL::GL::VertexArrayVertexBuffer(
		GLVertexArray::shared_ptr_type vertex_array,
		GLuint bindingindex,
		GLBuffer::shared_ptr_type buffer,
		GLintptr offset,
		GLsizei stride)
{
	d_opengl_functions.glVertexArrayVertexBuffer(vertex_array->get_resource_handle(), bindingindex, buffer->get_resource_handle(), offset, stride);
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


const GPlatesOpenGL::GLViewport &
GPlatesOpenGL::GL::get_scissor() const
{
	return d_current_state->get_scissor(d_default_viewport);
}


const GPlatesOpenGL::GLViewport &
GPlatesOpenGL::GL::get_viewport() const
{
	return d_current_state->get_viewport(d_default_viewport);
}


bool
GPlatesOpenGL::GL::is_capability_enabled(
		GLenum cap,
		GLuint index) const
{
	return d_current_state->is_capability_enabled(cap, index);
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
