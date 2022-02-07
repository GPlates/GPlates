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

#include "GLMatrix.h"
#include "GLPixelBuffer.h"
#include "GLTexture.h"

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	namespace GLTextureUtils
	{
		/**
		 * Initialises the specified texture object as a 1D texture matching the specified parameters.
		 *
		 * NOTE: The dimensions do *not* need to be a power-of-two.
		 *
		 * NOTE: The specified texture will have its level zero initialised (memory allocated for image)
		 * but the image data will be unspecified.
		 * If @a mipmapped is true then all mipmap level will also be initialised but unspecified.
		 *
		 * NOTE: Other texture parameters (such as filtering, etc) are not specified here so
		 * you will probably want to explicitly set all that state in the texture object.
		 */
		void
		initialise_texture_object_1D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture_object,
				GLenum target,
				GLint internalformat,
				GLsizei width,
				GLint border,
				bool mipmapped);

		/**
		 * Initialises the specified texture object as a 2D texture matching the specified parameters.
		 *
		 * NOTE: The dimensions do *not* need to be a power-of-two.
		 *
		 * NOTE: The specified texture will have its level zero initialised (memory allocated for image)
		 * but the image data will be unspecified.
		 * If @a mipmapped is true then all mipmap level will also be initialised but unspecified.
		 *
		 * NOTE: Other texture parameters (such as filtering, etc) are not specified here so
		 * you will probably want to explicitly set all that state in the texture object.
		 */
		void
		initialise_texture_object_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture_object,
				GLenum target,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLint border,
				bool mipmapped);

		/**
		 * Initialises the specified texture object as a 3D texture matching the specified parameters.
		 *
		 * NOTE: The dimensions do *not* need to be a power-of-two.
		 *
		 * NOTE: The specified texture will have its level zero initialised (memory allocated for image)
		 * but the image data will be unspecified.
		 * If @a mipmapped is true then all mipmap level will also be initialised but unspecified.
		 *
		 * NOTE: Other texture parameters (such as filtering, etc) are not specified here so
		 * you will probably want to explicitly set all that state in the texture object.
		 */
		void
		initialise_texture_object_3D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture_object,
				GLenum target,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLint border,
				bool mipmapped);


		/**
		 * Loads the specified image into the specified texture.
		 *
		 * The format and type of data contained in @a image are specified with @a format and @a type.
		 *
		 * NOTE: It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 */
		void
		load_image_into_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const void *image,
				GLenum format,
				GLenum type,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);

		/**
		 * Same as the other overload of @a load_image_into_texture_2D but loads image from a pixel buffer.
		 *
		 * NOTE: The image data is read beginning at offset @a pixels_offset in the specified pixel buffer.
		 */
		void
		load_image_into_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint pixels_offset,
				GLenum format,
				GLenum type,
				unsigned int image_width,
				unsigned int image_height,
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
		 */
		inline
		void
		load_image_into_rgba8_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const void *image,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0)
		{
			load_image_into_texture_2D(
					renderer,
					texture,
					image, GL_RGBA, GL_UNSIGNED_BYTE,
					image_width, image_height,
					texel_u_offset, texel_v_offset);
		}


		/**
		 * Same as the other overload of @a load_image_into_rgba8_texture_2D but loads image from a pixel buffer.
		 *
		 * NOTE: The image data is read beginning at offset @a pixels_offset in the specified pixel buffer.
		 */
		inline
		void
		load_image_into_rgba8_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint pixels_offset,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0)
		{
			load_image_into_texture_2D(
					renderer,
					texture,
					pixels, pixels_offset, GL_RGBA, GL_UNSIGNED_BYTE,
					image_width, image_height,
					texel_u_offset, texel_v_offset);
		}


		/**
		 * Loads the specified RGBA8 image into the specified RGBA texture.
		 *
		 * NOTE: It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 * It is also the caller's responsibility to ensure that @a image points
		 * to @a image_width by @a image_height colour values.
		 */
		inline
		void
		load_image_into_rgba8_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GPlatesGui::rgba8_t *image,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0)
		{
			load_image_into_rgba8_texture_2D(
					renderer,
					texture,
					static_cast<const void *>(image),
					image_width, image_height,
					texel_u_offset, texel_v_offset);
		}


		/**
		 * Loads the specified QImage, must be QImage::Format_ARGB32, into the specified texture.
		 *
		 * NOTE: It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 */
		void
		load_argb32_qimage_into_rgba8_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const QImage &argb32_qimage,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified region of the RGBA8 texture with a single colour.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 */
		void
		load_colour_into_rgba8_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GPlatesGui::rgba8_t &colour,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified region of the RGBA32F *float-point* texture with a single colour.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * This is the equivalent of having a @a fill_float_texture_2D with four fill values.
		 *
		 * NOTE: The GL_ARB_texture_float extension must be supported.
		 */
		void
		load_colour_into_rgba32f_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GPlatesGui::Colour &colour,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified *floating-point* fill value into the specified *floating-point* texture.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * NOTE: The GL_ARB_texture_float extension must be supported and @a format should be one
		 * that allows specifying image data containing one floating-point value per pixel such as:
		 *   GL_RED, GL_ALPHA, GL_INTENSITY, GL_LUMINANCE.
		 */
		void
		fill_float_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GLfloat fill_value,
				GLenum format,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified *floating-point* fill values into the specified *floating-point* texture.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * NOTE: The GL_ARB_texture_float extension must be supported and @a format should be one
		 * that allows specifying image data containing *two* floating-point values per pixel such as:
		 *   GL_RG, GL_LUMINANCE_ALPHA.
		 */
		void
		fill_float_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GLfloat first_fill_value,
				const GLfloat second_fill_value,
				GLenum format,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified *floating-point* fill values into the specified *floating-point* texture.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * NOTE: The GL_ARB_texture_float extension must be supported and @a format should be one
		 * that allows specifying image data containing *three* floating-point values per pixel such as:
		 *   GL_RGB.
		 */
		void
		fill_float_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GLfloat first_fill_value,
				const GLfloat second_fill_value,
				const GLfloat third_fill_value,
				GLenum format,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0);


		/**
		 * Loads the specified *floating-point* fill values into the specified *floating-point* texture.
		 *
		 * It is the caller's responsibility to ensure the region is inside an
		 * already allocated and created OpenGL texture.
		 *
		 * NOTE: The GL_ARB_texture_float extension must be supported.
		 */
		inline
		void
		fill_float_texture_2D(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture,
				const GLfloat red_fill_value,
				const GLfloat green_fill_value,
				const GLfloat blue_fill_value,
				const GLfloat alpha_fill_value,
				unsigned int texel_width,
				unsigned int texel_height,
				unsigned int texel_u_offset = 0,
				unsigned int texel_v_offset = 0)
		{
			load_colour_into_rgba32f_texture_2D(
					renderer,
					texture,
					GPlatesGui::Colour(red_fill_value, green_fill_value, blue_fill_value, alpha_fill_value),
					texel_width,
					texel_height,
					texel_u_offset,
					texel_v_offset);
		}


		/**
		 * Creates a new 4x4 texel clip texture whose centre 2x2 texels are white with the
		 * remaining texels black (including alpha channel).
		 */
		GLTexture::shared_ptr_type
		create_xy_clip_texture_2D(
				GLRenderer &renderer);


		/**
		 * Creates a new 2x1 texel clip texture whose first texel is black and second texel white (including alpha channel).
		 *
		 * NOTE: The created texture is actually a 2D texture and *not* a 1D texture.
		 */
		GLTexture::shared_ptr_type
		create_z_clip_texture_2D(
				GLRenderer &renderer);


		/**
		 * Initialise clip texture transform to convert the clip-space range [-1, 1] to
		 * range [0.25, 0.75] to map to the interior 2x2 texel region of the 4x4 clip texture.
		 */
		const GLMatrix &
		get_clip_texture_clip_space_to_texture_space_transform();
	}
}

#endif // GPLATES_OPENGL_GLTEXTUREUTILS_H
