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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include "GLFramebuffer.h"

#include "GL.h"
#include "GLContext.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::GLFramebuffer::GLFramebuffer(
		GL &gl) :
	d_object_state(gl.get_capabilities().gl_max_color_attachments)
{
}


GLuint
GPlatesOpenGL::GLFramebuffer::get_resource_handle(
		GL &gl) const
{
	return get_object_state_for_current_context(gl).resource->get_resource_handle();
}


void
GPlatesOpenGL::GLFramebuffer::synchronise_current_context(
		GL &gl,
		GLenum target)
{
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);

	// Return early if the current context state is already up-to-date.
	// This is an optimisation (it's not strictly necessary).
	if (d_object_state_subject.is_observer_up_to_date(current_context_object_state.object_state_observer))
	{
		return;
	}

	//
	// Synchronise draw/read buffers.
	//

	// Draw buffers.
	if (current_context_object_state.object_state.draw_buffers_ != d_object_state.draw_buffers_)
	{
		glDrawBuffers(d_object_state.draw_buffers_.size(), d_object_state.draw_buffers_.data());

		// Record updated context state.
		current_context_object_state.object_state.draw_buffers_ = d_object_state.draw_buffers_;
	}

	// Read buffer.
	if (current_context_object_state.object_state.read_buffer_ != d_object_state.read_buffer_)
	{
		glReadBuffer(d_object_state.read_buffer_);

		// Record updated context state.
		current_context_object_state.object_state.read_buffer_ = d_object_state.read_buffer_;
	}

	//
	// Synchronise the attachments.
	//

	const unsigned int num_color_attachments = gl.get_capabilities().gl_max_color_attachments;
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_object_state.color_attachments.size() == num_color_attachments &&
				current_context_object_state.object_state.color_attachments.size() == num_color_attachments,
			GPLATES_ASSERTION_SOURCE);

	// Synchronise colour attachments.
	for (GLuint color_attachment_index = 0; color_attachment_index < num_color_attachments; ++color_attachment_index)
	{
		const GLenum color_attachment = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + color_attachment_index);

		const ObjectState::Attachment &color_attachment_state =
				d_object_state.color_attachments[color_attachment_index];
		ObjectState::Attachment &context_color_attachment_state =
				current_context_object_state.object_state.color_attachments[color_attachment_index];

		synchronise_current_context_attachment(
				gl, target, color_attachment, color_attachment_state, context_color_attachment_state);
	}

	// Synchronise depth attachment.
	synchronise_current_context_attachment(
			gl, target, GL_DEPTH_ATTACHMENT,
			d_object_state.depth_attachment,
			current_context_object_state.object_state.depth_attachment);

	// Synchronise stencil attachment.
	synchronise_current_context_attachment(
			gl, target, GL_STENCIL_ATTACHMENT,
			d_object_state.stencil_attachment,
			current_context_object_state.object_state.stencil_attachment);

	// The current context state is now up-to-date.
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


