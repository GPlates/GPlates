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


bool
GPlatesOpenGL::GLScreenRenderTarget::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		const GLContext::Parameters &context_params = GLContext::get_parameters();

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
		GLScreenRenderTarget render_target(renderer);

		// Make sure we allocate storage first.
		render_target.begin_render(renderer, 64, 64);
		render_target.end_render(renderer);
		if (!render_target.d_framebuffer->gl_check_frame_buffer_status(renderer))
		{
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


GPlatesOpenGL::GLScreenRenderTarget::GLScreenRenderTarget(
		GLRenderer &renderer) :
	d_framebuffer(GLFrameBufferObject::create(renderer)),
	d_texture(GLTexture::create(renderer)),
	d_depth_buffer(GLRenderBufferObject::create(renderer)),
	d_allocated_storage(false)
{
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
	// Ensure the texture and render buffer have been allocated and
	// their dimensions match the client's dimensions.
	if (!d_allocated_storage ||
		render_target_width != d_texture->get_width().get() ||
		render_target_height != d_texture->get_height().get())
	{
		// Allocate the texture storage of the requested dimensions.
		d_texture->gl_tex_image_2D(
				renderer, GL_TEXTURE_2D, 0, GL_RGBA8,
				render_target_width, render_target_height,
				0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		// Allocate the depth buffer storage of the requested dimensions.
		d_depth_buffer->gl_render_buffer_storage(
				renderer, GL_DEPTH_COMPONENT, render_target_width, render_target_height);

		// If first time allocating storage then also need to attach to framebuffer object.
		if (!d_allocated_storage)
		{
			// Attach the texture and depth buffer to the framebuffer object.
			d_framebuffer->gl_attach_texture_2D(
					renderer, GL_TEXTURE_2D, d_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);
			d_framebuffer->gl_attach_render_buffer(
					renderer, d_depth_buffer, GL_DEPTH_ATTACHMENT_EXT);
		}

		d_allocated_storage = true;
	}

	// Bind the framebuffer.
	renderer.gl_bind_frame_buffer(d_framebuffer);
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLScreenRenderTarget::end_render(
		GLRenderer &renderer)
{
	// Return to targeting the main framebuffer.
	renderer.gl_unbind_frame_buffer();

	return d_texture;
}
