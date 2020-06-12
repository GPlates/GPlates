/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLPIXELBUFFERIMPL_H
#define GPLATES_OPENGL_GLPIXELBUFFERIMPL_H

#include <opengl/OpenGL.h>

#include "GLBufferImpl.h"
#include "GLPixelBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLTexture;

	/**
	 * An implementation of the OpenGL buffer objects extension as used for pixel buffers containing
	 * framebuffer data - either from or to OpenGL (eg, streaming to a texture or reading back pixels
	 * from the framebuffer).
	 *
	 * This implementation is used when the OpenGL extension is not supported - in which case
	 * pixel buffer objects are simulated by using client-side memory arrays in a base OpenGL 1.1 way.
	 */
	class GLPixelBufferImpl :
			public GLPixelBuffer
	{
	public:
		//! A convenience typedef for a shared pointer to a @a GLPixelBufferImpl.
		typedef boost::shared_ptr<GLPixelBufferImpl> shared_ptr_type;
		typedef boost::shared_ptr<const GLPixelBufferImpl> shared_ptr_to_const_type;


		/**
		 * Creates a @a GLPixelBufferImpl object attached to the specified buffer.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer)
		{
			return shared_ptr_type(create_as_unique_ptr(renderer, buffer).release());
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLPixelBufferImpl>
		create_as_unique_ptr(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer)
		{
			return std::unique_ptr<GLPixelBufferImpl>(new GLPixelBufferImpl(renderer, buffer));
		}


		/**
		 * Returns the buffer used to store the pixel data.
		 */
		virtual
		GLBuffer::shared_ptr_to_const_type
		get_buffer() const;


		/**
		 * Binds this pixel buffer as a pixel *unpack* buffer so that data can be unpacked (read) from the buffer.
		 *
		 * Note that it's possible to bind the same buffer to the unpack *and* pack targets.
		 */
		virtual
		void
		gl_bind_unpack(
				GLRenderer &renderer) const;


		/**
		 * Binds this pixel buffer as a pixel *pack* buffer so that data can be packed (written) into the buffer.
		 *
		 * Note that it's possible to bind the same buffer to the unpack *and* pack targets.
		 */
		virtual
		void
		gl_bind_pack(
				GLRenderer &renderer) const;


		/**
		 * Performs the equivalent of the OpenGL command 'glDrawPixels' with the exception that,
		 * to mirror 'glReadPixels', the x and y pixel offsets are also specified (internally
		 * 'glWindowPos2i(x, y)' is called since 'glDrawPixels' does not accept x and y).
		 *
		 * NOTE: You must have called @a gl_bind_unpack to bind 'this' buffer as an *unpack* target.
		 *
		 * @a offset is a byte offset from the start of 'this' pixel buffer to start copying pixels from.
		 */
		virtual
		void
		gl_draw_pixels(
				GLRenderer &renderer,
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				GLint offset);


		/**
		 * Performs the equivalent of the OpenGL command 'glReadPixels'.
		 *
		 * NOTE: You must have called @a gl_bind_pack to bind 'this' buffer as an *pack* target.
		 *
		 * @a offset is a byte offset from the start of 'this' pixel buffer to start reading pixels into.
		 */
		virtual
		void
		gl_read_pixels(
				GLRenderer &renderer,
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				GLint offset);


		/**
		 * Performs same function as the glTexImage1D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 */
		virtual
		void
		gl_tex_image_1D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLint border,
				GLenum format,
				GLenum type,
				GLint offset) const
		{
			const GLvoid *pixels = d_buffer->get_buffer_resource() + offset;
			gl_tex_image_1D(renderer, texture, target, level, internalformat, width, border, format, type, pixels);
		}

		/**
		 * Performs same function as the glTexImage1D OpenGL function using pixel data from @a pixels.
		 *
		 * NOTE: This 'static' method is to support texture image loading from a client memory array
		 * that is *not* in a @a GLPixelBufferImpl object (ie, a client memory array wrapping in
		 * a pixel buffer emulator). This is useful for texture image data that was not loaded into
		 * a pixel buffer (because it was difficult to layer the source code as such).
		 */
		static
		void
		gl_tex_image_1D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLint border,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);


		/**
		 * Performs same function as the glTexImage2D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 */
		virtual
		void
		gl_tex_image_2D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLint border,
				GLenum format,
				GLenum type,
				GLint offset) const
		{
			const GLvoid *pixels = d_buffer->get_buffer_resource() + offset;
			gl_tex_image_2D(renderer, texture, target, level, internalformat, width, height, border, format, type, pixels);
		}

		/**
		 * Performs same function as the glTexImage2D OpenGL function using pixel data from @a pixels.
		 *
		 * NOTE: This 'static' method is to support texture image loading from a client memory array
		 * that is *not* in a @a GLPixelBufferImpl object (ie, a client memory array wrapping in
		 * a pixel buffer emulator). This is useful for texture image data that was not loaded into
		 * a pixel buffer (because it was difficult to layer the source code as such).
		 */
		static
		void
		gl_tex_image_2D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLint border,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);


		/**
		 * Performs same function as the glTexImage3D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: OpenGL 1.2 must be supported.
		 */
		virtual
		void
		gl_tex_image_3D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLint border,
				GLenum format,
				GLenum type,
				GLint offset) const
		{
			const GLvoid *pixels = d_buffer->get_buffer_resource() + offset;
			gl_tex_image_3D(renderer, texture, target, level, internalformat, width, height, depth, border, format, type, pixels);
		}

		/**
		 * Performs same function as the glTexImage3D OpenGL function using pixel data from @a pixels.
		 *
		 * NOTE: This 'static' method is to support texture image loading from a client memory array
		 * that is *not* in a @a GLPixelBufferImpl object (ie, a client memory array wrapping in
		 * a pixel buffer emulator). This is useful for texture image data that was not loaded into
		 * a pixel buffer (because it was difficult to layer the source code as such).
		 */
		static
		void
		gl_tex_image_3D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLint border,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);


		/**
		 * Performs same function as the glTexSubImage1D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 */
		virtual
		void
		gl_tex_sub_image_1D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLsizei width,
				GLenum format,
				GLenum type,
				GLint offset) const
		{
			const GLvoid *pixels = d_buffer->get_buffer_resource() + offset;
			gl_tex_sub_image_1D(renderer, texture, target, level, xoffset, width, format, type, pixels);
		}

		/**
		 * Performs same function as the glTexSubImage1D OpenGL function.
		 *
		 * NOTE: This 'static' method is to support texture image loading from a client memory array
		 * that is *not* in a @a GLPixelBufferImpl object (ie, a client memory array wrapping in
		 * a pixel buffer emulator). This is useful for texture image data that was not loaded into
		 * a pixel buffer (because it was difficult to layer the source code as such).
		 */
		static
		void
		gl_tex_sub_image_1D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLsizei width,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);


		/**
		 * Performs same function as the glTexSubImage2D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 */
		virtual
		void
		gl_tex_sub_image_2D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				GLint offset) const
		{
			const GLvoid *pixels = d_buffer->get_buffer_resource() + offset;
			gl_tex_sub_image_2D(renderer, texture, target, level, xoffset, yoffset, width, height, format, type, pixels);
		}

		/**
		 * Performs same function as the glTexSubImage2D OpenGL function.
		 *
		 * NOTE: This 'static' method is to support texture image loading from a client memory array
		 * that is *not* in a @a GLPixelBufferImpl object (ie, a client memory array wrapping in
		 * a pixel buffer emulator). This is useful for texture image data that was not loaded into
		 * a pixel buffer (because it was difficult to layer the source code as such).
		 */
		static
		void
		gl_tex_sub_image_2D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);


		/**
		 * Performs same function as the glTexSubImage3D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: OpenGL 1.2 must be supported.
		 */
		virtual
		void
		gl_tex_sub_image_3D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLint zoffset,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLenum format,
				GLenum type,
				GLint offset) const
		{
			const GLvoid *pixels = d_buffer->get_buffer_resource() + offset;
			gl_tex_sub_image_3D(
					renderer, texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
		}

		/**
		 * Performs same function as the glTexSubImage3D OpenGL function.
		 *
		 * NOTE: This 'static' method is to support texture image loading from a client memory array
		 * that is *not* in a @a GLPixelBufferImpl object (ie, a client memory array wrapping in
		 * a pixel buffer emulator). This is useful for texture image data that was not loaded into
		 * a pixel buffer (because it was difficult to layer the source code as such).
		 *
		 * NOTE: OpenGL 1.2 must be supported.
		 */
		static
		void
		gl_tex_sub_image_3D(
				GLRenderer &renderer,
				const boost::shared_ptr<const GLTexture> &texture,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLint zoffset,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

	private:
		//! The buffer being targeted by this pixel buffer.
		GLBufferImpl::shared_ptr_type d_buffer;


		//! Constructor.
		explicit
		GLPixelBufferImpl(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer);
	};
}

#endif // GPLATES_OPENGL_GLPIXELBUFFERIMPL_H
