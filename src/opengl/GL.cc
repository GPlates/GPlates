/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include <opengl/OpenGL3.h>
#include <exception>
#include <QtGlobal>

#include "GL.h"

#include "OpenGLException.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GL::GL(
		const GLContext::non_null_ptr_type &context,
		const GLStateStore::shared_ptr_type &state_store) :
	d_context(context),
	d_state_store(state_store),
	d_default_state(d_state_store->allocate_state()),
	d_current_state(d_state_store->allocate_state())
{
}


void
GPlatesOpenGL::GL::ActiveTexture(
		GLenum active_texture_)
{
	d_current_state->active_texture(active_texture_);
}


void
GPlatesOpenGL::GL::BindBuffer(
		GLenum target,
		boost::optional<GLBuffer::shared_ptr_type> buffer)
{
	// Re-route binding of element array buffers to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).
	//
	// The element array buffer binding is not really global state in the OpenGL core profile.
	// The binding is stored in a vertex array object. And it's invalid to bind an element array buffer
	// when no vertex array object is bound. Which means it's not really global state in the core profile.
	if (target == GL_ELEMENT_ARRAY_BUFFER)
	{
		// Get the currently bound vertex array object.
		boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

		// Can only bind a vertex element buffer when a vertex array object is currently bound.
		GPlatesGlobal::Assert<OpenGLException>(
				vertex_array,
				GPLATES_ASSERTION_SOURCE,
				"Cannot bind GL_ELEMENT_ARRAY_BUFFER because a vertex array object is not currently bound.");

		vertex_array.get()->bind_element_array_buffer(*this, buffer);
	}
	else
	{
		d_current_state->bind_buffer(target, buffer);
	}
}


void
GPlatesOpenGL::GL::BindTexture(
		GLenum texture_target,
		boost::optional<GLTexture::shared_ptr_type> texture_object)
{
	d_current_state->bind_texture(
			texture_target,
			// Bind to the currently active texture unit (glActiveTexture)...
			d_current_state->get_active_texture()/*texture_unit*/,
			texture_object);
}


void
GPlatesOpenGL::GL::BindVertexArray(
		boost::optional<GLVertexArray::shared_ptr_type> vertex_array)
{
	if (vertex_array)
	{
		// Bind.
		d_current_state->bind_vertex_array(
				vertex_array.get(),
				// Array resource handle associated with the current OpenGL context...
				vertex_array.get()->get_resource_handle(*this));

		// Ensure the vertex array's internal state is reflected in the current context.
		// Each 'GLVertexArray' instance has one native vertex array object per OpenGL context.
		//
		// NOTE: This must be done after binding.
		vertex_array.get()->synchronise_current_context(*this);
	}
	else
	{
		// Unbind.
		d_current_state->bind_vertex_array(boost::none, 0);
	}
}


void
GPlatesOpenGL::GL::DisableVertexAttribArray(
		GLuint index)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only disable an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot disable vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->disable_vertex_attrib_array(*this, index);
}


void
GPlatesOpenGL::GL::EnableVertexAttribArray(
		GLuint index)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only enable an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot enable vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->enable_vertex_attrib_array(*this, index);
}


void
GPlatesOpenGL::GL::VertexAttribDivisor(
		GLuint index,
		GLuint divisor)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only disable an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot set vertex attribute divisor because a vertex array object is not currently bound.");

	vertex_array.get()->vertex_attrib_divisor(*this, index, divisor);
}


void
GPlatesOpenGL::GL::VertexAttribIPointer(
		GLuint index,
		GLint size,
		GLenum type,
		GLsizei stride,
		const GLvoid *pointer)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).
	//
	// The currently bound array buffer, along with the attribute array parameters, are stored in the
	// currently bound vertex array object. And it's invalid to specify an attribute array when no
	// vertex array object is bound.

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only specify an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot specify vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->vertex_attrib_i_pointer(
			*this,
			index, size, type, stride, pointer,
			// The currently bound array buffer...
			d_current_state->get_bind_buffer(GL_ARRAY_BUFFER));
}


void
GPlatesOpenGL::GL::VertexAttribPointer(
		GLuint index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		const GLvoid *pointer)
{
	// Re-route attribute arrays to the currently bound vertex array object
	// (which needs to track its internal state across contexts since cannot be shared across contexts).
	//
	// The currently bound array buffer, along with the attribute array parameters, are stored in the
	// currently bound vertex array object. And it's invalid to specify an attribute array when no
	// vertex array object is bound.

	// Get the currently bound vertex array object.
	boost::optional<GLVertexArray::shared_ptr_type> vertex_array = d_current_state->get_bind_vertex_array();

	// Can only specify an attribute array when a vertex array object is currently bound.
	GPlatesGlobal::Assert<OpenGLException>(
			vertex_array,
			GPLATES_ASSERTION_SOURCE,
			"Cannot specify vertex attribute array because a vertex array object is not currently bound.");

	vertex_array.get()->vertex_attrib_pointer(
			*this,
			index, size, type, normalized, stride, pointer,
			// The currently bound array buffer...
			d_current_state->get_bind_buffer(GL_ARRAY_BUFFER));
}


GPlatesOpenGL::GL::StateScope::StateScope(
		GL &gl,
		bool reset_to_default_state) :
	d_gl(gl),
	d_entry_state(gl.d_current_state->clone()),
	d_have_exited_scope(false)
{
}


GPlatesOpenGL::GL::StateScope::~StateScope()
{
	// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
	// But we log the exception and the location it was emitted.
	try
	{
		end_scope();
	}
	catch (std::exception &exc)
	{
		qWarning() << "GL: exception thrown during state block scope: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "GL: exception thrown during state block scope: Unknown error";
	}
}


void
GPlatesOpenGL::GL::StateScope::end_scope()
{
	if (!d_have_exited_scope)
	{
		// Restore the global state to what it was on scope entry.
		// On returning 'd_gl.d_current_state' will be equivalent to 'd_entry_state'.
		d_entry_state->apply_state(*d_gl.d_current_state);

		d_have_exited_scope = true;
	}
}
