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

#ifndef GPLATES_OPENGL_GLSAVERESTOREFRAMEBUFFER_H
#define GPLATES_OPENGL_GLSAVERESTOREFRAMEBUFFER_H

#include <vector>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLPixelBuffer.h"
#include "GLTexture.h"
#include "GLTileRender.h"


namespace GPlatesOpenGL
{
	class GLCapabilities;
	class GLRenderer;

	/**
	 * Copies the currently bound colour framebuffer (and optionally depth and stencil buffers)
	 * to a temporary texture and subsequently restores framebuffer from that texture.
	 *
	 * This enables the framebuffer to be used as a render target without losing its original contents.
	 * Note that this is only really useful for the main framebuffer - when GL_EXT_framebuffer_object
	 * is supported then this ability is not very useful since framebuffer objects can be used to
	 * render directly to a texture.
	 */
	class GLSaveRestoreFrameBuffer
	{
	public:

		/**
		 * Specify the save/restore dimensions.
		 *
		 * Note that the internal colour texture (and optional depth/stencil buffers) are not acquired
		 * until @a save and then released at @a restore.
		 *
		 * NOTE: You should not draw to the framebuffer (between @a save and @a restore) outside of
		 * the specified dimensions. To ensure this you can enable the scissor test and specify a
		 * scissor rectangle with these dimensions.
		 */
		GLSaveRestoreFrameBuffer(
				const GLCapabilities &capabilities,
				unsigned int save_restore_width,
				unsigned int save_restore_height,
				GLint save_restore_colour_texture_internalformat = GL_RGBA8,
				bool save_restore_depth_buffer = false,
				bool save_restore_stencil_buffer = false);


		/**
		 * Saves the currently bound (colour) framebuffer to a temporary internal texture of
		 * power-of-two dimensions large enough to contain the specified save/restore dimensions.
		 *
		 * NOTE: You should not draw to the frame buffer outside of the specified dimensions.
		 * For example, by enabling scissor test and specifying a scissor rectangle with these dimensions
		 * after calling @a save to avoid corrupting frame buffer outside of save/restore region.
		 */
		void
		save(
				GLRenderer &renderer);


		/**
		 * Restores the (colour) framebuffer to its contents prior to the GLSaveRestoreFrameBuffer constructor.
		 *
		 * NOTE: This temporarily resets OpenGL to the default state and hence ignores any scissoring.
		 * In other words the entire saved region is always restored regardless of scissoring.
		 */
		void
		restore(
				GLRenderer &renderer);

	private:

		/**
		 * The save/restore colour textures and depth/stencil pixel buffers.
		 */
		struct SaveRestore
		{
			// May need multiple textures if frame buffer larger than maximum texture dimensions.
			std::vector<GLTexture::shared_ptr_type> colour_textures;

			// One pixel buffer suffices to capture depth/stencil any size frame buffer.
			GLPixelBuffer::shared_ptr_type depth_pixel_buffer;
			GLPixelBuffer::shared_ptr_type stencil_pixel_buffer;
		};


		unsigned int d_save_restore_frame_buffer_width;
		unsigned int d_save_restore_frame_buffer_height;

		unsigned int d_save_restore_texture_width;
		unsigned int d_save_restore_texture_height;
		GLint d_save_restore_colour_texture_internal_format;

		/**
		 * We use a tile render in case the save/restore dimensions are larger than the
		 * maximum texture dimensions - in which case multiple save/restore textures are needed -.
		 * this should never happen though (but it might for really old hardware with tiny maximum
		 * texture dimensions.
		 */
		GLTileRender d_save_restore_texture_tile_render;

		/**
		 * Size, in bytes, of save/restore pixel buffer for depth values.
		 *
		 * Is boost::none if not saving/restoring depth buffer.
		 */
		boost::optional<unsigned int> d_save_restore_depth_pixel_buffer_size;

		/**
		 * Size, in bytes, of save/restore pixel buffer for stencil values.
		 *
		 * Is boost::none if not saving/restoring stencil buffer.
		 */
		boost::optional<unsigned int> d_save_restore_stencil_pixel_buffer_size;

		/**
		 * One (or more) save/restore colour textures (and optional depth/stencil pixel buffers)
		 * that span the framebuffer.
		 *
		 * More than one texture is only needed if the maximum texture dimensions are not enough
		 * to cover the current framebuffer dimensions.
		 */
		boost::optional<SaveRestore> d_save_restore;


		/**
		 * Returns true if between @a save and @a restore.
		 */
		bool
		between_save_and_restore() const
		{
			return d_save_restore;
		}

		/**
		 * Acquire one save/restore colour texture.
		 */
		GLTexture::shared_ptr_type
		acquire_save_restore_colour_texture(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLSAVERESTOREFRAMEBUFFER_H
