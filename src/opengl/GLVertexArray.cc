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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include "GLVertexArray.h"

#include "GL.h"
#include "GLContext.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::GLVertexArray::GLVertexArray(
		GL &gl) :
	d_object_state(gl.get_capabilities().shader.gl_max_vertex_attribs)
{
}


void
GPlatesOpenGL::GLVertexArray::clear(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind this vertex array first.
	// Note that this is context state (not VAO state) and will get restored on leaving the current scope.
	gl.BindVertexArray(shared_from_this());

	// Unbind element array buffer.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, boost::none);

	// Unbind array buffer.
	// Note that this is context state (not VAO state) and will get restored on leaving the current scope.
	gl.BindBuffer(GL_ARRAY_BUFFER, boost::none);

	// Reset the attribute arrays.
	const unsigned int num_attribute_arrays = gl.get_capabilities().shader.gl_max_vertex_attribs;
	for (GLuint attribute_index = 0; attribute_index < num_attribute_arrays; ++attribute_index)
	{
		gl.DisableVertexAttribArray(attribute_index);
		gl.VertexAttribDivisor(attribute_index, 0);
		gl.VertexAttribPointer(attribute_index, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	}
}


GLuint
GPlatesOpenGL::GLVertexArray::get_resource_handle(
		GL &gl) const
{
	return get_object_state_for_current_context(gl).resource->get_resource_handle();
}


void
GPlatesOpenGL::GLVertexArray::synchronise_current_context(
		GL &gl)
{
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);

	// Return early if the current context state is already up-to-date.
	// This is an optimisation (it's not strictly necessary).
	if (d_object_state_subject.is_observer_up_to_date(current_context_object_state.object_state_observer))
	{
		return;
	}

	// Make sure we leave the OpenGL global state the way it was.
	// This is because we may call 'gl.BindBuffer(GL_ARRAY_BUFFER, ...)' below, and it's part of the
	// global state (unlike binding the GL_ELEMENT_ARRAY_BUFFER target, which is part of vertex array state).
	GL::StateScope save_restore_state(gl);

	//
	// Synchronise the element array buffer binding.
	//
	if (current_context_object_state.object_state.element_array_buffer != d_object_state.element_array_buffer)
	{
		// Get the buffer resource to bind (or 0 to unbind).
		GLuint element_array_buffer_resource = 0;
		if (d_object_state.element_array_buffer)
		{
			element_array_buffer_resource = d_object_state.element_array_buffer.get()->get_resource_handle();
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_array_buffer_resource);

		// Record updated context state.
		current_context_object_state.object_state.element_array_buffer = d_object_state.element_array_buffer;
	}

	//
	// Synchronise the attribute arrays (parameters + array buffer binding).
	//
	const unsigned int num_attribute_arrays = gl.get_capabilities().shader.gl_max_vertex_attribs;
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_object_state.attribute_arrays.size() == num_attribute_arrays &&
				current_context_object_state.object_state.attribute_arrays.size() == num_attribute_arrays,
			GPLATES_ASSERTION_SOURCE);

	for (GLuint attribute_index = 0; attribute_index < num_attribute_arrays; ++attribute_index)
	{
		const ObjectState::AttributeArray &attribute_array = d_object_state.attribute_arrays[attribute_index];
		ObjectState::AttributeArray &context_attribute_array = current_context_object_state.object_state.attribute_arrays[attribute_index];

		if (context_attribute_array.enabled != attribute_array.enabled)
		{
			if (attribute_array.enabled)
			{
				glEnableVertexAttribArray(attribute_index);
			}
			else
			{
				glDisableVertexAttribArray(attribute_index);
			}
		}

		if (context_attribute_array.divisor != attribute_array.divisor)
		{
			glVertexAttribDivisor(attribute_index, attribute_array.divisor);
		}

		if (context_attribute_array.size != attribute_array.size ||
			context_attribute_array.type != attribute_array.type ||
			context_attribute_array.normalized != attribute_array.normalized ||
			context_attribute_array.integer != attribute_array.integer ||
			context_attribute_array.stride != attribute_array.stride ||
			// Pointer is really just an offset from beginning of bound buffer (so we're just comparing offsets)...
			context_attribute_array.pointer != attribute_array.pointer ||
			context_attribute_array.array_buffer != attribute_array.array_buffer)
		{
			// Bind the array buffer.
			//
			// Note that array buffer binding is global state (unlike binding the GL_ELEMENT_ARRAY_BUFFER target,
			// which is part of vertex array state) and hence goes through 'GL' instead of directly to OpenGL
			// like other calls in this function. It's also why the GL::StateScope at top of this function will
			// restore the binding when leaving this function (to what it was before entering).
			gl.BindBuffer(GL_ARRAY_BUFFER, attribute_array.array_buffer);

			if (attribute_array.integer)
			{
				glVertexAttribIPointer(
						attribute_index,
						attribute_array.size,
						attribute_array.type,
						attribute_array.stride,
						attribute_array.pointer);
			}
			else
			{
				glVertexAttribPointer(
						attribute_index,
						attribute_array.size,
						attribute_array.type,
						attribute_array.normalized,
						attribute_array.stride,
						attribute_array.pointer);
			}

			// Record updated context state.
			context_attribute_array = attribute_array;
		}
	}

	// The current context state is now up-to-date.
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


