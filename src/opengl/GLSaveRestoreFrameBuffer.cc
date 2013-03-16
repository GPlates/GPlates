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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLSaveRestoreFrameBuffer.h"

#include "GLCompiledDrawState.h"
#include "GLContext.h"
#include "GLRenderer.h"
#include "OpenGLException.h"

#include "global/GPlatesAssert.h"

#include "utils/Base2Utils.h"


GPlatesOpenGL::GLSaveRestoreFrameBuffer::GLSaveRestoreFrameBuffer(
		unsigned int save_restore_width,
		unsigned int save_restore_height,
		GLint save_restore_texture_internalformat) :
	d_save_restore_width(save_restore_width),
	d_save_restore_height(save_restore_height),
	d_save_restore_texture_internal_format(save_restore_texture_internalformat)
{
}


void
GPlatesOpenGL::GLSaveRestoreFrameBuffer::save(
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<OpenGLException>(
			!between_save_and_restore(),
			GPLATES_ASSERTION_SOURCE,
			"GLSaveRestoreFrameBuffer: 'save()' called between 'save()' and 'restore()'.");

	// Acquire a cached texture for saving the framebuffer to.
	// It'll get returned to its cache when we no longer reference it.
	d_save_restore_texture =
			renderer.get_context().get_shared_state()->acquire_texture(
					renderer,
					GL_TEXTURE_2D,
					d_save_restore_texture_internal_format,
					// The texture dimensions used to save restore the render target portion of the
					// framebuffer. The dimensions are expanded from the client-specified
					// viewport width/height as necessary to match a power-of-two save/restore texture.
					// We use power-of-two since non-power-of-two textures are probably not supported
					// and also (due to the finite number of power-of-two dimensions) we have more
					// chance of re-using an existing texture (rather than acquiring a new one).
					GPlatesUtils::Base2::next_power_of_two(d_save_restore_width),
					GPlatesUtils::Base2::next_power_of_two(d_save_restore_height));

	// 'acquire_texture' initialises the texture memory (to empty) but does not set the filtering
	// state when it creates a new texture.
	// Also even if the texture was cached it might have been used by another client that specified
	// different filtering settings for it.
	// So we set the filtering settings each time we acquire.
	d_save_restore_texture.get()->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	d_save_restore_texture.get()->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Turn off anisotropic filtering (don't need it).
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		d_save_restore_texture.get()->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
	}
	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_save_restore_texture.get()->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		d_save_restore_texture.get()->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_save_restore_texture.get()->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		d_save_restore_texture.get()->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	//
	// Save the portion of the framebuffer used as a render target so we can restore it later.
	//

	// We don't want any state changes made here to interfere with the client's state changes.
	// So save the current state and revert back to it at the end of this scope.
	// We don't need to reset to the default OpenGL state because very little state
	// affects glCopyTexSubImage2D so it doesn't matter what the current OpenGL state is.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	renderer.gl_bind_texture(d_save_restore_texture.get(), GL_TEXTURE0, GL_TEXTURE_2D);

	// Copy the portion of the framebuffer (requested to save) to the backup texture.
	renderer.gl_copy_tex_sub_image_2D(
			GL_TEXTURE0,
			GL_TEXTURE_2D,
			0/*level*/,
			0, 0,
			0, 0,
			d_save_restore_width,
			d_save_restore_height);
}


void
GPlatesOpenGL::GLSaveRestoreFrameBuffer::restore(
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<OpenGLException>(
			between_save_and_restore(),
			GPLATES_ASSERTION_SOURCE,
			"GLSaveRestoreFrameBuffer: 'restore()' called without a matching 'save()'.");

	// NOTE: We (temporarily) reset to the default OpenGL state since we need to draw a
	// save/restore size quad into the framebuffer with the save/restore texture applied.
	// And we don't know what state has already been set.
	GLRenderer::StateBlockScope save_restore_state(renderer, true/*reset_to_default_state*/);

	// Disable depth writing for render targets otherwise the framebuffer's depth buffer would get corrupted.
	renderer.gl_depth_mask(GL_FALSE);

	//
	// Restore the portion of the framebuffer that was saved.
	//

	// Bind the save restore texture to use for rendering.
	renderer.gl_bind_texture(d_save_restore_texture.get(), GL_TEXTURE0, GL_TEXTURE_2D);

	// Set up to render using the texture.
	renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
	renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Scale the texture coordinates to account for the fact that we're only writing part of the
	// save/restore texture to the framebuffer (only the part that was saved).
	GLMatrix texture_coord_scale;
	texture_coord_scale.gl_scale(
			double(d_save_restore_width) / d_save_restore_texture.get()->get_width().get(),
			double(d_save_restore_height) / d_save_restore_texture.get()->get_height().get(),
			1);
	renderer.gl_load_texture_matrix(GL_TEXTURE0, texture_coord_scale);

	// We only want to draw the full-screen quad into the part of the framebuffer that was saved.
	// The remaining area of the framebuffer should not be touched.
	// NOTE: The viewport does *not* clip (eg, fat points whose centres are inside the viewport
	// can be rendered outside the viewport bounds due to the fatness) but in our case we're only
	// copying a texture so we don't need to worry - if we did need to worry then we would specify
	// a scissor rectangle also.
	renderer.gl_viewport(
			0, 0,
			d_save_restore_width,
			d_save_restore_height);

	//
	// Draw a save/restore sized quad into the framebuffer.
	// This restores that part of the framebuffer used to generate render-textures.
	//

	// Get the full-screen quad.
	const GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad =
			renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer);

	// Draw the full-screen quad into the save/restore sized viewport.
	renderer.apply_compiled_draw_state(*full_screen_quad);

	// Release the save/restore texture.
	d_save_restore_texture = boost::none;
}
