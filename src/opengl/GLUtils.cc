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

#include <sstream>
#include <vector>
#include <QDebug>
#include <QString>

#include "GLUtils.h"

#include "GLBuffer.h"
#include "GLContext.h"
#include "GLVertexArray.h"
#include "GLVertexUtils.h"
#include "OpenGLException.h"

#include "global/GPlatesAssert.h"

#include "utils/CallStackTracker.h"


void
GPlatesOpenGL::GLUtils::check_gl_errors(
		GL &gl,
		const GPlatesUtils::CallStack::Trace &assert_location)
{
	GLenum error;
	while ((error = gl.GetError()) != GL_NO_ERROR)
	{
		const char *error_message = NULL;

		// These are the error codes returned for all OpenGL versions up to, and including, 4.5.
		switch (error)
		{
		case GL_INVALID_ENUM:
			error_message = "Enumerated argument not legal - offending command ignored.";
			break;
		case GL_INVALID_VALUE:
			error_message = "Numeric argument out of range - offending command ignored.";
			break;
		case GL_INVALID_OPERATION:
			error_message = "Operation not allowed in the current state - offending command ignored.";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			error_message = "Framebuffer object not complete - offending command ignored.";
			break;
		case GL_OUT_OF_MEMORY:
			error_message = "Not enough memory to execute command - state of GL is undefined.";
			break;
		case GL_STACK_UNDERFLOW:
			error_message = "Internal stack underflow.";
			break;
		case GL_STACK_OVERFLOW:
			error_message = "Internal stack overflow.";
			break;

		default:
			// Unexpected error code - we'll just ignore and continue to next error code (if any).
			error_message = "Unknown.";
			break;
		}

		// Push the location of the caller onto the call stack writing out the call stack trace.
		GPlatesUtils::CallStackTracker call_stack_tracker(assert_location);

		// Get the call stack trace as a string.
		std::ostringstream call_stack_string_stream;
		GPlatesUtils::CallStack::instance().write_call_stack_trace(call_stack_string_stream);

		// Output the warning message.
		qWarning() << "OpenGL error: " << error_message;
		// Also write out the call-stack - it's useful to know where this error happened.
		qWarning("    %s", QString::fromStdString(call_stack_string_stream.str()).toLatin1().data());

#ifdef GPLATES_DEBUG
		GPlatesGlobal::Abort(assert_location);
#endif
	}
}


