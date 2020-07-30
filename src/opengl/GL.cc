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
GPlatesOpenGL::GL::BindVertexArray(
		GPlatesOpenGL::GLVertexArray::shared_ptr_to_const_type array)
{
	d_current_state->bind_vertex_array(array);
}


void
GPlatesOpenGL::GL::BindVertexArray()
{
	d_current_state->bind_vertex_array();
}


void
GPlatesOpenGL::GL::BindBuffer(
		GLenum target,
		GPlatesOpenGL::GLBuffer::shared_ptr_to_const_type buffer)
{
	d_current_state->bind_buffer(target, buffer);
}

void
GPlatesOpenGL::GL::BindBuffer(
		GLenum target)
{
	d_current_state->bind_buffer(target);
}


void
GPlatesOpenGL::GL::EnableVertexAttribArray(
		GLuint index)
{
}


void
GPlatesOpenGL::GL::DisableVertexAttribArray(
		GLuint index)
{
}



void
GPlatesOpenGL::GL::VertexAttribPointer(
		GLuint index,
		GLint size,
		GLenum type,
		GLboolean normalized,
		GLsizei stride,
		GLvoid *pointer)
{
}


void
GPlatesOpenGL::GL::VertexAttribIPointer(
		GLuint index,
		GLint size,
		GLenum type,
		GLsizei stride,
		GLvoid *pointer)
{
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
		d_entry_state->apply_state(d_gl.get_capabilities(), *d_gl.d_current_state);

		d_have_exited_scope = true;
	}
}
