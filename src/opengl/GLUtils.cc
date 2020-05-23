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

#include <vector>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLUtils.h"

#include "OpenGLException.h"
#include "GLContext.h"
#include "GLRenderer.h"
#include "GLVertex.h"
#include "GLVertexArray.h"

#include "global/GPlatesAssert.h"


namespace GPlatesOpenGL
{
	namespace
	{
		//! Converts an array of 4 numbers representing a tex gen plane into a std::vector.
		std::vector<GLdouble>
		create_object_plane(
				const GLdouble &x,
				const GLdouble &y,
				const GLdouble &z,
				const GLdouble &w)
		{
			std::vector<GLdouble> plane(4);

			plane[0] = x;
			plane[1] = y;
			plane[2] = z;
			plane[3] = w;

			return plane;
		}
	}
}

void
GPlatesOpenGL::GLUtils::check_gl_errors(
		const GPlatesUtils::CallStack::Trace &assert_location)
{
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR)
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

		qWarning() << "OpenGL error: " << error_message;

#ifdef GPLATES_DEBUG
		GPlatesGlobal::Abort(assert_location);
#endif
	}
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_2D_textured_quad(
		GLRenderer &renderer)
{
	return create_full_screen_2D_coloured_textured_quad(
			renderer,
			GPlatesGui::rgba8_t(255, 255, 255, 255/*white*/));
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_2D_coloured_quad(
		GLRenderer &renderer,
		const GPlatesGui::rgba8_t &colour)
{
	// The vertices for the full-screen quad.
	const GLColourVertex quad_vertices[4] =
	{  //  x,  y, z, colour
		GLColourVertex(-1, -1, 0, colour),
		GLColourVertex(1, -1, 0, colour),
		GLColourVertex(1,  1, 0, colour),
		GLColourVertex(-1,  1, 0, colour)
	};

	// The indices for the full-screen quad.
	const GLushort quad_indices[4] = { 0, 1, 2, 3 };

	// Create the vertex array.
	const GLVertexArray::shared_ptr_type vertex_array = GLVertexArray::create(renderer);

	// Store the vertices/indices in the vertex array and compile into a draw call.
	//
	// NOTE: Even though we don't return 'vertex_array' the compiled draw state maintains
	// shared references to the internal vertex buffer, etc - enough to be able to draw the
	// full-screen quad even though 'vertex_array' will no longer exist.
	return compile_vertex_array_draw_state(
			renderer,
			*vertex_array,
			std::vector<GLColourVertex>(quad_vertices, quad_vertices + 4),
			std::vector<GLushort>(quad_indices, quad_indices + 4),
			GL_QUADS);
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_2D_coloured_textured_quad(
		GLRenderer &renderer,
		const GPlatesGui::rgba8_t &colour)
{
	// The vertices for the full-screen quad.
	const GLColourTextureVertex quad_vertices[4] =
	{  //  x,  y, z, u, v, colour
		GLColourTextureVertex(-1, -1, 0, 0, 0, colour),
		GLColourTextureVertex(1, -1, 0, 1, 0, colour),
		GLColourTextureVertex(1,  1, 0, 1, 1, colour),
		GLColourTextureVertex(-1,  1, 0, 0, 1, colour)
	};

	// The indices for the full-screen quad.
	const GLushort quad_indices[4] = { 0, 1, 2, 3 };

	// Create the vertex array.
	const GLVertexArray::shared_ptr_type vertex_array = GLVertexArray::create(renderer);

	// Store the vertices/indices in the vertex array and compile into a draw call.
	//
	// NOTE: Even though we don't return 'vertex_array' the compiled draw state maintains
	// shared references to the internal vertex buffer, etc - enough to be able to draw the
	// full-screen quad even though 'vertex_array' will no longer exist.
	return compile_vertex_array_draw_state(
			renderer,
			*vertex_array,
			std::vector<GLColourTextureVertex>(quad_vertices, quad_vertices + 4),
			std::vector<GLushort>(quad_indices, quad_indices + 4),
			GL_QUADS);
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLUtils::get_clip_space_to_texture_space_transform()
{
	static GLMatrix matrix = GLMatrix().gl_translate(0.5, 0.5, 0.0).gl_scale(0.5, 0.5, 1.0);

	return matrix;
}


void
GPlatesOpenGL::GLUtils::set_full_screen_quad_texture_state(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &texture,
		const unsigned int texture_unit,
		const GLint tex_env_mode,
		const boost::optional<const GLMatrix &> &texture_transform_matrix,
		const GLenum texture_target)
{
	renderer.gl_bind_texture(texture, GL_TEXTURE0 + texture_unit, texture_target);

	renderer.gl_enable_texture(GL_TEXTURE0 + texture_unit, texture_target);
	renderer.gl_tex_env(GL_TEXTURE0 + texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, tex_env_mode);

	if (texture_transform_matrix)
	{
		renderer.gl_load_texture_matrix(GL_TEXTURE0 + texture_unit, texture_transform_matrix.get());
	}
}


void
GPlatesOpenGL::GLUtils::set_frustum_texture_state(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &texture,
		const GLMatrix &projection_transform,
		const GLMatrix &view_transform,
		const unsigned int texture_unit,
		const GLint tex_env_mode,
		const boost::optional<const GLMatrix &> &texture_transform_matrix,
		const GLenum texture_target)
{
	renderer.gl_bind_texture(texture, GL_TEXTURE0 + texture_unit, texture_target);

	renderer.gl_enable_texture(GL_TEXTURE0 + texture_unit, texture_target);
	renderer.gl_tex_env(GL_TEXTURE0 + texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, tex_env_mode);

	// Set up texture coordinate generation from the vertices (x,y,z) and
	// set up a texture matrix to perform the model-view and projection transforms of the frustum.
	// Set it on same texture unit, ie texture unit 'texture_unit'.
	set_object_linear_tex_gen_state(renderer, texture_unit);

	GLMatrix texture_matrix =
			texture_transform_matrix
			? texture_transform_matrix.get()
			: GLMatrix();
	texture_matrix.gl_mult_matrix(projection_transform);
	texture_matrix.gl_mult_matrix(view_transform);

	renderer.gl_load_texture_matrix(GL_TEXTURE0 + texture_unit, texture_matrix);
}


void
GPlatesOpenGL::GLUtils::set_object_linear_tex_gen_state(
		GLRenderer &renderer,
		const unsigned int texture_unit)
{
	// Enable tex gen.
	renderer.gl_enable_texture(GL_TEXTURE0 + texture_unit, GL_TEXTURE_GEN_S);
	renderer.gl_enable_texture(GL_TEXTURE0 + texture_unit, GL_TEXTURE_GEN_T);
	renderer.gl_enable_texture(GL_TEXTURE0 + texture_unit, GL_TEXTURE_GEN_R);
	renderer.gl_enable_texture(GL_TEXTURE0 + texture_unit, GL_TEXTURE_GEN_Q);

	// Set tex gen mode to object linear.
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	static const std::vector<GLdouble> OBJECT_PLANE_S = create_object_plane(1, 0, 0, 0);
	static const std::vector<GLdouble> OBJECT_PLANE_T = create_object_plane(0, 1, 0, 0);
	static const std::vector<GLdouble> OBJECT_PLANE_R = create_object_plane(0, 0, 1, 0);
	static const std::vector<GLdouble> OBJECT_PLANE_Q = create_object_plane(0, 0, 0, 1);

	// Set the object planes.
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_S, GL_OBJECT_PLANE, OBJECT_PLANE_S);
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_T, GL_OBJECT_PLANE, OBJECT_PLANE_T);
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_R, GL_OBJECT_PLANE, OBJECT_PLANE_R);
	renderer.gl_tex_gen(GL_TEXTURE0 + texture_unit, GL_Q, GL_OBJECT_PLANE, OBJECT_PLANE_Q);
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