void
GPlatesOpenGL::GLVertexArray::bind_element_array_buffer(
		GL &gl,
		boost::optional<GLBuffer::shared_ptr_type> buffer)
{
	glBindBuffer(
			GL_ELEMENT_ARRAY_BUFFER,
			// The buffer resource to bind (or 0 to unbind)...
			buffer ? buffer.get()->get_resource_handle() : 0);

	// Record the vertex array internal state.
	d_object_state.element_array_buffer = buffer;

	// Vertex array object in current context has been updated.
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);
	current_context_object_state.object_state.element_array_buffer = d_object_state.element_array_buffer;

	// Invalidate all contexts except the current one.
	// When we switch to the next context it will be out-of-date and require synchronisation.
	d_object_state_subject.invalidate();
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


void
GPlatesOpenGL::GLVertexArray::enable_vertex_attrib_array(
		GL &gl,
		GLuint index)
{
	glEnableVertexAttribArray(index);

	// Record the new vertex array internal state, and the state associated with the current context.
	ObjectState::AttributeArray &attribute_array = get_attribute_array(index);
	attribute_array.enabled = GL_TRUE;
	update_attribute_array(gl, index, attribute_array);
}


void
GPlatesOpenGL::GLVertexArray::disable_vertex_attrib_array(
		GL &gl,
		GLuint index)
{
	glDisableVertexAttribArray(index);

	// Record the new vertex array internal state, and the state associated with the current context.
	ObjectState::AttributeArray &attribute_array = get_attribute_array(index);
	attribute_array.enabled = GL_FALSE;
	update_attribute_array(gl, index, attribute_array);
}


void
GPlatesOpenGL::GLVertexArray::vertex_attrib_divisor(
		GL &gl,
		GLuint index,
		GLuint divisor)
{
	glVertexAttribDivisor(index, divisor);

	// Record the new vertex array internal state, and the state associated with the current context.
	ObjectState::AttributeArray &attribute_array = get_attribute_array(index);
	attribute_array.divisor = divisor;
	update_attribute_array(gl, index, attribute_array);
}


void
GPlatesOpenGL::GLVertexArray::vertex_attrib_pointer(
		GL &gl,
		GLuint index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		const GLvoid *pointer,
		boost::optional<GLBuffer::shared_ptr_type> array_buffer)
{
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);

	// Record the new vertex array internal state, and the state associated with the current context.
	ObjectState::AttributeArray &attribute_array = get_attribute_array(index);
	attribute_array.size = size;
	attribute_array.type = type;
	attribute_array.normalized = normalized;
	attribute_array.integer = GL_FALSE;
	attribute_array.stride = stride;
	attribute_array.pointer = pointer;
	// NOTE: 'array_buffer' is the currently bound array buffer - we're just recording that...
	attribute_array.array_buffer = array_buffer;
	update_attribute_array(gl, index, attribute_array);
}


