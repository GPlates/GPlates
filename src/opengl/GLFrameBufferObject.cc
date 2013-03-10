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

namespace GPlatesOpenGL
{
	namespace
	{
		//! Converts attachment GLenum into an index starting at GL_COLOR_ATTACHMENT0_EXT.
		unsigned int
		get_attachment_index(
				GLenum attachment)
		{
			// There are 16 colour attachments (supported by GL_EXT_framebuffer_object),
			// a depth attachment and a stencil attachment.

			if (attachment == GL_DEPTH_ATTACHMENT_EXT)
			{
				return 16;
			}
			else if (attachment == GL_STENCIL_ATTACHMENT_EXT)
			{
				return 17;
			}

			// All colour attachment enums are >= GL_COLOR_ATTACHMENT0_EXT and packed together.
			// Note that if the GL_MAX_COLOR_ATTACHMENTS_EXT query is less than GL_COLOR_ATTACHMENT15_EXT
			// then there will be unused slots in 'd_attachment_points'.
			return attachment - GL_COLOR_ATTACHMENT0_EXT;
		}
	}
}

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


const GLenum GPlatesOpenGL::GLFrameBufferObject::DEFAULT_DRAW_READ_BUFFER = GL_COLOR_ATTACHMENT0_EXT;


void
GPlatesOpenGL::GLFrameBufferObject::gl_generate_mipmap(
		GLRenderer &renderer,
		GLenum texture_target,
		const GLTexture::shared_ptr_to_const_type &texture)
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	//
	// Unbind any currently bound framebuffer object and bind the texture to generate mipmaps for.
	//

	// Revert our framebuffer unbinding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the unbind to OpenGL before we call OpenGL directly.
	GLRenderer::UnbindFrameBufferAndApply save_restore_unbind(renderer);

	// Doesn't really matter which texture unit we bind on so choose unit zero since all hardware supports it.
	// Revert our texture binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindTextureAndApply save_restore_bind(renderer, texture, GL_TEXTURE0, texture_target);

	glGenerateMipmapEXT(texture_target);
}