GPlatesOpenGL::GLVertexArray::shared_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_quad(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind vertex array object.
	GLVertexArray::shared_ptr_type vertex_array = GLVertexArray::create(gl);
	gl.BindVertexArray(vertex_array);

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	GLBuffer::shared_ptr_type vertex_buffer = GLBuffer::create(gl);
	gl.BindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	// The vertices for the full-screen quad (as a tri-strip).
	const GLVertexUtils::Vertex quad_vertices[4] =
	{  //  x,  y, z
		GLVertexUtils::Vertex(-1, -1, 0),
		GLVertexUtils::Vertex(1, -1, 0),
		GLVertexUtils::Vertex(-1,  1, 0),
		GLVertexUtils::Vertex(1,  1, 0)
	};

	// Transfer vertex data to currently bound vertex buffer object.
	gl.BufferData(
			GL_ARRAY_BUFFER,
			4 * sizeof(GLVertexUtils::Vertex),
			quad_vertices,
			GL_STATIC_DRAW);

	// Specify vertex attributes (position) in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertexUtils::Vertex), BUFFER_OFFSET(GLVertexUtils::Vertex, x));

	return vertex_array;
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLUtils::get_clip_space_to_texture_space_transform()
{
	static GLMatrix matrix = GLMatrix().gl_translate(0.5, 0.5, 0.0).gl_scale(0.5, 0.5, 1.0);

	return matrix;
}


GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::QuadTreeClipSpaceTransform(
		const double &expand_tile_ratio) :
	d_expand_tile_ratio(expand_tile_ratio),
	d_inverse_expand_tile_ratio(1.0 / expand_tile_ratio),
	d_scale(1),
	d_inverse_scale(1),
	d_relative_x_node_offset(0),
	d_relative_y_node_offset(0)
{
}


GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::QuadTreeClipSpaceTransform(
			const QuadTreeClipSpaceTransform &parent_clip_space_transform,
			unsigned int x_offset,
			unsigned int y_offset) :
	d_expand_tile_ratio(parent_clip_space_transform.d_expand_tile_ratio),
	d_inverse_expand_tile_ratio(parent_clip_space_transform.d_inverse_expand_tile_ratio),
	d_scale(2 * parent_clip_space_transform.d_scale),
	d_inverse_scale(0.5 * parent_clip_space_transform.d_inverse_scale),
	d_relative_x_node_offset((parent_clip_space_transform.d_relative_x_node_offset << 1) + x_offset),
	d_relative_y_node_offset((parent_clip_space_transform.d_relative_y_node_offset << 1) + y_offset)
{
}


void
GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::transform(
		GLMatrix &matrix) const
{
	const double &scale = get_scale();

	const double translate_x = get_translate_x();
	const double translate_y = get_translate_y();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_x, translate_y, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}


void
GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::loose_transform(
		GLMatrix &matrix) const
{
	const double &scale = get_loose_scale();

	const double translate_x = get_loose_translate_x();
	const double translate_y = get_loose_translate_y();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_x, translate_y, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}


void
GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::inverse_transform(
		GLMatrix &matrix) const
{
	const double &scale = get_inverse_scale();

	const double translate_x = get_inverse_translate_x();
	const double translate_y = get_inverse_translate_y();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_x, translate_y, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}


void
GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::inverse_loose_transform(
		GLMatrix &matrix) const
{
	const double &scale = get_inverse_loose_scale();

	const double translate_x = get_inverse_loose_translate_x();
	const double translate_y = get_inverse_loose_translate_y();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_x, translate_y, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}


GPlatesOpenGL::GLUtils::QuadTreeUVTransform::QuadTreeUVTransform(
		const double &expand_tile_ratio) :
	d_clip_space_transform(expand_tile_ratio)
{
}


GPlatesOpenGL::GLUtils::QuadTreeUVTransform::QuadTreeUVTransform(
			const QuadTreeUVTransform &parent_uv_transform,
			unsigned int u_offset,
			unsigned int v_offset) :
	d_clip_space_transform(parent_uv_transform.d_clip_space_transform, u_offset, v_offset)
{
}


void
GPlatesOpenGL::GLUtils::QuadTreeUVTransform::transform(
		GLMatrix &matrix) const
{
	const double &scale = get_scale();

	const double translate_u = get_translate_u();
	const double translate_v = get_translate_v();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_u, translate_v, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}


void
GPlatesOpenGL::GLUtils::QuadTreeUVTransform::loose_transform(
		GLMatrix &matrix) const
{
	const double &scale = get_loose_scale();

	const double translate_u = get_loose_translate_u();
	const double translate_v = get_loose_translate_v();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_u, translate_v, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}


void
GPlatesOpenGL::GLUtils::QuadTreeUVTransform::inverse_transform(
		GLMatrix &matrix) const
{
	const double &scale = get_inverse_scale();

	const double translate_u = get_inverse_translate_u();
	const double translate_v = get_inverse_translate_v();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_u, translate_v, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}


void
GPlatesOpenGL::GLUtils::QuadTreeUVTransform::inverse_loose_transform(
		GLMatrix &matrix) const
{
	const double &scale = get_inverse_loose_scale();

	const double translate_u = get_inverse_loose_translate_u();
	const double translate_v = get_inverse_loose_translate_v();

	// Matrix in column-major format (first column, then second, etc).
	const GLdouble post_multiply_matrix[16] =
	{
		scale, 0, 0, 0,                    // 1st column
		0, scale, 0, 0,                    // 2nd column
		0, 0, 1, 0,                        // 3rd column
		translate_u, translate_v, 0, 1     // 4th column
	};

	matrix.gl_mult_matrix(post_multiply_matrix);
}
