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

#include <boost/scoped_array.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QGLWidget>
#include <QImage>
#include <QMatrix>
#include <QPainter>

#include "GLTextureUtils.h"

#include "GLUtils.h"

#include "utils/Profile.h"


void
GPlatesOpenGL::GLTextureUtils::load_colour_into_texture(
		const GLTexture::shared_ptr_type &texture,
		const GPlatesGui::rgba8_t &colour,
		unsigned int texel_width,
		unsigned int texel_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	const unsigned num_texels_to_load = texel_width * texel_height;

	// Create an array of same-colour entries.
	boost::scoped_array<GPlatesGui::rgba8_t> image_data_storage(
			new GPlatesGui::rgba8_t[num_texels_to_load]);
	GPlatesGui::rgba8_t *const image_data = image_data_storage.get();
	for (unsigned int n = 0; n < num_texels_to_load; ++n)
	{
		image_data[n] = colour;
	}

	// Load image into texture...
	load_rgba8_image_into_texture(
			texture,
			image_data_storage.get(),
			texel_width, texel_height,
			texel_u_offset, texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::load_rgba8_image_into_texture(
		const GLTexture::shared_ptr_type &texture,
		const void *image,
		unsigned int image_width,
		unsigned int image_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	PROFILE_FUNC();

	// Each row of texels, in the raster image, is not aligned to 4 bytes.
	// This is a direct call to OpenGL but it only affects how images are unpacked
	// from CPU memory so its really a client side state (rather than a graphics card state).
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	texture->gl_bind_texture(GL_TEXTURE_2D);

	// The client has ensured that the texture has been created in OpenGL (eg, by using
	// glTexImage2D) so we can use the faster glTexSubImage2D that doesn't recreate the texture.
	glTexSubImage2D(GL_TEXTURE_2D, 0,
			texel_u_offset, texel_v_offset, image_width, image_height,
			GL_RGBA, GL_UNSIGNED_BYTE, image);

#if 0 // No need to check this so frequently.
	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
#endif
}


void
GPlatesOpenGL::GLTextureUtils::load_rgba8_image_into_texture(
		const GLTexture::shared_ptr_type &texture,
		const GPlatesGui::rgba8_t *image,
		unsigned int image_width,
		unsigned int image_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	load_rgba8_image_into_texture(
			texture,
			static_cast<const void *>(image),
			image_width, image_height,
			texel_u_offset, texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::load_argb32_qimage_into_texture(
		const GLTexture::shared_ptr_type &texture,
		const QImage &argb32_qimage,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	PROFILE_FUNC();

	const int argb32_image_width = argb32_qimage.width();
	const int argb32_image_height = argb32_qimage.height();

	// Create an array of pixels to copy into the texture.
	boost::scoped_array<GPlatesGui::rgba8_t> texture_data_storage(
			new GPlatesGui::rgba8_t[argb32_image_width * argb32_image_height]);
	GPlatesGui::rgba8_t *const texture_data = texture_data_storage.get();
	for (int i = 0; i < argb32_image_height; ++i)
	{
		// Convert a row of QImage::Format_ARGB32 pixels to GPlatesGui::rgba8_t.
		convert_argb32_to_rgba8(
				reinterpret_cast<const boost::uint32_t *>(argb32_qimage.scanLine(i)),
				texture_data + i * argb32_image_width,
				argb32_image_width);
	}

	load_rgba8_image_into_texture(
			texture,
			texture_data_storage.get(),
			argb32_image_width,
			argb32_image_height,
			texel_u_offset,
			texel_v_offset);
}


QImage
GPlatesOpenGL::GLTextureUtils::draw_text_into_qimage(
		const QString &text,
		unsigned int image_width,
		unsigned int image_height,
		const float text_scale,
		const QColor &text_colour,
		const QColor &background_colour)
{
	PROFILE_FUNC();

	// Start off with half-size dimensions - we'll scale to full-size later
	// so that image is more visible (because image will map roughly one texel to one
	// screen pixel which can be hard to read).

	const int scaled_width = static_cast<int>(image_width / text_scale);
	const int scaled_height = static_cast<int>(image_height / text_scale);

	QImage scaled_image(scaled_width, scaled_height, QImage::Format_ARGB32);

	QPainter painter(&scaled_image);
	// Draw filled background
	painter.fillRect(QRect(0, 0, scaled_width, scaled_height), background_colour);
	painter.setPen(text_colour);
	painter.drawText(
			0, 0,
			scaled_width, scaled_height,
			(Qt::AlignCenter | Qt::TextWordWrap),
			text);
	painter.end();

	// Scale the rendered text.
	return scaled_image.scaled(
			image_width, image_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}