GPlatesOpenGL::GLFrameBufferObject::GLFrameBufferObject(
		GLRenderer &renderer) :
	d_resource(
			resource_type::create(
					renderer.get_context().get_non_shared_state()->get_frame_buffer_object_resource_manager()))
{
	// We should only get here if the framebuffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_framebuffer_object),
			GPLATES_ASSERTION_SOURCE);

	// Resize to the maximum number of colour attachments supported by GL_EXT_framebuffer_object.
	// This is 16 colour attachments (supported by GL_EXT_framebuffer_object), a depth attachment and a stencil attachment.
	// NOTE: This is more than the runtime system may support depending on GL_MAX_COLOR_ATTACHMENTS_EXT.
	d_attachment_points.resize(16 + 2);
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_texture_1D(
		GLRenderer &renderer,
		GLenum texture_target,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLenum attachment)
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

	// Attachment must be a valid value.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				attachment < GL_COLOR_ATTACHMENT0_EXT +
						renderer.get_capabilities().framebuffer.gl_max_color_attachments) ||
			(attachment == GL_DEPTH_ATTACHMENT_EXT) ||
			(attachment == GL_STENCIL_ATTACHMENT_EXT),
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTexture1DEXT(
			GL_FRAMEBUFFER_EXT,
			attachment,
			texture_target,
			texture->get_texture_resource_handle(),
			level);

	// Keep track of the attachment.
	const AttachmentPoint attachment_point(
			attachment,
			ATTACHMENT_TEXTURE_1D,
			texture_target,
			texture,
			level);
	d_attachment_points[get_attachment_index(attachment)] = attachment_point;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_texture_2D(
		GLRenderer &renderer,
		GLenum texture_target,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLenum attachment)
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

	// Attachment must be a valid value.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				attachment < GL_COLOR_ATTACHMENT0_EXT +
						renderer.get_capabilities().framebuffer.gl_max_color_attachments) ||
			(attachment == GL_DEPTH_ATTACHMENT_EXT) ||
			(attachment == GL_STENCIL_ATTACHMENT_EXT),
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTexture2DEXT(
			GL_FRAMEBUFFER_EXT,
			attachment,
			texture_target,
			texture->get_texture_resource_handle(),
			level);

	// Keep track of the attachment.
	const AttachmentPoint attachment_point(
			attachment,
			ATTACHMENT_TEXTURE_2D,
			texture_target,
			texture,
			level);
	d_attachment_points[get_attachment_index(attachment)] = attachment_point;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_texture_3D(
		GLRenderer &renderer,
		GLenum texture_target,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLint zoffset,
		GLenum attachment)
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

	// Attachment must be a valid value.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				attachment < GL_COLOR_ATTACHMENT0_EXT +
						renderer.get_capabilities().framebuffer.gl_max_color_attachments) ||
			(attachment == GL_DEPTH_ATTACHMENT_EXT) ||
			(attachment == GL_STENCIL_ATTACHMENT_EXT),
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTexture3DEXT(
			GL_FRAMEBUFFER_EXT,
			attachment,
			texture_target,
			texture->get_texture_resource_handle(),
			level,
			zoffset);

	// Keep track of the attachment.
	const AttachmentPoint attachment_point(
			attachment,
			ATTACHMENT_TEXTURE_3D,
			texture_target,
			texture,
			level,
			zoffset);
	d_attachment_points[get_attachment_index(attachment)] = attachment_point;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_texture_array_layer(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLint layer,
		GLenum attachment)
{
	//PROFILE_FUNC();

	// The texture must be initialised with a width and a height minimum.
	// This is for 1D array textures. 2D array textures also require depth but we don't check
	// because we don't know the texture target (not needed for glFramebufferTextureLayerEXT).
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture->get_width() && texture->get_height(),
			GPLATES_ASSERTION_SOURCE);

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// Attachment must be a valid value.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				attachment < GL_COLOR_ATTACHMENT0_EXT +
						renderer.get_capabilities().framebuffer.gl_max_color_attachments) ||
			(attachment == GL_DEPTH_ATTACHMENT_EXT) ||
			(attachment == GL_STENCIL_ATTACHMENT_EXT),
			GPLATES_ASSERTION_SOURCE);

	// The GL_EXT_texture_array extension is required for this call.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_texture_array),
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTextureLayerEXT(
			GL_FRAMEBUFFER_EXT,
			attachment,
			texture->get_texture_resource_handle(),
			level,
			layer);

	// Keep track of the attachment.
	const AttachmentPoint attachment_point(
			attachment,
			texture,
			level,
			layer);
	d_attachment_points[get_attachment_index(attachment)] = attachment_point;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_texture_array(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &texture,
		GLint level,
		GLenum attachment)
{
	//PROFILE_FUNC();

	// The texture must be initialised with a width and a height minimum.
	// This is for 1D array textures. 2D array textures also require depth but we don't check
	// because we don't know the texture target (not needed for glFramebufferTextureEXT).
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture->get_width() && texture->get_height(),
			GPLATES_ASSERTION_SOURCE);

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// Attachment must be a valid value.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				attachment < GL_COLOR_ATTACHMENT0_EXT +
						renderer.get_capabilities().framebuffer.gl_max_color_attachments) ||
			(attachment == GL_DEPTH_ATTACHMENT_EXT) ||
			(attachment == GL_STENCIL_ATTACHMENT_EXT),
			GPLATES_ASSERTION_SOURCE);

	// The GL_EXT_geometry_shader4 extension is required for this call.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_EXT_geometry_shader4),
			GPLATES_ASSERTION_SOURCE);

	// Attach to the texture.
	glFramebufferTextureEXT(
			GL_FRAMEBUFFER_EXT,
			attachment,
			texture->get_texture_resource_handle(),
			level);

	// Keep track of the attachment.
	const AttachmentPoint attachment_point(
			attachment,
			texture,
			level);
	d_attachment_points[get_attachment_index(attachment)] = attachment_point;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_attach_render_buffer(
		GLRenderer &renderer,
		const GLRenderBufferObject::shared_ptr_to_const_type &render_buffer,
		GLenum attachment)
{
	//PROFILE_FUNC();

	// The render buffer must have its storage initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			render_buffer->get_dimensions(),
			GPLATES_ASSERTION_SOURCE);

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// Attachment must be a valid value.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				attachment < GL_COLOR_ATTACHMENT0_EXT +
						renderer.get_capabilities().framebuffer.gl_max_color_attachments) ||
			(attachment == GL_DEPTH_ATTACHMENT_EXT) ||
			(attachment == GL_STENCIL_ATTACHMENT_EXT),
			GPLATES_ASSERTION_SOURCE);

	// Attach to the render buffer.
	glFramebufferRenderbufferEXT(
			GL_FRAMEBUFFER_EXT,
			attachment,
			GL_RENDERBUFFER_EXT,
			render_buffer->get_render_buffer_resource_handle());

	// Keep track of the attachment.
	const AttachmentPoint attachment_point(attachment, render_buffer);
	d_attachment_points[get_attachment_index(attachment)] = attachment_point;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_detach(
		GLRenderer &renderer,
		GLenum attachment)
{
	//PROFILE_FUNC();

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// Attachment must be a valid value.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(attachment >= GL_COLOR_ATTACHMENT0_EXT &&
				attachment < GL_COLOR_ATTACHMENT0_EXT +
						renderer.get_capabilities().framebuffer.gl_max_color_attachments) ||
			(attachment == GL_DEPTH_ATTACHMENT_EXT) ||
			(attachment == GL_STENCIL_ATTACHMENT_EXT),
			GPLATES_ASSERTION_SOURCE);

	const boost::optional<AttachmentPoint> &attachment_point = d_attachment_points[get_attachment_index(attachment)];
	if (!attachment_point)
	{
		qWarning() << "GLFrameBufferObject::gl_detach: Attempted to detach unattached texture or render buffer.";
		return;
	}

	// Detach by binding to texture object zero (or render buffer zero).
	//
	// NOTE: I don't think we need to match the function call and parameters when the object
	// is zero (at least the parameters are supposed to be ignored) but we'll do it anyway.
	switch (attachment_point->attachment_type)
	{
	case ATTACHMENT_TEXTURE_1D:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attachment_point->attachment_target &&
					attachment_point->texture &&
					attachment_point->texture_level,
				GPLATES_ASSERTION_SOURCE);
		glFramebufferTexture1DEXT(
				GL_FRAMEBUFFER_EXT,
				attachment_point->attachment,
				attachment_point->attachment_target.get(),
				0/*texture*/,
				attachment_point->texture_level.get());
		break;

	case ATTACHMENT_TEXTURE_2D:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attachment_point->attachment_target &&
					attachment_point->texture &&
					attachment_point->texture_level,
				GPLATES_ASSERTION_SOURCE);
		glFramebufferTexture2DEXT(
				GL_FRAMEBUFFER_EXT,
				attachment_point->attachment,
				attachment_point->attachment_target.get(),
				0/*texture*/,
				attachment_point->texture_level.get());
		break;

	case ATTACHMENT_TEXTURE_3D:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attachment_point->attachment_target &&
					attachment_point->texture &&
					attachment_point->texture_level &&
					attachment_point->texture_zoffset,
				GPLATES_ASSERTION_SOURCE);
		glFramebufferTexture3DEXT(
				GL_FRAMEBUFFER_EXT,
				attachment_point->attachment,
				attachment_point->attachment_target.get(),
				0/*texture*/,
				attachment_point->texture_level.get(),
				attachment_point->texture_zoffset.get());
		break;

	case ATTACHMENT_TEXTURE_ARRAY_LAYER:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attachment_point->texture &&
					attachment_point->texture_level &&
					attachment_point->texture_zoffset,
				GPLATES_ASSERTION_SOURCE);
		glFramebufferTextureLayerEXT(
				GL_FRAMEBUFFER_EXT,
				attachment_point->attachment,
				0/*texture*/,
				attachment_point->texture_level.get(),
				attachment_point->texture_zoffset.get());
		break;

	case ATTACHMENT_TEXTURE_ARRAY:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attachment_point->texture && attachment_point->texture_level,
				GPLATES_ASSERTION_SOURCE);
		glFramebufferTextureEXT(
				GL_FRAMEBUFFER_EXT,
				attachment_point->attachment,
				0/*texture*/,
				attachment_point->texture_level.get());
		break;

	case ATTACHMENT_RENDER_BUFFER:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				attachment_point->attachment_target,
				GPLATES_ASSERTION_SOURCE);
		glFramebufferRenderbufferEXT(
				GL_FRAMEBUFFER_EXT,
				attachment_point->attachment,
				attachment_point->attachment_target.get(),
				0/*render buffer*/);
		break;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// No longer tracking the attachment point.
	d_attachment_points[get_attachment_index(attachment)] = boost::none;
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_detach_all(
		GLRenderer &renderer)
{
	// Detach any currently attached attachment points.
	BOOST_FOREACH(const boost::optional<AttachmentPoint> &attachment_point, d_attachment_points)
	{
		if (attachment_point)
		{
			gl_detach(renderer, attachment_point->attachment);
		}
	}
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_draw_buffers(
		GLRenderer &renderer,
		const std::vector<GLenum> &bufs)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!bufs.empty() &&
				bufs.size() <= renderer.get_capabilities().framebuffer.gl_max_draw_buffers,
			GPLATES_ASSERTION_SOURCE);

	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	// If just one buffer then use the OpenGL 1.1 function.
	if (bufs.size() == 1)
	{
		glDrawBuffer(bufs[0]);
		return;
	}
	// Otherwise use the GL_ARB_draw_buffers extension for multiple buffers.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_draw_buffers),
			GPLATES_ASSERTION_SOURCE);

	glDrawBuffersARB(bufs.size(), &bufs[0]);
}


