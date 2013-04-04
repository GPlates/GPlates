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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/Base2Utils.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Returns the next power-of-two dimension greater-than-or-equal to @a save_restore_dimension.
		 *
		 * The texture dimensions used to save restore the render target portion of the
		 * framebuffer. The dimensions are expanded from the client-specified
		 * viewport width/height as necessary to match a power-of-two save/restore texture.
		 * We use power-of-two since non-power-of-two textures are probably not supported
		 * and also (due to the finite number of power-of-two dimensions) we have more
		 * chance of re-using an existing texture (rather than acquiring a new one).
		 *
		 * Limits to the maximum texture dimension if necessary.
		 */
		unsigned int
		get_power_of_two_save_restore_dimension(
				const GLCapabilities &capabilities,
				unsigned int save_restore_dimension)
		{
			save_restore_dimension = GPlatesUtils::Base2::next_power_of_two(save_restore_dimension);

			// Must not exceed maximum texture dimensions.
			return (save_restore_dimension > capabilities.texture.gl_max_texture_size)
					? capabilities.texture.gl_max_texture_size
					: save_restore_dimension;
		}
	}
}

GPlatesOpenGL::GLSaveRestoreFrameBuffer::GLSaveRestoreFrameBuffer(
		const GLCapabilities &capabilities,
		unsigned int save_restore_width,
		unsigned int save_restore_height,
		GLint save_restore_colour_texture_internalformat,
		bool save_restore_depth_buffer,
		bool save_restore_stencil_buffer) :
	d_save_restore_frame_buffer_width(save_restore_width),
	d_save_restore_frame_buffer_height(save_restore_height),
	d_save_restore_texture_width(
			get_power_of_two_save_restore_dimension(capabilities, save_restore_width)),
	d_save_restore_texture_height(
			get_power_of_two_save_restore_dimension(capabilities, save_restore_height)),
	d_save_restore_colour_texture_internal_format(save_restore_colour_texture_internalformat),
	d_save_restore_texture_tile_render(
			// This could be less than 'save_restore_width'...
			d_save_restore_texture_width,
			// This could be less than 'save_restore_height'...
			d_save_restore_texture_height,
			// The part of the framebuffer we are saving/restoring...
			GLViewport(0, 0, save_restore_width, save_restore_height))
{
	if (save_restore_depth_buffer)
	{
		d_save_restore_depth_pixel_buffer_size = sizeof(GLfloat)
				// We use power-of-two dimensions since (due to the finite number of
				// power-of-two dimensions) we have more chance of re-using a pixel buffer...
				* GPlatesUtils::Base2::next_power_of_two(save_restore_width)
				* GPlatesUtils::Base2::next_power_of_two(save_restore_height);
	}
	if (save_restore_stencil_buffer)
	{
		d_save_restore_stencil_pixel_buffer_size = sizeof(GLubyte)
				// We use power-of-two dimensions since (due to the finite number of
				// power-of-two dimensions) we have more chance of re-using a pixel buffer...
				* GPlatesUtils::Base2::next_power_of_two(save_restore_width)
				* GPlatesUtils::Base2::next_power_of_two(save_restore_height);
	}
}


