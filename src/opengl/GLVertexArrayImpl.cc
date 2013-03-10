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

#include "GLVertexArrayImpl.h"

#include "GLContext.h"
#include "GLRenderer.h"


GPlatesOpenGL::GLVertexArrayImpl::GLVertexArrayImpl(
		GLRenderer &renderer) :
	// Create a compiled draw state that unbinds/disables all vertex attribute arrays, etc.
	//
	// This is necessary so that this vertex array does not inherit state from a previously
	// bound vertex array (eg, if a previous vertex array is bound and then this one is bound).
	//
	// The normal procedure for this sort of thing is for the client to use state blocks but here
	// we need something different since we can't expect the client to wrap the binding of each
	// vertex array into a separate state block because the client should be able to view each bind
	// as an atomic rendering operation (ie, subsequent binds completely override previous binds)...
	d_compiled_bind_state(create_unbound_vertex_array_compiled_draw_state(renderer))
{
}


void
GPlatesOpenGL::GLVertexArrayImpl::gl_bind(
		GLRenderer &renderer) const
{
	// Make sure a vertex element buffer has been set.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_vertex_element_buffer,
			GPLATES_ASSERTION_SOURCE);

	// Note that we don't need to save/restore render state or apply changes immediately.
	// We're only changing state that we've been requested to change and we are not making any
	// *direct* calls to OpenGL (that would require the binding to be applied immediately).
	renderer.apply_compiled_draw_state(*d_compiled_bind_state);
}


void
GPlatesOpenGL::GLVertexArrayImpl::gl_draw_range_elements(
		GLRenderer &renderer,
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		GLint indices_offset) const
{
	// Make sure a vertex element buffer has been set.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_vertex_element_buffer,
			GPLATES_ASSERTION_SOURCE);

	// Get the vertex element buffer to apply the indices offset and then draw.
	d_vertex_element_buffer.get()->gl_draw_range_elements(renderer, mode, start, end, count, type, indices_offset);
}


void
GPlatesOpenGL::GLVertexArrayImpl::clear(
		GLRenderer &renderer)
{
	// Just reset our compiled bind state to the unbound vertex array.
	d_compiled_bind_state = create_unbound_vertex_array_compiled_draw_state(renderer);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_vertex_element_buffer(
		GLRenderer &renderer,
		const GLVertexElementBuffer::shared_ptr_to_const_type &vertex_element_buffer)
{
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_element_buffer->gl_bind(renderer);

	// We'll need a reference later when we're asked to draw something.
	d_vertex_element_buffer = vertex_element_buffer;
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_enable_client_state(
		GLRenderer &renderer,
		GLenum array,
		bool enable)
{
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	renderer.gl_enable_client_state(array, enable);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_enable_client_texture_state(
		GLRenderer &renderer,
		GLenum texture_unit,
		bool enable)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + renderer.get_context().get_capabilities().texture.gl_max_texture_coords,
			GPLATES_ASSERTION_SOURCE);

	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	renderer.gl_enable_client_texture_state(texture_unit, enable);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_vertex_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_buffer->gl_vertex_pointer(renderer, size, type, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_color_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_buffer->gl_color_pointer(renderer, size, type, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_normal_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_buffer->gl_normal_pointer(renderer, type, stride, offset);
}

void
GPlatesOpenGL::GLVertexArrayImpl::set_tex_coord_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLenum texture_unit,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			texture_unit >= GL_TEXTURE0 &&
					texture_unit < GL_TEXTURE0 + renderer.get_context().get_capabilities().texture.gl_max_texture_coords,
			GPLATES_ASSERTION_SOURCE);

	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_buffer->gl_tex_coord_pointer(renderer, texture_unit, size, type, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_enable_vertex_attrib_array(
		GLRenderer &renderer,
		GLuint attribute_index,
		bool enable)
{
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	renderer.gl_enable_vertex_attrib_array(attribute_index, enable);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_vertex_attrib_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLuint attribute_index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		GLint offset)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			attribute_index < renderer.get_context().get_capabilities().shader.gl_max_vertex_attribs,
			GPLATES_ASSERTION_SOURCE);

	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_buffer->gl_vertex_attrib_pointer(renderer, attribute_index, size, type, normalized, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_vertex_attrib_i_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLuint attribute_index,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			attribute_index < renderer.get_context().get_capabilities().shader.gl_max_vertex_attribs,
			GPLATES_ASSERTION_SOURCE);

	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_buffer->gl_vertex_attrib_i_pointer(renderer, attribute_index, size, type, stride, offset);
}


void
GPlatesOpenGL::GLVertexArrayImpl::set_vertex_attrib_l_pointer(
		GLRenderer &renderer,
		const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
		GLuint attribute_index,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLint offset)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			attribute_index < renderer.get_context().get_capabilities().shader.gl_max_vertex_attribs,
			GPLATES_ASSERTION_SOURCE);

	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_compiled_bind_state);

	vertex_buffer->gl_vertex_attrib_l_pointer(renderer, attribute_index, size, type, stride, offset);
}
