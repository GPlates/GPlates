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

#ifndef GPLATES_OPENGL_GLPIXELBUFFER_H
#define GPLATES_OPENGL_GLPIXELBUFFER_H

#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLTexture;

	/**
	 * An abstraction of the OpenGL buffer objects extension as used for pixel buffers containing
	 * framebuffer data - either from or to OpenGL (eg, streaming to a texture or reading back pixels
	 * from the framebuffer).
	 */
	class GLPixelBuffer :
			public boost::enable_shared_from_this<GLPixelBuffer>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLPixelBuffer.
		typedef boost::shared_ptr<GLPixelBuffer> shared_ptr_type;
		typedef boost::shared_ptr<const GLPixelBuffer> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLPixelBuffer.
		typedef boost::weak_ptr<GLPixelBuffer> weak_ptr_type;
		typedef boost::weak_ptr<const GLPixelBuffer> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLPixelBuffer object attached to the specified buffer.
		 *
		 * Note that is is possible to attach the same buffer object to a @a GLVertexBuffer.
		 * This allows you to, for example, render vertices to the framebuffer (using a fragment
		 * shader), then read the pixels to a pixel buffer ,then bind the buffer as a vertex buffer
		 * then render the data as vertices.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				const GLBuffer::shared_ptr_type &buffer)
		{
			return shared_ptr_type(create_as_auto_ptr(renderer, buffer).release());
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLPixelBuffer>
		create_as_auto_ptr(
				GLRenderer &renderer,
				const GLBuffer::shared_ptr_type &buffer);


		virtual
		~GLPixelBuffer()
		{  }


		/**
		 * Returns the 'const' buffer used to store the pixel data.
		 */
		virtual
		GLBuffer::shared_ptr_to_const_type
		get_buffer() const = 0;

		/**
		 * Returns the 'non-const' buffer used to store the pixel data.
		 */
		GLBuffer::shared_ptr_type
		get_buffer()
		{
			return boost::const_pointer_cast<GLBuffer>(
					static_cast<const GLPixelBuffer*>(this)->get_buffer());
		}


		/**
		 * Binds this pixel buffer as a pixel *unpack* buffer so that data can be unpacked (read) from the buffer.
		 *
		 * Note that it's possible to bind the same buffer to the unpack *and* pack targets.
		 */
		virtual
		void
		gl_bind_unpack(
				GLRenderer &renderer) const = 0;


		/**
		 * Binds this pixel buffer as a pixel *pack* buffer so that data can be packed (written) into the buffer.
		 *
		 * Note that it's possible to bind the same buffer to the unpack *and* pack targets.
		 */
		virtual
		void
		gl_bind_pack(
				GLRenderer &renderer) const = 0;


		/**
		 * Performs the equivalent of the OpenGL command 'glReadPixels'.
		 *
		 * If native pixel buffer objects are supported (ie, derived type is @a GLPixelBufferObject)
		 * then this call will start an asynchronous read back from GPU to CPU and
		 * return immediately without blocking. The only time blocking will happen is when the
		 * pixel buffer is read (via @a get_buffer) in which case the CPU will wait for the GPU
		 * to finish generating data and transferring it to the CPU (if it hasn't already completed).
		 * So it is a good idea to delay reading of the buffer where possible by doing some work
		 * in between (a good way to do this is to double buffer - ie, have two alternating pixel buffers).
		 *
		 * NOTE: You must have called @a gl_bind_pack to bind 'this' buffer as a *pack* target.
		 *
		 * NOTE: You should have enough memory in the buffer to accept the incoming data
		 * (see 'GLBuffer::gl_buffer_data()').
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
				GLint offset) = 0;

	private:
		//
		// The following methods are private since they should only be called by @a GLTexture as
		// part of its implementation.
		//

		/**
		 * Performs same function as the glTexImage1D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: There is no need to bind 'this' pixel buffer or the specified texture - it's handled internally.
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
				GLint offset) const = 0;


		/**
		 * Performs same function as the glTexImage2D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: There is no need to bind 'this' pixel buffer or the specified texture - it's handled internally.
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
				GLint offset) const = 0;


		/**
		 * Performs same function as the glTexImage3D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: There is no need to bind 'this' pixel buffer or the specified texture - it's handled internally.
		 *
		 * NOTE: The GL_EXT_texture3D extension must be available.
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
				GLint offset) const = 0;


		/**
		 * Performs same function as the glTexSubImage1D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: There is no need to bind 'this' pixel buffer or the specified texture - it's handled internally.
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
				GLint offset) const = 0;


		/**
		 * Performs same function as the glTexSubImage2D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: There is no need to bind 'this' pixel buffer or the specified texture - it's handled internally.
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
				GLint offset) const = 0;


		/**
		 * Performs same function as the glTexSubImage3D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the pixel buffer.
		 *
		 * NOTE: There is no need to bind 'this' pixel buffer or the specified texture - it's handled internally.
		 *
		 * NOTE: The GL_EXT_subtexture extension must be available.
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
				GLint offset) const = 0;

		// Class @a GLTexture calls the above private methods.
		friend class GLTexture;
	};
}

#endif // GPLATES_OPENGL_GLPIXELBUFFER_H