void
GPlatesOpenGL::GLFramebuffer::synchronise_current_context_attachment(
		GL &gl,
		GLenum target,
		GLenum attachment,
		const ObjectState::Attachment &attachment_state,
		ObjectState::Attachment &context_attachment_state)
{
	// Return early if the state does not differ.
	if (context_attachment_state == attachment_state)
	{
		return;
	}

	if (attachment_state.type)
	{
		//
		// Attach in context.
		//
		// Currently context is either detached, or attached but with a different attachment state.
		//
		switch (attachment_state.type.get())
		{
		case ObjectState::Attachment::FRAMEBUFFER_RENDERBUFFER:
			// Attach the renderbuffer.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					attachment_state.renderbuffer,
					GPLATES_ASSERTION_SOURCE);
			glFramebufferRenderbuffer(
					target,
					attachment,
					attachment_state.renderbuffertarget,
					attachment_state.renderbuffer.get()->get_resource_handle());
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE:
			// Attach the texture.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					attachment_state.texture,
					GPLATES_ASSERTION_SOURCE);
			glFramebufferTexture(
					target,
					attachment,
					attachment_state.texture.get()->get_resource_handle(),
					attachment_state.level);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_1D:
			// Attach the texture.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					attachment_state.texture,
					GPLATES_ASSERTION_SOURCE);
			glFramebufferTexture1D(
					target,
					attachment,
					attachment_state.textarget,
					attachment_state.texture.get()->get_resource_handle(),
					attachment_state.level);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_2D:
			// Attach the texture.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					attachment_state.texture,
					GPLATES_ASSERTION_SOURCE);
			glFramebufferTexture2D(
					target,
					attachment,
					attachment_state.textarget,
					attachment_state.texture.get()->get_resource_handle(),
					attachment_state.level);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_3D:
			// Attach the texture.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					attachment_state.texture,
					GPLATES_ASSERTION_SOURCE);
			glFramebufferTexture3D(
					target,
					attachment,
					attachment_state.textarget,
					attachment_state.texture.get()->get_resource_handle(),
					attachment_state.level,
					attachment_state.layer);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_LAYER:
			// Attach the texture.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					attachment_state.texture,
					GPLATES_ASSERTION_SOURCE);
			glFramebufferTextureLayer(
					target,
					attachment,
					attachment_state.texture.get()->get_resource_handle(),
					attachment_state.level,
					attachment_state.layer);
			break;

		default:
			// Shouldn't be able to get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}
	}
	else  // attachment is in detached state ...
	{
		// Both states are different so they cannot both be in the detached state ('type' is none).
		// Therefore the context attachment must be in an attached state.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				context_attachment_state.type,
				GPLATES_ASSERTION_SOURCE);

		//
		// Detach in context.
		//
		// Currently context is attached.
		//
		// Note: We could probably use a single function (like 'glFramebufferRenderbuffer')
		//       to detach in all cases (especially since the extra parameters like
		//       renderbuffer/texture target, texture level, texture layer are ignored when detaching).
		//       However to be safest (in case the OpenGL driver causes problems) we'll detach in the
		//       same way the renderbuffer/texture object was attached.
		//
		switch (context_attachment_state.type.get())
		{
		case ObjectState::Attachment::FRAMEBUFFER_RENDERBUFFER:
			// Detach the currently attached renderbuffer.
			glFramebufferRenderbuffer(
					target,
					attachment,
					context_attachment_state.renderbuffertarget,
					0/*renderbuffer*/);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE:
			// Detach the currently attached texture.
			glFramebufferTexture(
					target,
					attachment,
					0/*texture*/,
					context_attachment_state.level/*ignored*/);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_1D:
			// Detach the currently attached texture.
			glFramebufferTexture1D(
					target,
					attachment,
					context_attachment_state.textarget/*ignored*/,
					0/*texture*/,
					context_attachment_state.level/*ignored*/);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_2D:
			// Detach the currently attached texture.
			glFramebufferTexture2D(
					target,
					attachment,
					context_attachment_state.textarget/*ignored*/,
					0/*texture*/,
					context_attachment_state.level/*ignored*/);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_3D:
			// Detach the currently attached texture.
			glFramebufferTexture3D(
					target,
					attachment,
					context_attachment_state.textarget/*ignored*/,
					0/*texture*/,
					context_attachment_state.level/*ignored*/,
					context_attachment_state.layer/*ignored*/);
			break;

		case ObjectState::Attachment::FRAMEBUFFER_TEXTURE_LAYER:
			// Detach the currently attached texture.
			glFramebufferTextureLayer(
					target,
					attachment,
					0/*texture*/,
					context_attachment_state.level/*ignored*/,
					context_attachment_state.layer/*ignored*/);
			break;

		default:
			// Shouldn't be able to get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}
	}

	// Record updated context state.
	context_attachment_state = attachment_state;
}


void
GPlatesOpenGL::GLFramebuffer::draw_buffer(
		GL &gl,
		GLenum buf)
{
	glDrawBuffer(buf);

	// Record the new framebuffer internal state.
	d_object_state.draw_buffers_.resize(1, buf);

	// Note that the framebuffer object in current context has been updated,
	// so we'll need to update our record of its state also.
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);
	current_context_object_state.object_state.draw_buffers_ = d_object_state.draw_buffers_;

	// Invalidate all contexts except the current one.
	// When we switch to the next context it will be out-of-date and require synchronisation.
	d_object_state_subject.invalidate();
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


