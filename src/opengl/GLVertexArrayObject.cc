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
#include <QDebug>

#include "GLVertexArrayObject.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "GLState.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLVertexArrayObject::resource_handle_type
GPlatesOpenGL::GLVertexArrayObject::Allocator::allocate()
{
#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	// We should only get here if the vertex array object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_array_object),
			GPLATES_ASSERTION_SOURCE);

	resource_handle_type vertex_array_object;
	glGenVertexArrays(1, &vertex_array_object);
	return vertex_array_object;
#else
	throw OpenGLException(GPLATES_EXCEPTION_SOURCE, "Internal Error: GL_ARB_vertex_array_object not supported");
#endif
}


void
GPlatesOpenGL::GLVertexArrayObject::Allocator::deallocate(
		resource_handle_type vertex_array_object)
{
#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	// We should only get here if the vertex array object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_array_object),
			GPLATES_ASSERTION_SOURCE);

	glDeleteVertexArrays(1, &vertex_array_object);
#else
	throw OpenGLException(GPLATES_EXCEPTION_SOURCE, "Internal Error: GL_ARB_vertex_array_object not supported");
#endif
}


GPlatesOpenGL::GLVertexArrayObject::GLVertexArrayObject(
		GLRenderer &renderer) :
	d_object_state(GLVertexArrayImpl::create(renderer))
{
#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	// We should only get here if the vertex array object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_array_object),
			GPLATES_ASSERTION_SOURCE);
#else
	throw OpenGLException(GPLATES_EXCEPTION_SOURCE, "Internal Error: GL_ARB_vertex_array_object not supported");
#endif
}


void
GPlatesOpenGL::GLVertexArrayObject::gl_bind(
		GLRenderer &renderer) const
{
	// Note that we don't need to save/restore render state and apply the gl_bind immediately
	// since we've been explicitly requested by the client to gl_bind (so we're only changing state
	// that we've been requested to change) and we are not making any *direct* calls to OpenGL
	// (that would require the binding to be applied immediately).
	renderer.gl_bind_vertex_array_object(
			boost::dynamic_pointer_cast<const GLVertexArrayObject>(shared_from_this()));
}


void
GPlatesOpenGL::GLVertexArrayObject::gl_draw_range_elements(
		GLRenderer &renderer,
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		GLint indices_offset) const
{
	d_object_state->gl_draw_range_elements(renderer, mode, start, end, count, type, indices_offset);
}


void
GPlatesOpenGL::GLVertexArrayObject::clear(
		GLRenderer &renderer)
{
	d_object_state->clear(renderer);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_vertex_element_buffer(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_to_const_type &vertex_element_buffer)
{
	d_object_state->set_vertex_element_buffer(renderer, vertex_element_buffer);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_enable_client_state(
		GLRenderer &renderer,
		GLenum array,
		bool enable)
{
	d_object_state->set_enable_client_state(renderer, array, enable);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_enable_client_texture_state(
		GLRenderer &renderer,
		GLenum texture_unit,
		bool enable)
{
	d_object_state->set_enable_client_texture_state(renderer, texture_unit, enable);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_vertex_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	d_object_state->set_vertex_pointer(renderer, vertex_buffer, size, type, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_color_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	d_object_state->set_color_pointer(renderer, vertex_buffer, size, type, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_normal_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	d_object_state->set_normal_pointer(renderer, vertex_buffer, type, stride, offset);
}

void
GPlatesOpenGL::GLVertexArrayObject::set_tex_coord_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLenum texture_unit,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	d_object_state->set_tex_coord_pointer(renderer, vertex_buffer, texture_unit, size, type, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_enable_vertex_attrib_array(
		GLRenderer &renderer,
		GLuint attribute_index,
		bool enable)
{
	d_object_state->set_enable_vertex_attrib_array(renderer, attribute_index, enable);
}


void
GPlatesOpenGL::GLVertexArrayObject::set_vertex_attrib_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLuint attribute_index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		GLint offset)
{
	d_object_state->set_vertex_attrib_pointer(
			renderer, vertex_buffer, attribute_index, size, type, normalized, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayObject::get_vertex_array_resource(
		GLRenderer &renderer,
		GPlatesOpenGL::GLVertexArrayObject::resource_handle_type &resource_handle,
		GLState::shared_ptr_type &current_resource_state,
		GLState::shared_ptr_to_const_type &target_resource_state) const
{
	const GPlatesOpenGL::GLVertexArrayObject::ContextObjectState &current_context_object_state =
			get_object_state_for_current_context(renderer);

	//
	// Return the resource handle and current/target states to the caller.
	//

	// The resource handle.
	resource_handle = current_context_object_state.resource->get_resource_handle();

	// The current state of the resource (as seen by OpenGL).
	current_resource_state = current_context_object_state.resource_state;

	// The state that we want the vertex array to be in.
	target_resource_state = d_object_state->get_compiled_bind_state();
}


const GPlatesOpenGL::GLVertexArrayObject::ContextObjectState &
GPlatesOpenGL::GLVertexArrayObject::get_object_state_for_current_context(
		GLRenderer &renderer) const
{
	const GLContext &current_context = renderer.get_context();

	context_object_state_seq_type::const_iterator context_object_state_iter = d_context_object_states.begin();
	context_object_state_seq_type::const_iterator context_object_state_end = d_context_object_states.end();
	for ( ; context_object_state_iter != context_object_state_end; ++context_object_state_iter)
	{
		if (context_object_state_iter->context == &current_context)
		{
			return *context_object_state_iter;
		}
	}

	// Context not yet encountered so create a new context object state.
	d_context_object_states.push_back(ContextObjectState(current_context, renderer));
	return d_context_object_states.back();
}


GPlatesOpenGL::GLVertexArrayObject::ContextObjectState::ContextObjectState(
		const GLContext &context_,
		GLRenderer &renderer_) :
	context(&context_),
	// Create a vertex array object resource using the resource manager associated with the context...
	resource(resource_type::create(context_.get_non_shared_state()->get_vertex_array_object_resource_manager())),
	// Get the default vertex array state.
	// This is the state that the newly created vertex array resource starts out in...
	resource_state(create_unbound_vertex_array_compiled_draw_state(renderer_)->get_state()->clone())
{
}
