/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLScreenRenderTarget.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "GLUtils.h"
#include "GLViewport.h"
#include "OpenGLException.h"


bool
GPlatesOpenGL::GLScreenRenderTarget::is_supported(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer)
{
	static supported_map_type supported;

	// Parameters to lookup supported map.
	const supported_key_type supported_key(texture_internalformat, include_depth_buffer);

	// Only test for support the first time we're called with these parameters.
	if (supported.find(supported_key) == supported.end())
	{
		// By default unsupported unless we make it through the following tests.
		supported[supported_key] = false;

		const GLContext::Parameters &context_params = GLContext::get_parameters();

		// Require support for framebuffer objects.
		if (!context_params.framebuffer.gl_EXT_framebuffer_object)
		{
			return false;
		}

		// Make sure we leave the OpenGL state the way it was.
		GLRenderer::StateBlockScope save_restore_state_scope(renderer);

		// Try a small render target so we can check the framebuffer status and make sure
		// the internal formats of texture and depth buffer are compatible.
		// GL_EXT_framebuffer_object is fairly strict there (GL_ARB_framebuffer_object is better)
		// but not supported on as much hardware).
		GLScreenRenderTarget render_target(renderer, texture_internalformat, include_depth_buffer);

		// Make sure we allocate storage first.
		render_target.begin_render(renderer, 64, 64);
		render_target.end_render(renderer);

		// Get access to the internal framebuffer object associated with the current OpenGL context.
		if (!render_target.get_frame_buffer_object(renderer)->gl_check_frame_buffer_status(renderer))
		{
			return false;
		}

		// If we get this far then we have support.
		supported[supported_key] = true;
	}

	return supported[supported_key];
}


GPlatesOpenGL::GLScreenRenderTarget::GLScreenRenderTarget(
		GLRenderer &renderer,
		GLint texture_internalformat,
		bool include_depth_buffer) :
	d_texture(GLTexture::create(renderer)),
	d_texture_internalformat(texture_internalformat),
	d_allocated_storage(false),
	d_currently_rendering(false)
{
	if (include_depth_buffer)
	{
		d_depth_buffer = GLRenderBufferObject::create(renderer);
	}

	//
	// Set up the texture.
	//

	// It's a full-screen texture (meant to be point sampled) so nearest filtering is all that's needed.
	d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	d_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
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
GPlatesOpenGL::GLScreenRenderTarget::begin_render(
		GLRenderer &renderer,
		unsigned int render_target_width,
		unsigned int render_target_height)
{
	// Get the OpenGL context-specific state (framebuffer object) for the current OpenGL context.
	ContextObjectState &context_object_state = get_object_state_for_current_context(renderer);

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

		// Allocate the depth buffer storage of the requested dimensions.
		if (d_depth_buffer)
		{
			d_depth_buffer.get()->gl_render_buffer_storage(
					renderer, GL_DEPTH_COMPONENT, render_target_width, render_target_height);
		}
	}

	d_allocated_storage = true;

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
					renderer, d_depth_buffer.get(), GL_DEPTH_ATTACHMENT_EXT);
		}

		context_object_state.attached_to_framebuffer = true;
	}

	// Bind the framebuffer.
	renderer.gl_bind_frame_buffer(context_object_state.framebuffer);

	d_currently_rendering = true;
}


void
GPlatesOpenGL::GLScreenRenderTarget::end_render(
		GLRenderer &renderer)
{
	d_currently_rendering = false;

	// Return to targeting the main framebuffer.
	renderer.gl_unbind_frame_buffer();
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLScreenRenderTarget::get_texture() const
{
	// Must not currently be rendering because that means the client could be trying to use
	// the same texture they're currently rendering to.
	GPlatesGlobal::Assert<OpenGLException>(
			!d_currently_rendering,
			GPLATES_ASSERTION_SOURCE,
			"GLScreenRenderTarget::get_texture: cannot use texture while rendering to it.");

	return d_texture;
}


GPlatesOpenGL::GLScreenRenderTarget::ContextObjectState &
GPlatesOpenGL::GLScreenRenderTarget::get_object_state_for_current_context(
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


GPlatesOpenGL::GLScreenRenderTarget::ContextObjectState::ContextObjectState(
		const GLContext &context_,
		GLRenderer &renderer_) :
	context(&context_),
	// Create a framebuffer object associated with the context...
	framebuffer(GLFrameBufferObject::create(renderer_)),
	attached_to_framebuffer(false)
{
}