void
GPlatesOpenGL::GLFramebuffer::draw_buffers(
		GL &gl,
		const std::vector<GLenum> &bufs)
{
	GPlatesGlobal::Assert<OpenGLException>(
			bufs.size() < gl.get_capabilities().gl_max_draw_buffers,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer draw buffers exceed GL_MAX_DRAW_BUFFERS.");

	glDrawBuffers(bufs.size(), bufs.data());

	// Record the new framebuffer internal state.
	d_object_state.draw_buffers_ = bufs;

	// Note that the framebuffer object in current context has been updated,
	// so we'll need to update our record of its state also.
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);
	current_context_object_state.object_state.draw_buffers_ = d_object_state.draw_buffers_;

	// Invalidate all contexts except the current one.
	// When we switch to the next context it will be out-of-date and require synchronisation.
	d_object_state_subject.invalidate();
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


void
GPlatesOpenGL::GLFramebuffer::read_buffer(
		GL &gl,
		GLenum src)
{
	glReadBuffer(src);

	// Record the new framebuffer internal state.
	d_object_state.read_buffer_ = src;

	// Note that the framebuffer object in current context has been updated,
	// so we'll need to update our record of its state also.
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);
	current_context_object_state.object_state.read_buffer_ = d_object_state.read_buffer_;

	// Invalidate all contexts except the current one.
	// When we switch to the next context it will be out-of-date and require synchronisation.
	d_object_state_subject.invalidate();
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


void
GPlatesOpenGL::GLFramebuffer::framebuffer_renderbuffer(
		GL &gl,
		GLenum target,
		GLenum attachment,
		GLenum renderbuffertarget,
		boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer)
{
	// Either attach the specified renderbuffer or detach.
	GLuint renderbuffer_resource = 0;
	if (renderbuffer)
	{
		renderbuffer_resource = renderbuffer.get()->get_resource_handle();
	}

	glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer_resource);

	// Record the new framebuffer internal state, and the state associated with the current context.
	ObjectState::Attachment attachment_state; // Detached state.
	if (renderbuffer)
	{
		// Attached state.
		attachment_state.type = ObjectState::Attachment::FRAMEBUFFER_RENDERBUFFER;
		attachment_state.renderbuffertarget = renderbuffertarget;
		attachment_state.renderbuffer = renderbuffer;
	}

	set_attachment(gl, attachment, attachment_state);
}


void
GPlatesOpenGL::GLFramebuffer::framebuffer_texture(
		GL &gl,
		GLenum target,
		GLenum attachment,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	glFramebufferTexture(target, attachment, texture_resource, level);

	// Record the new framebuffer internal state, and the state associated with the current context.
	ObjectState::Attachment attachment_state; // Detached state.
	if (texture)
	{
		// Attached state.
		attachment_state.type = ObjectState::Attachment::FRAMEBUFFER_TEXTURE;
		attachment_state.texture = texture;
		attachment_state.level = level;
	}

	set_attachment(gl, attachment, attachment_state);
}


void
GPlatesOpenGL::GLFramebuffer::framebuffer_texture_1D(
		GL &gl,
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	glFramebufferTexture1D(target, attachment, textarget, texture_resource, level);

	// Record the new framebuffer internal state, and the state associated with the current context.
	ObjectState::Attachment attachment_state; // Detached state.
	if (texture)
	{
		// Attached state.
		attachment_state.type = ObjectState::Attachment::FRAMEBUFFER_TEXTURE_1D;
		attachment_state.textarget = textarget;
		attachment_state.texture = texture;
		attachment_state.level = level;
	}

	set_attachment(gl, attachment, attachment_state);
}


void
GPlatesOpenGL::GLFramebuffer::framebuffer_texture_2D(
		GL &gl,
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	glFramebufferTexture2D(target, attachment, textarget, texture_resource, level);

	// Record the new framebuffer internal state, and the state associated with the current context.
	ObjectState::Attachment attachment_state; // Detached state.
	if (texture)
	{
		// Attached state.
		attachment_state.type = ObjectState::Attachment::FRAMEBUFFER_TEXTURE_2D;
		attachment_state.textarget = textarget;
		attachment_state.texture = texture;
		attachment_state.level = level;
	}

	set_attachment(gl, attachment, attachment_state);
}


void
GPlatesOpenGL::GLFramebuffer::framebuffer_texture_3D(
		GL &gl,
		GLenum target,
		GLenum attachment,
		GLenum textarget,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLint layer)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	glFramebufferTexture3D(target, attachment, textarget, texture_resource, level, layer);

	// Record the new framebuffer internal state, and the state associated with the current context.
	ObjectState::Attachment attachment_state; // Detached state.
	if (texture)
	{
		// Attached state.
		attachment_state.type = ObjectState::Attachment::FRAMEBUFFER_TEXTURE_3D;
		attachment_state.textarget = textarget;
		attachment_state.texture = texture;
		attachment_state.level = level;
		attachment_state.layer = layer;
	}

	set_attachment(gl, attachment, attachment_state);
}