void
GPlatesOpenGL::GLSaveRestoreFrameBuffer::save(
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<OpenGLException>(
			!between_save_and_restore(),
			GPLATES_ASSERTION_SOURCE,
			"GLSaveRestoreFrameBuffer: 'save()' called between 'save()' and 'restore()'.");

	// Sequence of save/restore textures/buffers.
	d_save_restore = SaveRestore();

	//
	// Save the portion of the framebuffer used as a render target so we can restore it later.
	//

	// We don't want any state changes made here to interfere with the client's state changes.
	// So save the current state and revert back to it at the end of this scope.
	// We don't need to reset to the default OpenGL state because very little state affects
	// glCopyTexSubImage2D and glReadPixels so it doesn't matter what the current OpenGL state is.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	//
	// Save the (colour) framebuffer tile-by-tile into textures.
	//

	for (d_save_restore_texture_tile_render.first_tile();
		!d_save_restore_texture_tile_render.finished();
		d_save_restore_texture_tile_render.next_tile())
	{
		GLViewport tile_save_restore_texture_viewport;
		d_save_restore_texture_tile_render.get_tile_source_viewport(tile_save_restore_texture_viewport);

		GLViewport tile_save_restore_frame_buffer_viewport;
		d_save_restore_texture_tile_render.get_tile_destination_viewport(tile_save_restore_frame_buffer_viewport);

		// Acquire a save/restore texture for the current tile.
		GLTexture::shared_ptr_type save_restore_texture = acquire_save_restore_colour_texture(renderer);
		d_save_restore->colour_textures.push_back(save_restore_texture);

		renderer.gl_bind_texture(save_restore_texture, GL_TEXTURE0, GL_TEXTURE_2D);

		// Copy the portion of the framebuffer (requested to save) to the backup texture.
		renderer.gl_copy_tex_sub_image_2D(
				GL_TEXTURE0,
				GL_TEXTURE_2D,
				0/*level*/,
				tile_save_restore_texture_viewport.x(),
				tile_save_restore_texture_viewport.y(),
				tile_save_restore_frame_buffer_viewport.x(),
				tile_save_restore_frame_buffer_viewport.y(),
				tile_save_restore_frame_buffer_viewport.width(),
				tile_save_restore_frame_buffer_viewport.height());
	}

	// If saving depth or stencil buffer...
	if (d_save_restore_depth_pixel_buffer_size ||
		d_save_restore_stencil_pixel_buffer_size)
	{
		glPixelStorei(GL_PACK_ALIGNMENT, 1);

		//
		// Save the depth framebuffer into a pixel buffer.
		//

		if (d_save_restore_depth_pixel_buffer_size)
		{
			// Acquire a cached pixel buffer for saving the depth framebuffer to.
			// It'll get returned to its cache when we no longer reference it.
			d_save_restore->depth_pixel_buffer =
					renderer.get_context().get_shared_state()->acquire_pixel_buffer(
							renderer,
							d_save_restore_depth_pixel_buffer_size.get(),
							// Copying from frame buffer to pixel buffer and back again...
							GLBuffer::USAGE_STREAM_COPY);

			// Pack depth frame buffer into pixel buffer.
			d_save_restore->depth_pixel_buffer->gl_bind_pack(renderer);
			d_save_restore->depth_pixel_buffer->gl_read_pixels(
					renderer,
					0/*x*/,
					0/*y*/,
					d_save_restore_frame_buffer_width,
					d_save_restore_frame_buffer_height,
					GL_DEPTH_COMPONENT,
					GL_FLOAT,
					0/*offset*/);
		}

		//
		// Save the stencil framebuffer into a pixel buffer.
		//

		if (d_save_restore_stencil_pixel_buffer_size)
		{
			// Acquire a cached pixel buffer for saving the stencil framebuffer to.
			// It'll get returned to its cache when we no longer reference it.
			d_save_restore->stencil_pixel_buffer =
					renderer.get_context().get_shared_state()->acquire_pixel_buffer(
							renderer,
							d_save_restore_stencil_pixel_buffer_size.get(),
							// Copying from frame buffer to pixel buffer and back again...
							GLBuffer::USAGE_STREAM_COPY);

			// Pack stencil frame buffer into pixel buffer.
			d_save_restore->stencil_pixel_buffer->gl_bind_pack(renderer);
			d_save_restore->stencil_pixel_buffer->gl_read_pixels(
					renderer,
					0/*x*/,
					0/*y*/,
					d_save_restore_frame_buffer_width,
					d_save_restore_frame_buffer_height,
					GL_STENCIL_INDEX,
					GL_UNSIGNED_BYTE,
					0/*offset*/);
		}

		// Restore to default value since calling OpenGL directly instead of using GLRenderer.
		// FIXME: Shouldn't really be making direct calls to OpenGL - transfer to GLRenderer.
		glPixelStorei(GL_PACK_ALIGNMENT, 4);
	}
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
	// Also, if we save/restore depth or stencil, then we use 'glDrawPixels' which uses texturing
	// and all fragment operations and we don't know what the current state is.
	GLRenderer::StateBlockScope save_restore_state(renderer, true/*reset_to_default_state*/);

	// If restoring depth or stencil buffer.
	// We do this before restoring colour buffer since it's easier to manage OpenGL state.
	if (d_save_restore_depth_pixel_buffer_size ||
		d_save_restore_stencil_pixel_buffer_size)
	{
		// Avoid drawing to the colour buffer and avoid depth testing when writing to depth buffer.
		renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		renderer.gl_enable(GL_DEPTH_TEST, false);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		//
		// Save the depth framebuffer into a pixel buffer.
		//

		if (d_save_restore_depth_pixel_buffer_size)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_save_restore->depth_pixel_buffer,
					GPLATES_ASSERTION_SOURCE);

			// Disable stencil writes and enable depth writes.
			renderer.gl_stencil_mask(0);
			renderer.gl_depth_mask(GL_TRUE);

			// Unpack depth pixel buffer into frame buffer.
			d_save_restore->depth_pixel_buffer->gl_bind_unpack(renderer);
			d_save_restore->depth_pixel_buffer->gl_draw_pixels(
					renderer,
					0/*x*/,
					0/*y*/,
					d_save_restore_frame_buffer_width,
					d_save_restore_frame_buffer_height,
					GL_DEPTH_COMPONENT,
					GL_FLOAT,
					0/*offset*/);
		}

		//
		// Save the stencil framebuffer into a pixel buffer.
		//

		if (d_save_restore_stencil_pixel_buffer_size)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_save_restore->stencil_pixel_buffer,
					GPLATES_ASSERTION_SOURCE);

			// Disable depth writes and enable stencil writes.
			renderer.gl_depth_mask(GL_FALSE);
			renderer.gl_stencil_mask(~GLuint(0)/*all ones*/);

			// Unpack stencil pixel buffer into frame buffer.
			d_save_restore->stencil_pixel_buffer->gl_bind_unpack(renderer);
			d_save_restore->stencil_pixel_buffer->gl_draw_pixels(
					renderer,
					0/*x*/,
					0/*y*/,
					d_save_restore_frame_buffer_width,
					d_save_restore_frame_buffer_height,
					GL_STENCIL_INDEX,
					GL_UNSIGNED_BYTE,
					0/*offset*/);
		}

		// Restore to default value since calling OpenGL directly instead of using GLRenderer.
		// FIXME: Shouldn't really be making direct calls to OpenGL - transfer to GLRenderer.
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}

	// Re-enable colour writes.
	renderer.gl_color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Disable depth and stencil writes to avoid overwriting restored depth and stencil buffers.
	renderer.gl_depth_mask(GL_FALSE);
	renderer.gl_stencil_mask(0);

	// Avoid avoid depth testing when writing to colour buffer.
	renderer.gl_enable(GL_DEPTH_TEST, false);

	//
	// Restore the portion of the framebuffer that was saved.
	//

	// Save the framebuffer tile-by-tile.
	unsigned int tile_index = 0;
	for (d_save_restore_texture_tile_render.first_tile();
		!d_save_restore_texture_tile_render.finished();
		d_save_restore_texture_tile_render.next_tile(), ++tile_index)
	{

		GLViewport tile_save_restore_texture_viewport;
		d_save_restore_texture_tile_render.get_tile_source_viewport(tile_save_restore_texture_viewport);

		GLViewport tile_save_restore_frame_buffer_viewport;
		d_save_restore_texture_tile_render.get_tile_destination_viewport(tile_save_restore_frame_buffer_viewport);

		// Get the save/restore texture for the current tile.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				tile_index < d_save_restore->colour_textures.size(),
				GPLATES_ASSERTION_SOURCE);
		GLTexture::shared_ptr_type save_restore_texture = d_save_restore->colour_textures[tile_index];

		// Bind the save restore texture to use for rendering.
		renderer.gl_bind_texture(save_restore_texture, GL_TEXTURE0, GL_TEXTURE_2D);

		// Set up to render using the texture.
		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		// Scale the texture coordinates to account for the fact that we're only writing part of the
		// save/restore texture to the framebuffer (only the part that was saved).
		GLMatrix texture_coord_scale;
		texture_coord_scale.gl_scale(
				double(tile_save_restore_texture_viewport.width()) / d_save_restore_texture_width,
				double(tile_save_restore_texture_viewport.height()) / d_save_restore_texture_height,
				1);
		renderer.gl_load_texture_matrix(GL_TEXTURE0, texture_coord_scale);

		// We only want to draw the full-screen quad into the part of the framebuffer that was saved.
		// The remaining area of the framebuffer should not be touched.
		// NOTE: The viewport does *not* clip (eg, fat points whose centres are inside the viewport
		// can be rendered outside the viewport bounds due to the fatness) but in our case we're only
		// copying a texture so we don't need to worry - if we did need to worry then we would specify
		// a scissor rectangle also.
		renderer.gl_viewport(
				tile_save_restore_frame_buffer_viewport.x(),
				tile_save_restore_frame_buffer_viewport.y(),
				tile_save_restore_frame_buffer_viewport.width(),
				tile_save_restore_frame_buffer_viewport.height());

		//
		// Draw a save/restore sized quad into the framebuffer.
		// This restores that part of the framebuffer used to generate render-textures.
		//

		// Get the full-screen quad.
		const GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad =
				renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer);

		// Draw the full-screen quad into the save/restore sized viewport.
		renderer.apply_compiled_draw_state(*full_screen_quad);
	}

	// Release the save/restore textures/buffers.
	d_save_restore = boost::none;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLSaveRestoreFrameBuffer::acquire_save_restore_colour_texture(
		GLRenderer &renderer)
{
	// Acquire a cached texture for saving (part or all of) the framebuffer to.
	// It'll get returned to its cache when we no longer reference it.
	const GLTexture::shared_ptr_type save_restore_texture =
			renderer.get_context().get_shared_state()->acquire_texture(
					renderer,
					GL_TEXTURE_2D,
					d_save_restore_colour_texture_internal_format,
					d_save_restore_texture_width,
					d_save_restore_texture_height);

	// 'acquire_texture' initialises the texture memory (to empty) but does not set the filtering
	// state when it creates a new texture.
	// Also even if the texture was cached it might have been used by another client that specified
	// different filtering settings for it.
	// So we set the filtering settings each time we acquire.
	save_restore_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	save_restore_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Turn off anisotropic filtering (don't need it).
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		save_restore_texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
	}
	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		save_restore_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		save_restore_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		save_restore_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		save_restore_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	return save_restore_texture;
}