void
GPlatesOpenGL::GLVertexArray::vertex_attrib_i_pointer(
		GL &gl,
		GLuint index,
		GLint size,
		GLenum type,
		GLsizei stride,
		const GLvoid *pointer,
		boost::optional<GLBuffer::shared_ptr_type> array_buffer)
{
	glVertexAttribIPointer(index, size, type, stride, pointer);

	// Record the new vertex array internal state, and the state associated with the current context.
	ObjectState::AttributeArray &attribute_array = get_attribute_array(index);
	attribute_array.size = size;
	attribute_array.type = type;
	attribute_array.normalized = GL_FALSE;
	attribute_array.integer = GL_TRUE;
	attribute_array.stride = stride;
	attribute_array.pointer = pointer;
	// NOTE: 'array_buffer' is the currently bound array buffer - we're just recording that...
	attribute_array.array_buffer = array_buffer;
	update_attribute_array(gl, index, attribute_array);
}


GPlatesOpenGL::GLVertexArray::ContextObjectState &
GPlatesOpenGL::GLVertexArray::get_object_state_for_current_context(
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


GPlatesOpenGL::GLVertexArray::ObjectState::AttributeArray &
GPlatesOpenGL::GLVertexArray::get_attribute_array(
		GLuint index)
{
	GPlatesGlobal::Assert<OpenGLException>(
			index < d_object_state.attribute_arrays.size(),
			GPLATES_ASSERTION_SOURCE,
			"Vertex attribute array index exceeds GL_MAX_VERTEX_ATTRIBS.");

	return d_object_state.attribute_arrays[index];
}


void
GPlatesOpenGL::GLVertexArray::update_attribute_array(
		GL &gl,
		GLuint index,
		const ObjectState::AttributeArray &attribute_array)
{
	GPlatesGlobal::Assert<OpenGLException>(
			index < d_object_state.attribute_arrays.size(),
			GPLATES_ASSERTION_SOURCE,
			"Vertex attribute array index exceeds GL_MAX_VERTEX_ATTRIBS.");

	// Record the vertex array internal state.
	d_object_state.attribute_arrays[index] = attribute_array;

	// Vertex array object in current context has been updated also.
	ContextObjectState &current_context_object_state = get_object_state_for_current_context(gl);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			current_context_object_state.object_state.attribute_arrays.size() ==
				d_object_state.attribute_arrays.size(),
			GPLATES_ASSERTION_SOURCE);
	current_context_object_state.object_state.attribute_arrays[index] = attribute_array;

	// Invalidate all contexts except the current one.
	// When we switch to the next context it will be out-of-date and require synchronisation.
	d_object_state_subject.invalidate();
	d_object_state_subject.update_observer(current_context_object_state.object_state_observer);
}


GLuint
GPlatesOpenGL::GLVertexArray::Allocator::allocate(
		const GLCapabilities &capabilities)
{
	GLuint vertex_array_object;
	glGenVertexArrays(1, &vertex_array_object);
	return vertex_array_object;
}


void
GPlatesOpenGL::GLVertexArray::Allocator::deallocate(
		GLuint vertex_array_object)
{
	glDeleteVertexArrays(1, &vertex_array_object);
}


GPlatesOpenGL::GLVertexArray::ContextObjectState::ContextObjectState(
		const GLContext &context_,
		GL &gl) :
	context(&context_),
	// Create a vertex array object resource using the resource manager associated with the context...
	resource(
			resource_type::create(
					context_.get_capabilities(),
					context_.get_non_shared_state()->get_vertex_array_resource_manager())),
	object_state(gl.get_capabilities().shader.gl_max_vertex_attribs)
{
}
