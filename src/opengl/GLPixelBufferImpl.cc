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


/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLPixelBufferImpl.h"

#include "GLRenderer.h"
#include "GLTexture.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::GLPixelBufferImpl::GLPixelBufferImpl(
		const GLBufferImpl::shared_ptr_type &buffer) :
	d_buffer(buffer)
{
	// We should only get here if the pixel buffer object extension is *not* supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!GPLATES_OPENGL_BOOL(GLEW_ARB_pixel_buffer_object),
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLBuffer::shared_ptr_to_const_type
GPlatesOpenGL::GLPixelBufferImpl::get_buffer() const
{
	return d_buffer;
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_bind_unpack(
		GLRenderer &renderer) const
{
	// We're not using pixel buffer objects for the *unpack* target so there should be none bound.
	renderer.gl_unbind_pixel_unpack_buffer_object();
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_bind_pack(
		GLRenderer &renderer) const
{
	// We're not using pixel buffer objects for the *pack* target so there should be none bound.
	renderer.gl_unbind_pixel_pack_buffer_object();
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_read_pixels(
		GLRenderer &renderer,
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		GLint offset)
{
	renderer.gl_read_pixels(x, y, width, height, format, type, offset, d_buffer);
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_tex_image_1D(
		GLRenderer &renderer,
		const boost::shared_ptr<const GLTexture> &texture,
		GLenum target,
		GLint level,
		GLint internalformat,
		GLsizei width,
		GLint border,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Unbind pixel buffers on the *unpack* target so that client memory arrays are used.
	GLRenderer::UnbindBufferObjectAndApply save_restore_unbind_pixel_buffer(renderer, GLBuffer::TARGET_PIXEL_UNPACK_BUFFER);

	glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_tex_image_2D(
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
		const GLvoid *pixels)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Unbind pixel buffers on the *unpack* target so that client memory arrays are used.
	GLRenderer::UnbindBufferObjectAndApply save_restore_unbind_pixel_buffer(renderer, GLBuffer::TARGET_PIXEL_UNPACK_BUFFER);

	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_tex_image_3D(
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
		const GLvoid *pixels)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind(renderer, texture, GL_TEXTURE0, target);

	// Unbind pixel buffers on the *unpack* target so that client memory arrays are used.
	GLRenderer::UnbindBufferObjectAndApply save_restore_unbind_pixel_buffer(renderer, GLBuffer::TARGET_PIXEL_UNPACK_BUFFER);

	// The GL_EXT_texture3D extension must be available.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_texture3D),
			GPLATES_ASSERTION_SOURCE);

	glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels);
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_tex_sub_image_1D(
		GLRenderer &renderer,
		const boost::shared_ptr<const GLTexture> &texture,
		GLenum target,
		GLint level,
		GLint xoffset,
		GLsizei width,
		GLenum format,
		GLenum type,
		const GLvoid *pixels)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Unbind pixel buffers on the *unpack* target so that client memory arrays are used.
	GLRenderer::UnbindBufferObjectAndApply save_restore_unbind_pixel_buffer(renderer, GLBuffer::TARGET_PIXEL_UNPACK_BUFFER);

	glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_tex_sub_image_2D(
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
		const GLvoid *pixels)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Unbind pixel buffers on the *unpack* target so that client memory arrays are used.
	GLRenderer::UnbindBufferObjectAndApply save_restore_unbind_pixel_buffer(renderer, GLBuffer::TARGET_PIXEL_UNPACK_BUFFER);

	glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}


void
GPlatesOpenGL::GLPixelBufferImpl::gl_tex_sub_image_3D(
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
		const GLvoid *pixels)
{
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Unbind pixel buffers on the *unpack* target so that client memory arrays are used.
	GLRenderer::UnbindBufferObjectAndApply save_restore_unbind_pixel_buffer(renderer, GLBuffer::TARGET_PIXEL_UNPACK_BUFFER);

	// The GL_EXT_subtexture extension must be available.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_subtexture),
			GPLATES_ASSERTION_SOURCE);

	glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}