void
GPlatesOpenGL::GLFrameBufferObject::gl_read_buffer(
		GLRenderer &renderer,
		GLenum mode)
{
	// Revert our framebuffer binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindFrameBufferAndApply save_restore_bind(renderer, shared_from_this());

	glReadBuffer(mode);
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


GPlatesOpenGL::GLFrameBufferObject::AttachmentPoint::AttachmentPoint(
		GLenum attachment_,
		AttachmentType attachment_type_,
		GLenum attachment_target_,
		const GLTexture::shared_ptr_to_const_type &texture_,
		GLint texture_level_,
		boost::optional<GLint> texture_zoffset_) :
	attachment(attachment_),
	attachment_type(attachment_type_),
	attachment_target(attachment_target_),
	texture(texture_),
	texture_level(texture_level_),
	texture_zoffset(texture_zoffset_)
{
}


GPlatesOpenGL::GLFrameBufferObject::AttachmentPoint::AttachmentPoint(
		GLenum attachment_,
		const GLTexture::shared_ptr_to_const_type &texture_,
		GLint texture_level_,
		GLint texture_layer_) :
	attachment(attachment_),
	attachment_type(ATTACHMENT_TEXTURE_ARRAY_LAYER),
	texture(texture_),
	texture_level(texture_level_),
	texture_zoffset(texture_layer_)
{
}


GPlatesOpenGL::GLFrameBufferObject::AttachmentPoint::AttachmentPoint(
		GLenum attachment_,
		const GLTexture::shared_ptr_to_const_type &texture_,
		GLint texture_level_) :
	attachment(attachment_),
	attachment_type(ATTACHMENT_TEXTURE_ARRAY),
	texture(texture_),
	texture_level(texture_level_)
{
}


GPlatesOpenGL::GLFrameBufferObject::AttachmentPoint::AttachmentPoint(
		GLenum attachment_,
		const GLRenderBufferObject::shared_ptr_to_const_type &render_buffer_) :
	attachment(attachment_),
	attachment_type(ATTACHMENT_RENDER_BUFFER),
	attachment_target(GL_RENDERBUFFER_EXT),
	render_buffer(render_buffer_)
{
}

// We use macros in <GL/glew.h> that contain old-style casts.
ENABLE_GCC_WARNING("-Wold-style-cast")
