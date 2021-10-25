/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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
#include <map>
#include <boost/utility/in_place_factory.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLRenderTargetImpl.h"

#include "GLCapabilities.h"
#include "GLContext.h"
#include "GLRenderer.h"
#include "GLUtils.h"
#include "GLViewport.h"
#include "OpenGLException.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


bool
GPlatesOpenGL::GLRenderTargetImpl::is_supported(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer,
		bool include_stencil_buffer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Require support for framebuffer objects.
	if (!capabilities.framebuffer.gl_EXT_framebuffer_object)
	{
		return false;
	}

	const GLuint render_target_test_dimension = 64;

	// Classify our frame buffer object according to texture format/dimensions, etc.
	GLFrameBufferObject::Classification frame_buffer_object_classification;
	frame_buffer_object_classification.set_dimensions(render_target_test_dimension, render_target_test_dimension);
	frame_buffer_object_classification.set_texture_internal_format(texture_internalformat);
	if (include_stencil_buffer)
	{
		// We need support for GL_EXT_packed_depth_stencil because, for the most part, consumer
		// hardware only supports stencil for FBOs if it's packed in with depth.
		if (!capabilities.framebuffer.gl_EXT_packed_depth_stencil)
		{
			return false;
		}

		// With GL_EXT_packed_depth_stencil both depth and stencil share the same render buffer.
		// And both must be enabled for the frame buffer completeness check to succeed.
		frame_buffer_object_classification.set_stencil_buffer_internal_format(GL_DEPTH24_STENCIL8_EXT);
		frame_buffer_object_classification.set_depth_buffer_internal_format(GL_DEPTH24_STENCIL8_EXT);
	}
	else if (include_depth_buffer)
	{
		// To improve render buffer re-use we use packed depth/stencil (if supported)
		// though only depth was requested...
		if (capabilities.framebuffer.gl_EXT_packed_depth_stencil)
		{
			// With GL_EXT_packed_depth_stencil both depth and stencil share the same render buffer.
			// And both must be enabled for the frame buffer completeness check to succeed.
			frame_buffer_object_classification.set_depth_buffer_internal_format(GL_DEPTH24_STENCIL8_EXT);
			frame_buffer_object_classification.set_stencil_buffer_internal_format(GL_DEPTH24_STENCIL8_EXT);
		}
		else
		{
			frame_buffer_object_classification.set_depth_buffer_internal_format(GL_DEPTH_COMPONENT);
		}
	}

	//! Typedef for a mapping of supported parameters (key) to boolean flag indicating supported.
	typedef std::map<GLFrameBufferObject::Classification::tuple_type, bool> supported_map_type;

	static supported_map_type supported;

	// Parameters to lookup supported map.
	const GLFrameBufferObject::Classification::tuple_type supported_key =
			frame_buffer_object_classification.get_tuple();

	// Only test for support the first time we're called with these parameters.
	if (supported.find(supported_key) == supported.end())
	{
		// By default unsupported unless we make it through the following tests.
		supported[supported_key] = false;

		// Make sure we leave the OpenGL state the way it was.
		GLRenderer::StateBlockScope save_restore_state_scope(renderer);

		// Try a small render target so we can check the framebuffer status and make sure
		// the internal formats of texture and depth buffer are compatible.
		// GL_EXT_framebuffer_object is fairly strict there (GL_ARB_framebuffer_object is better
		// but not supported on as much hardware).
		GLRenderTargetImpl render_target(
				renderer,
				texture_internalformat,
				include_depth_buffer,
				include_stencil_buffer);

		// Make sure we allocate storage first.
		render_target.set_render_target_dimensions(
				renderer,
				render_target_test_dimension,
				render_target_test_dimension);
		render_target.begin_render(renderer);
		render_target.end_render(renderer);

		// Now that we've attached the texture (and optional depth/stencil buffer) to the framebuffer
		// object we need to check for framebuffer completeness.
		if (!renderer.get_context().get_non_shared_state()->check_framebuffer_object_completeness(
				renderer,
				// Get access to the internal framebuffer object associated with the current OpenGL context...
				render_target.get_frame_buffer_object(renderer),
				frame_buffer_object_classification))
		{
			return false;
		}

		// If we get this far then we have support.
		supported[supported_key] = true;
	}

	return supported[supported_key];
}


