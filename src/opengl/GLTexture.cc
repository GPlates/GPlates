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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

	glTexParameteri(target, pname, param);
}


void
GPlatesOpenGL::GLTexture::gl_tex_parameterf(
		GLRenderer &renderer,
		GLenum target,
		GLenum pname,
		GLfloat param)
{
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

	glTexImage1D(target, level, internalformat, width, border, format, type, pixels);

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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);

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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

	// The GL_EXT_texture3D extension must be available.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_texture3D),
			GPLATES_ASSERTION_SOURCE);

	glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels);

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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

	glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
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
	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

	glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
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
	// The GL_EXT_subtexture extension must be available.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_subtexture),
			GPLATES_ASSERTION_SOURCE);

	// Revert our texture binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_texture_and_apply(shared_from_this(), GL_TEXTURE0, target);

	glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
