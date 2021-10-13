/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_OPENGL_GLTEXTUREUTILS_H
#define GPLATES_OPENGL_GLTEXTUREUTILS_H

#include <QColor>
#include <QImage>
#include <QRect>
#include <QString>

#include "GLMatrix.h"
#include "GLTexture.h"
#include "GLTextureResource.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	namespace GLTextureUtils
	{
		/**
		 * Loads the specified region of the RGBA texture with a single colour.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * NOTE: This will bind @a texture to whatever the currently active texture unit is.
		 */
		void
		load_colour_into_texture(
				const GLTexture::shared_ptr_type &texture,
				const GPlatesGui::rgba8_t &colour,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified image into the specified RGBA texture.
		 *
		 * @a image must contains 4-byte (R,G,B,A) colour values in that order.
		 * Note that this is a byte ordering in *memory* (not in a 32-bit integer which
		 * is machine-endian dependent).
		 *
		 * NOTE: It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 * It is also the caller's responsibility to ensure that @a image points
		 * to @a image_width by @a image_height colour values.
		 *
		 * NOTE: This will bind @a texture to whatever the currently active texture unit is.
		 */
		void
		load_rgba8_image_into_texture(
				const GLTexture::shared_ptr_type &texture,
				const void *image,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified RGBA8 image into the specified RGBA texture.
		 *
		 * NOTE: It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 * It is also the caller's responsibility to ensure that @a image points
		 * to @a image_width by @a image_height colour values.
		 *
		 * NOTE: This will bind @a texture to whatever the currently active texture unit is.
		 */
		void
		load_rgba8_image_into_texture(
				const GLTexture::shared_ptr_type &texture,
				const GPlatesGui::rgba8_t *image,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified QImage, must be QImage::Format_ARGB32, into the specified texture.
		 *
		 * NOTE: It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * NOTE: This will bind @a texture to whatever the currently active texture unit is.
		 */
		void
		load_argb32_qimage_into_texture(
				const GLTexture::shared_ptr_type &texture,
				const QImage &argb32_qimage,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Draws the specified text into a QImage the specified size.
		 */
		QImage
		draw_text_into_qimage(
				const QString &text,
				unsigned int image_width,
				unsigned int image_height,
				const float text_scale = 1.0f,
				const QColor &text_colour = QColor(255, 255, 255, 255)/*white*/,
				const QColor &background_colour = QColor(0, 0, 0, 255)/*black*/);


		/**
		 * Creates a new 4x4 texel clip texture whose centre 2x2 texels are white with the
		 * remaining texels black (including alpha channel).
		 */
		GLTexture::shared_ptr_type
		create_xy_clip_texture(
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager);


		/**
		 * Initialise clip texture transform to convert the clip-space range [-1, 1] to
		 * range [0.25, 0.75] to map to the interior 2x2 texel region of the 4x4 clip texture.
		 */
		const GLMatrix &
		get_clip_texture_clip_space_to_texture_space_transform();
	}
}

#endif // GPLATES_OPENGL_GLTEXTUREUTILS_H