GPlatesOpenGL::GLRenderTargetImpl::GLRenderTargetImpl(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer,
		bool include_stencil_buffer) :
	d_texture(GLTexture::create(renderer)),
	d_texture_internalformat(texture_internalformat),
	d_allocated_storage(false)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Create the depth/stencil buffers if requested.
	if (include_stencil_buffer)
	{
		// This should have been tested in 'is_supported()'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				capabilities.framebuffer.gl_EXT_packed_depth_stencil,
				GPLATES_ASSERTION_SOURCE);

		// With GL_EXT_packed_depth_stencil both depth and stencil share the same render buffer.
		// And both must be enabled for the frame buffer completeness check to succeed.
		const GLRenderBufferObject::shared_ptr_type depth_stencil_render_buffer =
				GLRenderBufferObject::create(renderer);
		d_stencil_buffer = boost::in_place(depth_stencil_render_buffer, GL_DEPTH24_STENCIL8_EXT);
		d_depth_buffer = boost::in_place(depth_stencil_render_buffer, GL_DEPTH24_STENCIL8_EXT);
	}
	else if (include_depth_buffer)
	{
		// To improve render buffer re-use we use packed depth/stencil (if supported)
		// though only depth was requested...
		if (capabilities.framebuffer.gl_EXT_packed_depth_stencil)
		{
			// With GL_EXT_packed_depth_stencil both depth and stencil share the same render buffer.
			// And both must be enabled for the frame buffer completeness check to succeed.
			const GLRenderBufferObject::shared_ptr_type depth_stencil_render_buffer =
					GLRenderBufferObject::create(renderer);
			d_depth_buffer = boost::in_place(depth_stencil_render_buffer, GL_DEPTH24_STENCIL8_EXT);
			d_stencil_buffer = boost::in_place(depth_stencil_render_buffer, GL_DEPTH24_STENCIL8_EXT);
		}
		else
		{
			d_depth_buffer = boost::in_place(GLRenderBufferObject::create(renderer), GL_DEPTH_COMPONENT);
		}
	}

	//
	// Set up the texture.
	//
	// TODO: Allow client to set these parameters. For example, may not want nearest filtering if
	// *not* representing a full-screen texture (full-screen usually point sampled).

	d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
	{
		d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Note that we've not yet allocated storage for either the texture or depth buffer yet.
	// That happens in 'begin_render()'.

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLRenderTargetImpl::set_render_target_dimensions(
		GLRenderer &renderer,
		unsigned int render_target_width,
		unsigned int render_target_height)
{
	GPlatesGlobal::Assert<OpenGLException>(
			!is_currently_rendering(),
			GPLATES_ASSERTION_SOURCE,
			"GLRenderTargetImpl: 'set_render_target_dimensions()' called between 'begin_render()' and 'end_render()'.");

	const GLCapabilities &capabilities = renderer.get_capabilities();

	// Truncate render target dimensions if exceeds maximum texture size.
	// We emit a warning if this happens because this is not tested in 'is_supported()' since
	// GLScreenRenderTarget uses us and varies the dimensions as the screen is resized.
	if (render_target_width > capabilities.texture.gl_max_texture_size)
	{
		qWarning() << "Render target width exceeds maximum texture size: truncating width.";
		render_target_width = capabilities.texture.gl_max_texture_size;
	}
	if (render_target_height > capabilities.texture.gl_max_texture_size)
	{
		qWarning() << "Render target height exceeds maximum texture size: truncating height.";
		render_target_height = capabilities.texture.gl_max_texture_size;
	}

	// Ensure the texture and render buffer have been allocated and
	// their dimensions match the client's dimensions.
	if (!d_allocated_storage ||
		render_target_width != d_texture->get_width().get() ||
		render_target_height != d_texture->get_height().get())
	{
		// Allocate the texture storage of the requested dimensions.
		//
		// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
		// just use values that are compatible with all internal formats to avoid a possible error.
		d_texture->gl_tex_image_2D(
				renderer, GL_TEXTURE_2D, 0, d_texture_internalformat,
				render_target_width, render_target_height,
				0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		// Allocate the stencil buffer storage of the requested dimensions.
		if (d_stencil_buffer)
		{
			d_stencil_buffer->render_buffer->gl_render_buffer_storage(
					renderer, d_stencil_buffer->internalformat, render_target_width, render_target_height);
		}
		// Allocate the depth buffer storage of the requested dimensions.
		if (d_depth_buffer)
		{
			// If the stencil and depth buffer share the same buffer then no need to allocate storage again.
			if (!d_stencil_buffer ||
				d_stencil_buffer->render_buffer != d_depth_buffer->render_buffer)
			{
				d_depth_buffer->render_buffer->gl_render_buffer_storage(
						renderer, d_depth_buffer->internalformat, render_target_width, render_target_height);
			}
		}
	}

	d_allocated_storage = true;
}


void
GPlatesOpenGL::GLRenderTargetImpl::begin_render(
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<OpenGLException>(
			d_allocated_storage,
			GPLATES_ASSERTION_SOURCE,
			"GLRenderTargetImpl: 'set_render_target_dimensions()' was not called before 'begin_render()'.");

	GPlatesGlobal::Assert<OpenGLException>(
			!is_currently_rendering(),
			GPLATES_ASSERTION_SOURCE,
			"GLRenderTargetImpl: 'begin_render()' called twice without an intervening 'end_render()'.");

	// Get the currently bound framebuffer object (if any).
	boost::optional<GLFrameBufferObject::shared_ptr_to_const_type> previous_framebuffer =
			renderer.gl_get_bind_frame_buffer();

	// Record the render info now that we're in a 'begin_render()' and 'end_render()' pair.
	d_current_render_info = boost::in_place(previous_framebuffer);

	// Get the OpenGL context-specific state (framebuffer object) for the current OpenGL context.
	ContextObjectState &context_object_state = get_object_state_for_current_context(renderer);

	// If first time using current OpenGL context then attach to associated framebuffer object.
	if (!context_object_state.attached_to_framebuffer)
	{
		// Attach the texture to the framebuffer object.
		context_object_state.framebuffer->gl_attach_texture_2D(
				renderer, GL_TEXTURE_2D, d_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);

		// Attach the depth buffer to the framebuffer object.
		if (d_depth_buffer)
		{
			context_object_state.framebuffer->gl_attach_render_buffer(
					renderer, d_depth_buffer->render_buffer, GL_DEPTH_ATTACHMENT_EXT);
		}
		// Attach the stencil buffer to the framebuffer object.
		if (d_stencil_buffer)
		{
			context_object_state.framebuffer->gl_attach_render_buffer(
					renderer, d_stencil_buffer->render_buffer, GL_STENCIL_ATTACHMENT_EXT);
		}

		context_object_state.attached_to_framebuffer = true;
	}

	// Bind our framebuffer.
	renderer.gl_bind_frame_buffer(context_object_state.framebuffer);
}


void
GPlatesOpenGL::GLRenderTargetImpl::end_render(
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<OpenGLException>(
			is_currently_rendering(),
			GPLATES_ASSERTION_SOURCE,
			"GLRenderTargetImpl: 'end_render()' called without a matching 'begin_render()'.");

	// Re-bind the previous framebuffer object (if any).
	if (d_current_render_info->previous_framebuffer)
	{
		renderer.gl_bind_frame_buffer(d_current_render_info->previous_framebuffer.get());
	}
	else
	{
		// Return to the main framebuffer.
		renderer.gl_unbind_frame_buffer();
	}

	// No longer in a 'begin_render()' / 'end_render()' pair.
	d_current_render_info = boost::none;
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLRenderTargetImpl::get_texture() const
{
	// Must not currently be rendering because that means the client could be trying to use
	// the same texture they're currently rendering to.
	GPlatesGlobal::Assert<OpenGLException>(
			!is_currently_rendering(),
			GPLATES_ASSERTION_SOURCE,
			"GLRenderTargetImpl::get_texture: cannot use texture while rendering to it.");

	return d_texture;
}


GPlatesOpenGL::GLRenderTargetImpl::ContextObjectState &
GPlatesOpenGL::GLRenderTargetImpl::get_object_state_for_current_context(
		GLRenderer &renderer)
{
	const GLContext &current_context = renderer.get_context();

	context_object_state_seq_type::iterator context_object_state_iter = d_context_object_states.begin();
	context_object_state_seq_type::iterator context_object_state_end = d_context_object_states.end();
	for ( ; context_object_state_iter != context_object_state_end; ++context_object_state_iter)
	{
		if (context_object_state_iter->context == &current_context)
		{
			return *context_object_state_iter;
		}
	}

	// Context not yet encountered so create a new context object state.
	d_context_object_states.push_back(ContextObjectState(current_context, renderer));
	return d_context_object_states.back();
}


GPlatesOpenGL::GLRenderTargetImpl::ContextObjectState::ContextObjectState(
		const GLContext &context_,
		GLRenderer &renderer_) :
	context(&context_),
	// Create a framebuffer object associated with the context...
	framebuffer(GLFrameBufferObject::create(renderer_)),
	attached_to_framebuffer(false)
{
}
