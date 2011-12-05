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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLTexture.h"

#include "GLPixelBufferImpl.h"
#include "GLRenderer.h"


GPlatesOpenGL::GLTexture::GLTexture(
		GLRenderer &renderer) :
	d_resource(
			resource_type::create(
					renderer.get_context().get_shared_state()->get_texture_object_resource_manager()))
{
}


void
GPlatesOpenGL::GLTexture::gl_tex_parameteri(
		GLRenderer &renderer,
		GLenum target,
		GLenum pname,
		GLint param)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind(renderer, shared_from_this(), GL_TEXTURE0, target);

	glTexParameteri(target, pname, param);
}


void
GPlatesOpenGL::GLTexture::gl_tex_parameterf(
		GLRenderer &renderer,
		GLenum target,
		GLenum pname,
		GLfloat param)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind(renderer, shared_from_this(), GL_TEXTURE0, target);

	glTexParameterf(target, pname, param);
}


void
GPlatesOpenGL::GLTexture::gl_tex_image_1D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint internalformat,
		GLsizei width,
		GLint border,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	// Load the texture image via pixel buffer emulation functionality to ensure that no
	// native OpenGL pixel buffer objects are bound when the texture is uploaded
	// (because the texture is being sourced from client memory).
	GLPixelBufferImpl::gl_tex_image_1D(
			renderer, shared_from_this(), target, level, internalformat, width, border, format, type, pixels);

	if (level == 0)
	{
		d_width = width;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_tex_image_1D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint internalformat,
		GLsizei width,
		GLint border,
		GLenum format,
		GLenum type,
		const GLPixelBuffer::shared_ptr_to_const_type &pixels,
		GLint offset)
{
	// Get the pixel buffer to load the texture image.
	pixels->gl_tex_image_1D(renderer, shared_from_this(), target, level, internalformat, width, border, format, type, offset);

	if (level == 0)
	{
		d_width = width;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_tex_image_2D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint internalformat,
		GLsizei width,
		GLsizei height,
		GLint border,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	// Load the texture image via pixel buffer emulation functionality to ensure that no
	// native OpenGL pixel buffer objects are bound when the texture is uploaded
	// (because the texture is being sourced from client memory).
	GLPixelBufferImpl::gl_tex_image_2D(
			renderer, shared_from_this(), target, level, internalformat, width, height, border, format, type, pixels);

	if (level == 0)
	{
		d_width = width;
		d_height = height;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_tex_image_2D(
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
		GLint offset)
{
	// Get the pixel buffer to load the texture image.
	pixels->gl_tex_image_2D(
			renderer, shared_from_this(), target, level, internalformat, width, height, border, format, type, offset);

	if (level == 0)
	{
		d_width = width;
		d_height = height;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_tex_image_3D(
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
		const GLvoid *pixels)
{
	// Load the texture image via pixel buffer emulation functionality to ensure that no
	// native OpenGL pixel buffer objects are bound when the texture is uploaded
	// (because the texture is being sourced from client memory).
	GLPixelBufferImpl::gl_tex_image_3D(
			renderer, shared_from_this(), target, level, internalformat, width, height, depth, border, format, type, pixels);

	if (level == 0)
	{
		d_width = width;
		d_height = height;
		d_depth = depth;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_tex_image_3D(
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
		GLint offset)
{
	// Get the pixel buffer to load the texture image.
	pixels->gl_tex_image_3D(
			renderer, shared_from_this(), target, level, internalformat, width, height, depth, border, format, type, offset);

	if (level == 0)
	{
		d_width = width;
		d_height = height;
		d_depth = depth;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_copy_tex_image_1D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint internalformat,
		GLint x,
		GLint y,
		GLsizei width,
		GLint border)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind(renderer, shared_from_this(), GL_TEXTURE0, target);

	glCopyTexImage1D(target, level, internalformat, x, y, width, border);

	if (level == 0)
	{
		d_width = width;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_copy_tex_image_2D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint internalformat,
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height,
		GLint border)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind(renderer, shared_from_this(), GL_TEXTURE0, target);

	glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);

	if (level == 0)
	{
		d_width = width;
		d_height = height;
	}

	d_internal_format = internalformat;
}


void
GPlatesOpenGL::GLTexture::gl_tex_sub_image_1D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint xoffset,
		GLsizei width,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	// Load the texture image via pixel buffer emulation functionality to ensure that no
	// native OpenGL pixel buffer objects are bound when the texture is uploaded
	// (because the texture is being sourced from client memory).
	GLPixelBufferImpl::gl_tex_sub_image_1D(
			renderer, shared_from_this(), target, level, xoffset, width, format, type, pixels);
}


void
GPlatesOpenGL::GLTexture::gl_tex_sub_image_1D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint xoffset,
		GLsizei width,
		GLenum format,
		GLenum type,
		const GLPixelBuffer::shared_ptr_to_const_type &pixels,
		GLint offset)
{
	// Get the pixel buffer to load the texture image.
	pixels->gl_tex_sub_image_1D(renderer, shared_from_this(), target, level, xoffset, width, format, type, offset);
}


void
GPlatesOpenGL::GLTexture::gl_tex_sub_image_2D(
		GLRenderer &renderer,
		GLenum target,
		GLint level,
		GLint xoffset,
		GLint yoffset,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	// Load the texture image via pixel buffer emulation functionality to ensure that no
	// native OpenGL pixel buffer objects are bound when the texture is uploaded
	// (because the texture is being sourced from client memory).
	GLPixelBufferImpl::gl_tex_sub_image_2D(
			renderer, shared_from_this(), target, level, xoffset, yoffset, width, height, format, type, pixels);
}


void
GPlatesOpenGL::GLTexture::gl_tex_sub_image_2D(
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
		GLint offset)
{
	// Get the pixel buffer to load the texture image.
	pixels->gl_tex_sub_image_2D(
			renderer, shared_from_this(), target, level, xoffset, yoffset, width, height, format, type, offset);
}


void
GPlatesOpenGL::GLTexture::gl_tex_sub_image_3D(
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
		const GLvoid *pixels)
{
	// Load the texture image via pixel buffer emulation functionality to ensure that no
	// native OpenGL pixel buffer objects are bound when the texture is uploaded
	// (because the texture is being sourced from client memory).
	GLPixelBufferImpl::gl_tex_sub_image_3D(
			renderer, shared_from_this(), target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}


void
GPlatesOpenGL::GLTexture::gl_tex_sub_image_3D(
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
		GLint offset)
{
	// Get the pixel buffer to load the texture image.
	pixels->gl_tex_sub_image_3D(
			renderer, shared_from_this(), target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, offset);
}


bool
GPlatesOpenGL::GLTexture::is_format_floating_point(
		GLint internalformat)
{
	return
			// GL_ARB_texture_float ...
			(internalformat >= GL_RGBA32F_ARB && internalformat <= GL_LUMINANCE_ALPHA16F_ARB) ||
			// GL_ARB_texture_rg ...
			(internalformat >= GL_R16F && internalformat <= GL_RG32F)
#ifdef GL_EXT_packed_float // In case old GLEW header file.
			// GL_EXT_packed_float ...
			|| internalformat == GL_R11F_G11F_B10F_EXT || internalformat == GL_UNSIGNED_INT_10F_11F_11F_REV_EXT
#endif
			;
}