void
GPlatesOpenGL::GLFramebuffer::framebuffer_texture_layer(
		GL &gl,
		GLenum target,
		GLenum attachment,
		boost::optional<GLTexture::shared_ptr_type> texture,
		GLint level,
		GLint layer)
{
	// Either attach the specified texture or detach.
	GLuint texture_resource = 0;
	if (texture)
	{
		texture_resource = texture.get()->get_resource_handle();
	}

	glFramebufferTextureLayer(target, attachment, texture_resource, level, layer);

	// Record the new framebuffer internal state, and the state associated with the current context.
	ObjectState::Attachment attachment_state; // Detached state.
	if (texture)
	{
		// Attached state.
		attachment_state.type = ObjectState::Attachment::FRAMEBUFFER_TEXTURE_LAYER;
		attachment_state.texture = texture;
		attachment_state.level = level;
		attachment_state.layer = layer;
	}

	set_attachment(gl, attachment, attachment_state);
}


GPlatesOpenGL::GLFramebuffer::ContextObjectState &
GPlatesOpenGL::GLFramebuffer::get_object_state_for_current_context(
		GL &gl) const
{
	const GLContext &current_context = gl.get_context();

	context_object_state_seq_type::iterator context_object_state_iter = d_context_object_states.begin();
	context_object_state_seq_type::iterator context_object_state_end = d_context_object_states.end();
	for ( ; context_object_state_iter != context_object_state_end; ++context_object_state_iter)
	{
		if (context_object_state_iter->context == &current_context)
		{
			return *context_object_state_iter;
		}
	}

	// Context not yet encountered so create a new context object state.
	d_context_object_states.push_back(ContextObjectState(current_context, gl));

	return d_context_object_states.back();
}


void
GPlatesOpenGL::GLFramebuffer::set_attachment(
		GL &gl,
		GLenum attachment,
		const ObjectState::Attachment &attachment_state)
{
	// Record the framebuffer internal state.
	//
	// Note that the framebuffer object in current context has been updated,
	// so we'll need to update our record of its state also.
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);

	if (attachment == GL_DEPTH_ATTACHMENT)
	{
		// Set depth as attached, and leave stencil as is.
		d_object_state.depth_attachment = attachment_state;
		current_context_object_state.object_state.depth_attachment = attachment_state;
	}
	else if (attachment == GL_STENCIL_ATTACHMENT)
	{
		// Set stencil as attached, and leave depth as is.
		d_object_state.stencil_attachment = attachment_state;
		current_context_object_state.object_state.stencil_attachment = attachment_state;
	}
	else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
	{
		// Set both depth and stencil as attached.
		d_object_state.depth_attachment = attachment_state;
		d_object_state.stencil_attachment = attachment_state;
		current_context_object_state.object_state.depth_attachment = attachment_state;
		current_context_object_state.object_state.stencil_attachment = attachment_state;
	}
	else // GL_COLOR_ATTACHMENTi ...
	{
		GPlatesGlobal::Assert<OpenGLException>(
				attachment >= GL_COLOR_ATTACHMENT0 &&
					attachment < GL_COLOR_ATTACHMENT0 + gl.get_capabilities().gl_max_color_attachments,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer color attachment exceeds GL_MAX_COLOR_ATTACHMENTS.");

		const GLuint color_attachment_index = attachment - GL_COLOR_ATTACHMENT0;

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				color_attachment_index < d_object_state.color_attachments.size() && 
					color_attachment_index < current_context_object_state.object_state.color_attachments.size(),
				GPLATES_ASSERTION_SOURCE);

		d_object_state.color_attachments[color_attachment_index] = attachment_state;
		current_context_object_state.object_state.color_attachments[color_attachment_index] = attachment_state;
	}

	// Invalidate all contexts except the current one.
	// When we switch to the next context it will be out-of-date and require synchronisation.
	d_object_state_subject.invalidate();
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


GLuint
GPlatesOpenGL::GLFramebuffer::Allocator::allocate(
		const GLCapabilities &capabilities)
{
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	return fbo;
}


void
GPlatesOpenGL::GLFramebuffer::Allocator::deallocate(
		GLuint fbo)
{
	glDeleteFramebuffers(1, &fbo);
}


GPlatesOpenGL::GLFramebuffer::ContextObjectState::ContextObjectState(
		const GLContext &context_,
		GL &gl) :
	context(&context_),
	// Create a framebuffer object resource using the resource manager associated with the context...
	resource(
			resource_type::create(
					context_.get_capabilities(),
					context_.get_non_shared_state()->get_framebuffer_resource_manager())),
	object_state(gl.get_capabilities().gl_max_color_attachments)
{
}
