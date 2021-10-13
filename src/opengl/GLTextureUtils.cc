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

#include "GLRenderer.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


void
GPlatesOpenGL::GLTextureUtils::initialise_texture_object_1D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture_object,
		GLenum target,
		GLint internalformat,
		GLsizei width,
		GLint border,
		bool mipmapped)
{
	// And dimensions should be a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPlatesUtils::Base2::is_power_of_two(width),
			GPLATES_ASSERTION_SOURCE);

	// Generate level zero and the mip levels if requested.
	GLint level = 0;
	do
	{
		// Initialise the texture memory but provide no image data.
		texture_object->gl_tex_image_1D(
				renderer, target, level, internalformat, width, border,
				GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		width >>= 1;
		++level;
	}
	while (mipmapped && width);
}


void
GPlatesOpenGL::GLTextureUtils::initialise_texture_object_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture_object,
		GLenum target,
		GLint internalformat,
		GLsizei width,
		GLsizei height,
		GLint border,
		bool mipmapped)
{
	// And dimensions should be a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPlatesUtils::Base2::is_power_of_two(width) &&
				GPlatesUtils::Base2::is_power_of_two(height),
			GPLATES_ASSERTION_SOURCE);

	// Generate level zero and the mip levels if requested.
	GLint level = 0;
	do
	{
		// Initialise the texture memory but provide no image data.
		texture_object->gl_tex_image_2D(
				renderer, target, level, internalformat, width, height, border,
				GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		width >>= 1;
		height >>= 1;
		++level;
	}
	while (mipmapped && width && height);
}


void
GPlatesOpenGL::GLTextureUtils::initialise_texture_object_3D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture_object,
		GLenum target,
		GLint internalformat,
		GLsizei width,
		GLsizei height,
		GLsizei depth,
		GLint border,
		bool mipmapped)
{
	// And dimensions should be a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPlatesUtils::Base2::is_power_of_two(width) &&
				GPlatesUtils::Base2::is_power_of_two(height) &&
				GPlatesUtils::Base2::is_power_of_two(depth),
			GPLATES_ASSERTION_SOURCE);

	// Generate level zero and the mip levels if requested.
	GLint level = 0;
	do
	{
		// Initialise the texture memory but provide no image data.
		texture_object->gl_tex_image_3D(
				renderer, target, level, internalformat, width, height, depth, border,
				GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		width >>= 1;
		height >>= 1;
		depth >>= 1;
		++level;
	}
	while (mipmapped && width && height && depth);
}


void
GPlatesOpenGL::GLTextureUtils::load_image_into_texture_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture,
		const void *image,
		GLenum format,
		GLenum type,
		unsigned int image_width,
		unsigned int image_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	PROFILE_FUNC();

	// Each row of texels, in the raster image, is not necessarily aligned to 4 bytes.
	//
	// FIXME: Shouldn't really be making direct calls to OpenGL - transfer to GLRenderer.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// The client has ensured that the texture has been created in OpenGL (eg, by using
	// glTexImage2D) so we can use the faster glTexSubImage2D that doesn't recreate the texture.
	texture->gl_tex_sub_image_2D(renderer, GL_TEXTURE_2D, 0,
			texel_u_offset, texel_v_offset, image_width, image_height,
			format, type, image);

#if 0 // No need to check this so frequently.
	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
#endif
}


void
GPlatesOpenGL::GLTextureUtils::load_image_into_texture_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture,
		const GLPixelBuffer::shared_ptr_to_const_type &pixels,
		GLint pixels_offset,
		GLenum format,
		GLenum type,
		unsigned int image_width,
		unsigned int image_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	PROFILE_FUNC();

	// Each row of texels, in the raster image, is not necessarily aligned to 4 bytes.
	//
	// FIXME: Shouldn't really be making direct calls to OpenGL - transfer to GLRenderer.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// The client has ensured that the texture has been created in OpenGL (eg, by using
	// glTexImage2D) so we can use the faster glTexSubImage2D that doesn't recreate the texture.
	texture->gl_tex_sub_image_2D(renderer, GL_TEXTURE_2D, 0,
			texel_u_offset, texel_v_offset, image_width, image_height,
			format, type, pixels, pixels_offset);

