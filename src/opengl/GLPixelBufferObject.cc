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

#include "GLPixelBufferObject.h"

#include "GLRenderer.h"
#include "GLTexture.h"
#include "GLUtils.h"


GLenum
GPlatesOpenGL::GLPixelBufferObject::get_unpack_target_type()
{
	return GL_PIXEL_UNPACK_BUFFER_ARB;
}


GLenum
GPlatesOpenGL::GLPixelBufferObject::get_pack_target_type()
{
	return GL_PIXEL_PACK_BUFFER_ARB;
}


GPlatesOpenGL::GLPixelBufferObject::GLPixelBufferObject(
		const GLBufferObject::shared_ptr_type &buffer) :
	d_buffer(buffer)
{
	// We should only get here if the pixel buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_pixel_buffer_object),
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLBuffer::shared_ptr_to_const_type
GPlatesOpenGL::GLPixelBufferObject::get_buffer() const
{
	return d_buffer;
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_bind_unpack(
		GLRenderer &renderer) const
{
	renderer.gl_bind_pixel_unpack_buffer_object(
			boost::dynamic_pointer_cast<const GLPixelBufferObject>(shared_from_this()));
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_bind_pack(
		GLRenderer &renderer) const
{
	renderer.gl_bind_pixel_pack_buffer_object(
			boost::dynamic_pointer_cast<const GLPixelBufferObject>(shared_from_this()));
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_read_pixels(
		GLRenderer &renderer,
		GLint x,
		GLint y,
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		GLint offset)
{
	// Use the overload that doesn't require a client memory pointer since we're using
	// the bound buffer object and *not* client memory.
	renderer.gl_read_pixels(x, y, width, height, format, type, offset);
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_tex_image_1D(
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
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Bind this pixel buffer to the *unpack* target.
	GLRenderer::BindBufferObjectAndApply save_restore_bind_pixel_buffer(renderer, d_buffer, get_unpack_target_type());

	glTexImage1D(target, level, internalformat, width, border, format, type, GPLATES_OPENGL_BUFFER_OFFSET(offset));
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_tex_image_2D(
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
	// For cube map textures the target to bind is different than the target specifying the cube face.
	const GLenum bind_target =
			(target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)
			? GL_TEXTURE_CUBE_MAP_ARB
			: target;

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, bind_target);

	// Bind this pixel buffer to the *unpack* target.
	GLRenderer::BindBufferObjectAndApply save_restore_bind_pixel_buffer(renderer, d_buffer, get_unpack_target_type());

	glTexImage2D(target, level, internalformat, width, height, border, format, type, GPLATES_OPENGL_BUFFER_OFFSET(offset));
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_tex_image_3D(
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
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind(renderer, texture, GL_TEXTURE0, target);

	// Bind this pixel buffer to the *unpack* target.
	GLRenderer::BindBufferObjectAndApply save_restore_bind_pixel_buffer(renderer, d_buffer, get_unpack_target_type());

	// The GL_EXT_texture3D extension must be available.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_texture3D),
			GPLATES_ASSERTION_SOURCE);

	glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, GPLATES_OPENGL_BUFFER_OFFSET(offset));
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_tex_sub_image_1D(
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
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Bind this pixel buffer to the *unpack* target.
	GLRenderer::BindBufferObjectAndApply save_restore_bind_pixel_buffer(renderer, d_buffer, get_unpack_target_type());

	glTexSubImage1D(target, level, xoffset, width, format, type, GPLATES_OPENGL_BUFFER_OFFSET(offset));
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_tex_sub_image_2D(
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
	// For cube map textures the target to bind is different than the target specifying the cube face.
	const GLenum bind_target =
			(target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)
			? GL_TEXTURE_CUBE_MAP_ARB
			: target;

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, bind_target);

	// Bind this pixel buffer to the *unpack* target.
	GLRenderer::BindBufferObjectAndApply save_restore_bind_pixel_buffer(renderer, d_buffer, get_unpack_target_type());

	glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, GPLATES_OPENGL_BUFFER_OFFSET(offset));
}


void
GPlatesOpenGL::GLPixelBufferObject::gl_tex_sub_image_3D(
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
	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind_texture(renderer, texture, GL_TEXTURE0, target);

	// Bind this pixel buffer to the *unpack* target.
	GLRenderer::BindBufferObjectAndApply save_restore_bind_pixel_buffer(renderer, d_buffer, get_unpack_target_type());

	// For some reason the GL_EXT_subtexture extension is not well-supported even though pretty much
	// all hardware support it (was introduced in OpenGL 1.2 core).
	// We'll test for GL_EXT_texture3D instead and call the core function glTexSubImage3D
	// instead of the extension function glTexSubImage3DEXT.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_texture3D),
			GPLATES_ASSERTION_SOURCE);

	glTexSubImage3D(
			target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, GPLATES_OPENGL_BUFFER_OFFSET(offset));
}
