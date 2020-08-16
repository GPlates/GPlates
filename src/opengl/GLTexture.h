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
 
#ifndef GPLATES_OPENGL_GLTEXTURE_H
#define GPLATES_OPENGL_GLTEXTURE_H

#include <memory> // For std::unique_ptr
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL1.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLPixelBuffer.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A texture object.
	 *
	 * Note that since texture objects are supported in core OpenGL 1.1 (the lowest version GPlates supports)
	 * we don't need to emulate them (in case they're not supported) and so we call this
	 * class "GLTexture" instead of "GLTextureObject" like the other types of OpenGL resource objects.
	 */
	class GLTexture :
			public GLObject,
			public boost::enable_shared_from_this<GLTexture>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLTexture.
		typedef boost::shared_ptr<GLTexture> shared_ptr_type;
		typedef boost::shared_ptr<const GLTexture> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLTexture.
		typedef boost::weak_ptr<GLTexture> weak_ptr_type;
		typedef boost::weak_ptr<const GLTexture> weak_ptr_to_const_type;


		/**
		 * Creates a shared pointer to a @a GLTexture object.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer)
		{
			return shared_ptr_type(new GLTexture(renderer));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLTexture>
		create_as_unique_ptr(
				GLRenderer &renderer)
		{
			return std::unique_ptr<GLTexture>(new GLTexture(renderer));
		}


		/**
		 * Performs same function as the glTexParameteri OpenGL function.
		 */
		void
		gl_tex_parameteri(
				GLRenderer &renderer,
				GLenum target,
				GLenum pname,
				GLint param);

		/**
		 * Performs same function as the glTexParameterf OpenGL function.
		 */
		void
		gl_tex_parameterf(
				GLRenderer &renderer,
				GLenum target,
				GLenum pname,
				GLfloat param);


		//
		// Methods to initialise a texture image.
		//


		/**
		 * Performs same function as the glTexImage1D OpenGL function.
		 *
		 * NOTE: If @a pixels is NULL then the texture is created but the image data is left uninitialised.
		 *
		 * NOTE: There's no need to unbind pixel buffer objects (to ensure sourcing from client memory)
		 * because that is taken care of internally.
		 */
		void
		gl_tex_image_1D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLint border,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		/**
		 * Performs same function as the glTexImage1D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the specified pixel buffer.
		 */
		void
		gl_tex_image_1D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLint border,
				GLenum format,
				GLenum type,
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint offset = 0);

		/**
		 * Performs same function as the glTexImage2D OpenGL function.
		 *
		 * NOTE: If @a pixels is NULL then the texture is created but the image data is left uninitialised.
		 *
		 * NOTE: For cube map textures @a target is one of GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, etc
		 * specifying the cube face (and not the bind target GL_TEXTURE_CUBE_MAP_ARB).
		 *
		 * NOTE: There's no need to unbind pixel buffer objects (to ensure sourcing from client memory)
		 * because that is taken care of internally.
		 */
		void
		gl_tex_image_2D(
				GLRenderer &renderer,
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
		 * Performs same function as the glTexImage2D OpenGL function.
		 *
		 * NOTE: For cube map textures @a target is one of GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, etc
		 * specifying the cube face (and not the bind target GL_TEXTURE_CUBE_MAP_ARB).
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the specified pixel buffer.
		 */
		void
		gl_tex_image_2D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLint border,
				GLenum format,
				GLenum type,
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint offset = 0);

		/**
		 * Performs same function as the glTexImage3D OpenGL function.
		 *
		 * NOTE: If @a pixels is NULL then the texture is created but the image data is left uninitialised.
		 *
		 * NOTE: There's no need to unbind pixel buffer objects (to ensure sourcing from client memory)
		 * because that is taken care of internally.
		 *
		 * NOTE: OpenGL 1.2 must be supported.
		 */
		void
		gl_tex_image_3D(
				GLRenderer &renderer,
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
		 * Performs same function as the glTexImage3D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the specified pixel buffer.
		 *
		 * NOTE: OpenGL 1.2 must be supported.
		 */
		void
		gl_tex_image_3D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLsizei width,
				GLsizei height,
				GLsizei depth,
				GLint border,
				GLenum format,
				GLenum type,
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint offset = 0);


		/**
		 * Performs same function as the glCopyTexImage1D OpenGL function.
		 *
		 * Note that this is part of @a GLTexture instead of @a GLRenderer because it initialises
		 * the texture (sets up the internal object storage, etc) as opposed to
		 * GLRenderer::gl_copy_tex_sub_image_1D which copies into an already initialised texture object.
		 */
		void
		gl_copy_tex_image_1D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLint x,
				GLint y,
				GLsizei width,
				GLint border);

		/**
		 * Performs same function as the glCopyTexImage2D OpenGL function.
		 *
		 * Note that this is part of @a GLTexture instead of @a GLRenderer because it initialises
		 * the texture (sets up the internal object storage, etc) as opposed to
		 * GLRenderer::gl_copy_tex_sub_image_2D which copies into an already initialised texture object.
		 *
		 * NOTE: For cube map textures @a target is one of GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, etc
		 * specifying the cube face (and not the bind target GL_TEXTURE_CUBE_MAP_ARB).
		 */
		void
		gl_copy_tex_image_2D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint internalformat,
				GLint x,
				GLint y,
				GLsizei width,
				GLsizei height,
				GLint border);


		//
		// Methods to update a texture.
		//


		/**
		 * Performs same function as the glTexSubImage1D OpenGL function.
		 *
		 * NOTE: There's no need to unbind pixel buffer objects (to ensure sourcing from client memory)
		 * because that is taken care of internally.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_tex_sub_image_1D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLsizei width,
				GLenum format,
				GLenum type,
				const GLvoid *pixels);

		/**
		 * Performs same function as the glTexSubImage1D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the specified pixel buffer.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_tex_sub_image_1D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLsizei width,
				GLenum format,
				GLenum type,
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint offset = 0);

		/**
		 * Performs same function as the glTexSubImage2D OpenGL function.
		 *
		 * NOTE: For cube map textures @a target is one of GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, etc
		 * specifying the cube face (and not the bind target GL_TEXTURE_CUBE_MAP_ARB).
		 *
		 * NOTE: There's no need to unbind pixel buffer objects (to ensure sourcing from client memory)
		 * because that is taken care of internally.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_tex_sub_image_2D(
				GLRenderer &renderer,
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
		 * Performs same function as the glTexSubImage2D OpenGL function.
		 *
		 * NOTE: For cube map textures @a target is one of GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, etc
		 * specifying the cube face (and not the bind target GL_TEXTURE_CUBE_MAP_ARB).
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the specified pixel buffer.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_tex_sub_image_2D(
				GLRenderer &renderer,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLint yoffset,
				GLsizei width,
				GLsizei height,
				GLenum format,
				GLenum type,
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint offset = 0);

		/**
		 * Performs same function as the glTexSubImage3D OpenGL function.
		 *
		 * NOTE: There's no need to unbind pixel buffer objects (to ensure sourcing from client memory)
		 * because that is taken care of internally.
		 *
		 * NOTE: OpenGL 1.2 must be supported.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_tex_sub_image_3D(
				GLRenderer &renderer,
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

		/**
		 * Performs same function as the glTexSubImage3D OpenGL function.
		 *
		 * NOTE: The image data is read beginning at offset @a offset in the specified pixel buffer.
		 *
		 * NOTE: OpenGL 1.2 must be supported.
		 *
		 * @throws PreconditionViolationError if @a texture is not initialised.
		 */
		void
		gl_tex_sub_image_3D(
				GLRenderer &renderer,
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
				const GLPixelBuffer::shared_ptr_to_const_type &pixels,
				GLint offset = 0);


		//
		// General query methods.
		//


		/**
		 * Returns the width of the texture (level 0).
		 *
		 * Returns boost::none unless @a gl_tex_image_1D, @a gl_tex_image_2D, @a gl_tex_image_3D,
		 * @a gl_copy_tex_image_1D or @a gl_copy_tex_image_2D have been called.
		 */
		boost::optional<GLuint>
		get_width() const
		{
			return d_width;
		}

		/**
		 * Returns the height of the texture (level 0).
		 *
		 * Note that this is also the number of array layers for a 1D texture array (GL_TEXTURE_1D_ARRAY target).
		 *
		 * Returns boost::none unless @a gl_tex_image_2D, @a gl_tex_image_3D or @a gl_copy_tex_image_2D have been called.
		 */
		boost::optional<GLuint>
		get_height() const
		{
			return d_height;
		}

		/**
		 * Returns the depth of the texture (level 0).
		 *
		 * Note that this is also the number of array layers for a 2D texture array (GL_TEXTURE_2D_ARRAY target).
		 *
		 * Returns boost::none unless @a gl_tex_image_3D has been called.
		 */
		boost::optional<GLuint>
		get_depth() const
		{
			return d_depth;
		}


		/**
		 * Returns the internal format of the texture.
		 *
		 * Returns boost::none unless none of the texture image specification methods have been called.
		 */
		boost::optional<GLint>
		get_internal_format() const
		{
			return d_internal_format;
		}


		/**
		 * Returns true if 'this' texture is a floating-point texture.
		 *
		 * This is determined by the 'internalformat' parameter to @a gl_tex_image_2d for example.
		 *
		 * Returns false if either:
		 *   1) this texture is a fixed-point texture, or
		 *   2) none of the texture image specification methods have been called yet.
		 *
		 * NOTE: Excludes floating-point depth formats (only colour formats considered).
		 */
		bool
		is_floating_point() const
		{
			return d_internal_format ? is_format_floating_point(d_internal_format.get()) : false;
		}


		/**
		 * Returns true if the specified internal texture format is a floating-point format.
		 *
		 * This is the same as the 'internalformat' parameter to @a gl_tex_image_2d for example.
		 *
		 * NOTE: Excludes floating-point depth formats (only colour formats considered).
		 */
		static
		bool
		is_format_floating_point(
				GLint internalformat);


		/**
		 * Returns the texture resource handle.
		 */
		GLuint
		get_resource_handle() const;


	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL shader objects.
		 */
		class Allocator
		{
		public:
			GLuint
			allocate(
					const GLCapabilities &capabilities);

			void
			deallocate(
					GLuint);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<GLuint, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<GLuint, Allocator> resource_manager_type;

	private:
		resource_type::non_null_ptr_to_const_type d_resource;

		boost::optional<GLuint> d_width;
		boost::optional<GLuint> d_height;
		boost::optional<GLuint> d_depth;

		boost::optional<GLint> d_internal_format;

		//! Constructor.
		explicit
		GLTexture(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLTEXTURE_H