#if 0 // No need to check this so frequently.
	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
#endif
}


void
GPlatesOpenGL::GLTextureUtils::load_argb32_qimage_into_rgba8_texture_2D(
		GLRenderer &renderer,
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

	load_image_into_rgba8_texture_2D(
			renderer,
			texture,
			texture_data_storage.get(),
			argb32_image_width,
			argb32_image_height,
			texel_u_offset,
			texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::load_colour_into_rgba8_texture_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture,
		const GPlatesGui::rgba8_t &colour,
		unsigned int texel_width,
		unsigned int texel_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	const unsigned num_texels_to_load = texel_width * texel_height;

	// Create an array of same-colour entries.
	boost::scoped_array<GPlatesGui::rgba8_t> image_data_storage(new GPlatesGui::rgba8_t[num_texels_to_load]);
	GPlatesGui::rgba8_t *const image_data = image_data_storage.get();
	for (unsigned int n = 0; n < num_texels_to_load; ++n)
	{
		image_data[n] = colour;
	}

	// Load image into texture...
	load_image_into_rgba8_texture_2D(
			renderer,
			texture,
			image_data_storage.get(),
			texel_width, texel_height,
			texel_u_offset, texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::load_colour_into_rgba32f_texture_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture,
		const GPlatesGui::Colour &colour,
		unsigned int texel_width,
		unsigned int texel_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	// Floating-point textures must be supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_texture_float),
			GPLATES_ASSERTION_SOURCE);

	const unsigned num_texels_to_load = texel_width * texel_height;

	// Create an array of same-colour entries.
	boost::scoped_array<GPlatesGui::Colour> image_data_storage(new GPlatesGui::Colour[num_texels_to_load]);
	GPlatesGui::Colour *const image_data = image_data_storage.get();
	for (unsigned int n = 0; n < num_texels_to_load; ++n)
	{
		image_data[n] = colour;
	}

	// Load image into texture...
	load_image_into_texture_2D(
			renderer,
			texture,
			image_data_storage.get(),
			GL_RGBA,
			GL_FLOAT,
			texel_width, texel_height,
			texel_u_offset, texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::fill_float_texture_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture,
		const GLfloat fill_value,
		GLenum format,
		unsigned int texel_width,
		unsigned int texel_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	// Floating-point textures must be supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_texture_float),
			GPLATES_ASSERTION_SOURCE);

	const unsigned num_texels_to_load = texel_width * texel_height;

	// Create an array of same-fill entries.
	boost::scoped_array<GLfloat> image_data_storage(new GLfloat[num_texels_to_load]);
	GLfloat *const image_data = image_data_storage.get();
	for (unsigned int n = 0; n < num_texels_to_load; ++n)
	{
		image_data[n] = fill_value;
	}

	// Load image into texture...
	load_image_into_texture_2D(
			renderer,
			texture,
			image_data_storage.get(),
			format,
			GL_FLOAT,
			texel_width, texel_height,
			texel_u_offset, texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::fill_float_texture_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture,
		const GLfloat first_fill_value,
		const GLfloat second_fill_value,
		GLenum format,
		unsigned int texel_width,
		unsigned int texel_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	// Floating-point textures must be supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_texture_float),
			GPLATES_ASSERTION_SOURCE);

	const unsigned num_texels_to_load = texel_width * texel_height;

	// Create an array of same-fill entries.
	boost::scoped_array<GLfloat> image_data_storage(new GLfloat[2 * num_texels_to_load]);
	GLfloat *const image_data = image_data_storage.get();
	for (unsigned int n = 0; n < num_texels_to_load; ++n)
	{
		image_data[2 * n] = first_fill_value;
		image_data[2 * n + 1] = second_fill_value;
	}

	// Load image into texture...
	load_image_into_texture_2D(
			renderer,
			texture,
			image_data_storage.get(),
			format,
			GL_FLOAT,
			texel_width, texel_height,
			texel_u_offset, texel_v_offset);
}


void
GPlatesOpenGL::GLTextureUtils::fill_float_texture_2D(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture,
		const GLfloat first_fill_value,
		const GLfloat second_fill_value,
		const GLfloat third_fill_value,
		GLenum format,
		unsigned int texel_width,
		unsigned int texel_height,
		unsigned int texel_u_offset,
		unsigned int texel_v_offset)
{
	// Floating-point textures must be supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_texture_float),
			GPLATES_ASSERTION_SOURCE);

	const unsigned num_texels_to_load = texel_width * texel_height;

	// Create an array of same-fill entries.
	boost::scoped_array<GLfloat> image_data_storage(new GLfloat[3 * num_texels_to_load]);
	GLfloat *const image_data = image_data_storage.get();
	for (unsigned int n = 0; n < num_texels_to_load; ++n)
	{
		image_data[3 * n] = first_fill_value;
		image_data[3 * n + 1] = second_fill_value;
		image_data[3 * n + 2] = third_fill_value;
	}

	// Load image into texture...
	load_image_into_texture_2D(
			renderer,
			texture,
			image_data_storage.get(),
			format,
			GL_FLOAT,
			texel_width, texel_height,
			texel_u_offset, texel_v_offset);
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


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLTextureUtils::create_xy_clip_texture_2D(
		GLRenderer &renderer)
{
	GLTexture::shared_ptr_type xy_clip_texture = GLTexture::create(renderer);

	//
	// We *must* use nearest neighbour filtering otherwise the clip texture won't work.
	// We are relying on the hard transition from white to black to clip for us.
	//
	xy_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	xy_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		xy_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		xy_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		xy_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		xy_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	//
	// The clip texture is a 4x4 image where the centre 2x2 texels are 1.0
	// and the boundary texels are 0.0.
	// We will use the alpha channel for alpha-testing (to discard clipped regions).
	//
	const GPlatesGui::rgba8_t mask_zero(0, 0, 0, 0);
	const GPlatesGui::rgba8_t mask_one(255, 255, 255, 255);
	const GPlatesGui::rgba8_t mask_image[16] =
	{
		mask_zero, mask_zero, mask_zero, mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_zero, mask_zero, mask_zero
	};

	// Create the texture and load the data into it.
	xy_clip_texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_image);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

	return xy_clip_texture;
}


GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLTextureUtils::create_z_clip_texture_2D(
		GLRenderer &renderer)
{
	GLTexture::shared_ptr_type z_clip_texture = GLTexture::create(renderer);

	//
	// We *must* use nearest neighbour filtering otherwise the clip texture won't work.
	// We are relying on the hard transition from white to black to clip for us.
	//
	z_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	z_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		z_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		z_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		z_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		z_clip_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	//
	// The clip texture is a 2x1 image where the one texel is white and the other black.
	// We will use the alpha channel for alpha-testing (to discard clipped regions).
	//
	const GPlatesGui::rgba8_t mask_zero(0, 0, 0, 0);
	const GPlatesGui::rgba8_t mask_one(255, 255, 255, 255);
	const GPlatesGui::rgba8_t mask_image[2] = { mask_zero, mask_one };

	// Create the texture and load the data into it.
	z_clip_texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, GL_RGBA8, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_image);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

	return z_clip_texture;
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform()
{
	// Note that the scale is slightly less than 0.25 - this is to avoid seams/gaps between
	// adjacent tiles - this can occur if a screen pixel centre (in render-target) falls right on
	// the tile boundary - in this case slight numerical differences can mean the pixel is just
	// outside the clip zone of both adjacent tiles and hence does not get drawn - this usually
	// only happens when the view is aligned perfectly orthogonally to the tile boundary and this
	// can be the case when GPlates first starts - once the user rotates the view with the mouse
	// it generally isn't noticeable anymore. So the solution is to make the clip regions of
	// adjacent tiles overlap very slightly - here the overlap is 1/2,000th of a texel assuming
	// a 256x256 texel tile so the distortion should be very negligible and undetectable.
	static const double clip_texture_scale = 0.249999;
	static GLMatrix matrix = GLMatrix()
			.gl_translate(0.5, 0.5, 0.0)
			.gl_scale(clip_texture_scale, clip_texture_scale, 1.0);

	return matrix;
}
