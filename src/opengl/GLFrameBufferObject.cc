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

#include <boost/cast.hpp>
#include <boost/foreach.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLFrameBufferObject.h"

#include "GLContext.h"
#include "GLRenderer.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GLint
GPlatesOpenGL::GLFrameBufferObject::Allocator::allocate()
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	GLuint fbo;
	glGenFramebuffersEXT(1, &fbo);
	return fbo;
}


void
GPlatesOpenGL::GLFrameBufferObject::Allocator::deallocate(
		GLuint fbo)
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	glDeleteFramebuffersEXT(1, &fbo);
}


GPlatesOpenGL::GLFrameBufferObject::GLFrameBufferObject(
		const resource_type::non_null_ptr_to_const_type &resource) :
	d_resource(resource)
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	// Resize to the maximum number of colour attachments.
	d_colour_attachments.resize(
			GLContext::get_parameters().framebuffer.gl_max_color_attachments);
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_1D(
		GLRenderer &renderer,
		GLenum texture_target,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLenum colour_attachment)
{
	//PROFILE_FUNC();

	// The texture must be initialised with a width and no height and no depth.
	// If not then its either a 2D/3D texture or it has not been initialised with GLTexture::gl_tex_image_1D.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture->get_width() && !texture->get_height() && !texture->get_depth(),
			GPLATES_ASSERTION_SOURCE);

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object) &&
				colour_attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				colour_attachment < GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments,
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTexture1DEXT(
			GL_FRAMEBUFFER_EXT,
			colour_attachment,
			texture_target,
			texture->get_texture_resource_handle(),
			level);

	// Keep track of the colour attachment.
	const ColourAttachment attachment =
	{
		colour_attachment,
		GL_TEXTURE_1D, // glFramebufferTexture1DEXT
		texture_target,
		texture,
		level,
		boost::none
	};
	d_colour_attachments[colour_attachment - GL_COLOR_ATTACHMENT0_EXT] = attachment;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_2D(
		GLRenderer &renderer,
		GLenum texture_target,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLenum colour_attachment)
{
	//PROFILE_FUNC();

	// The texture must be initialised with a width and a height and no depth.
	// If not then its either a 1D/3D texture or it has not been initialised with GLTexture::gl_tex_image_2D.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture->get_width() && texture->get_height() && !texture->get_depth(),
			GPLATES_ASSERTION_SOURCE);

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object) &&
				colour_attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				colour_attachment < GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments,
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTexture2DEXT(
			GL_FRAMEBUFFER_EXT,
			colour_attachment,
			texture_target,
			texture->get_texture_resource_handle(),
			level);

	// Keep track of the colour attachment.
	const ColourAttachment attachment =
	{
		colour_attachment,
		GL_TEXTURE_2D, // glFramebufferTexture2DEXT
		texture_target,
		texture,
		level,
		boost::none
	};
	d_colour_attachments[colour_attachment - GL_COLOR_ATTACHMENT0_EXT] = attachment;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_3D(
		GLRenderer &renderer,
		GLenum texture_target,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLint zoffset,
		GLenum colour_attachment)
{
	//PROFILE_FUNC();

	// The texture must be initialised with a width and a height and a depth.
	// If not then its either a 1D/2D texture or it has not been initialised with GLTexture::gl_tex_image_3D.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture->get_width() && texture->get_height() && texture->get_depth(),
			GPLATES_ASSERTION_SOURCE);

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object) &&
				colour_attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				colour_attachment < GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments,
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTexture3DEXT(
			GL_FRAMEBUFFER_EXT,
			colour_attachment,
			texture_target,
			texture->get_texture_resource_handle(),
			level,
			zoffset);

	// Keep track of the colour attachment.
	const ColourAttachment attachment =
	{
		colour_attachment,
		GL_TEXTURE_3D_EXT, // glFramebufferTexture3DEXT
		texture_target,
		texture,
		level,
		zoffset
	};
	d_colour_attachments[colour_attachment - GL_COLOR_ATTACHMENT0_EXT] = attachment;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_detach(
		GLRenderer &renderer,
		GLenum colour_attachment)
{
	//PROFILE_FUNC();

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object) &&
				colour_attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				colour_attachment < GL_COLOR_ATTACHMENT0_EXT + GLContext::get_parameters().framebuffer.gl_max_color_attachments,
			GPLATES_ASSERTION_SOURCE);

	const boost::optional<ColourAttachment> &attachment =
			d_colour_attachments[colour_attachment - GL_COLOR_ATTACHMENT0_EXT];
	if (!attachment)
	{
		qWarning() << "GLFrameBufferObject::gl_detach: Attempted to detach unattached texture.";
		return;
	}

	// Detach by binding to texture object zero.
	//
	// NOTE: I don't think we need to match the function call and parameters when the texture object
	// is zero (at least the parameters are supposed to be ignored) but we'll do it anyway.
	switch (attachment->framebuffer_texture_type)
	{
	case GL_TEXTURE_1D:
		glFramebufferTexture1DEXT(
				GL_FRAMEBUFFER_EXT,
				attachment->attachment,
				attachment->texture_target,
				0/*texture*/,
				attachment->level);
		break;

	case GL_TEXTURE_2D:
		glFramebufferTexture2DEXT(
				GL_FRAMEBUFFER_EXT,
				attachment->attachment,
				attachment->texture_target,
				0/*texture*/,
				attachment->level);
		break;

	case GL_TEXTURE_3D_EXT:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attachment->zoffset,
				GPLATES_ASSERTION_SOURCE);
		glFramebufferTexture3DEXT(
				GL_FRAMEBUFFER_EXT,
				attachment->attachment,
				attachment->texture_target,
				0/*texture*/,
				attachment->level,
				attachment->zoffset.get());
		break;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// No longer tracking the colour attachment point.
	d_colour_attachments[colour_attachment - GL_COLOR_ATTACHMENT0_EXT] = boost::none;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_detach_all(
		GLRenderer &renderer)
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	// Detach any currently attached attachment points.
	BOOST_FOREACH(const boost::optional<ColourAttachment> &colour_attachment, d_colour_attachments)
	{
		if (colour_attachment)
		{
			gl_detach(renderer, colour_attachment->attachment);
		}
	}
}


bool
GPlatesOpenGL::GLFrameBufferObject::gl_check_frame_buffer_status(
		GLRenderer &renderer) const
{
	PROFILE_FUNC();

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE_EXT:
		return true;

	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
		return false;

	default:
		break;
	}

	// If the status is neither 'GL_FRAMEBUFFER_COMPLETE_EXT' nor 'GL_FRAMEBUFFER_UNSUPPORTED_EXT'
	// then an assertion/exception is triggered as this represents a programming error.
	qWarning() << "glCheckFramebufferStatusEXT returned status other than 'GL_FRAMEBUFFER_COMPLETE_EXT'"
			" or 'GL_FRAMEBUFFER_UNSUPPORTED_EXT'";
	GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);

	// Keep compiler happy - we can't get here due to above 'Abort'.
	return false;
}

// We use macros in <GL/glew.h> that contain old-style casts.
ENABLE_GCC_WARNING("-Wold-style-cast")
