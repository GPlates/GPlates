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
#include "GLMatrix.h"
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
GPlatesOpenGL::GLUtils::assert_no_gl_errors(
		const GPlatesUtils::CallStack::Trace &assert_location)
{
	const GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		const char *gl_error_string = reinterpret_cast<const char *>(gluErrorString(error));

		qWarning() << "OpenGL error: " << gl_error_string;

#ifdef GPLATES_DEBUG
		GPlatesGlobal::Abort(assert_location);
#else
		throw GPlatesOpenGL::OpenGLException(assert_location, gl_error_string);
#endif
	}
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_2D_textured_quad(
		GLRenderer &renderer)
{
	// The vertices for the full-screen quad.
	const GLTexturedVertex quad_vertices[4] =
	{  //  x,  y, z, u, v
		GLTexturedVertex(-1, -1, 0, 0, 0),
		GLTexturedVertex(1, -1, 0, 1, 0),
		GLTexturedVertex(1,  1, 0, 1, 1),
		GLTexturedVertex(-1,  1, 0, 0, 1)
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
			std::vector<GLTexturedVertex>(quad_vertices, quad_vertices + 4),
			std::vector<GLushort>(quad_indices, quad_indices + 4),
			GL_QUADS);
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_2D_coloured_quad(
		GLRenderer &renderer,
		const GPlatesGui::rgba8_t &colour)
{
	// The vertices for the full-screen quad.
	const GLColouredVertex quad_vertices[4] =
	{  //  x,  y, z, colour
		GLColouredVertex(-1, -1, 0, colour),
		GLColouredVertex(1, -1, 0, colour),
		GLColouredVertex(1,  1, 0, colour),
		GLColouredVertex(-1,  1, 0, colour)
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
			std::vector<GLColouredVertex>(quad_vertices, quad_vertices + 4),
			std::vector<GLushort>(quad_indices, quad_indices + 4),
			GL_QUADS);
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_2D_coloured_textured_quad(
		GLRenderer &renderer,
		const GPlatesGui::rgba8_t &colour)
{
	// The vertices for the full-screen quad.
	const GLColouredTexturedVertex quad_vertices[4] =
	{  //  x,  y, z, u, v, colour
		GLColouredTexturedVertex(-1, -1, 0, 0, 0, colour),
		GLColouredTexturedVertex(1, -1, 0, 1, 0, colour),
		GLColouredTexturedVertex(1,  1, 0, 1, 1, colour),
		GLColouredTexturedVertex(-1,  1, 0, 0, 1, colour)
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
			std::vector<GLColouredTexturedVertex>(quad_vertices, quad_vertices + 4),
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


GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::QuadTreeClipSpaceTransform() :
	d_scale(1),
	d_inverse_scale(1),
	d_translate_x(0),
	d_translate_y(0)
{
}


GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::QuadTreeClipSpaceTransform(
			unsigned int x_offset,
			unsigned int y_offset) :
	d_scale(2.0),
	d_inverse_scale(0.5),
	d_translate_x(0.5 - x_offset),
	d_translate_y(0.5 - y_offset)
{
}


GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::QuadTreeClipSpaceTransform(
			const QuadTreeClipSpaceTransform &parent_clip_space_transform,
			unsigned int x_offset,
			unsigned int y_offset)
{
	d_scale = 2.0 * parent_clip_space_transform.d_scale;
	d_inverse_scale = 0.5 * parent_clip_space_transform.d_inverse_scale;

	d_translate_x = parent_clip_space_transform.d_translate_x + (1 - 2 * x_offset) * d_inverse_scale;
	d_translate_y = parent_clip_space_transform.d_translate_y + (1 - 2 * y_offset) * d_inverse_scale;
}


void
GPlatesOpenGL::GLUtils::QuadTreeClipSpaceTransform::transform(
		  GLMatrix &matrix) const
{
	matrix
		.gl_scale(
				d_scale,
				d_scale,
				1.0)
		.gl_translate(
				d_translate_x,
				d_translate_y,
				0.0);
}


GPlatesOpenGL::GLUtils::QuadTreeUVTransform::QuadTreeUVTransform() :
	d_scale(1),
	d_translate_u(0),
	d_translate_v(0)
{
}


GPlatesOpenGL::GLUtils::QuadTreeUVTransform::QuadTreeUVTransform(
			unsigned int u_offset,
			unsigned int v_offset) :
	d_scale(0.5),
	d_translate_u(u_offset * 0.5),
	d_translate_v(v_offset * 0.5)
{
}


GPlatesOpenGL::GLUtils::QuadTreeUVTransform::QuadTreeUVTransform(
			const QuadTreeUVTransform &parent_uv_transform,
			unsigned int u_offset,
			unsigned int v_offset)
{
	d_scale = 0.5 * parent_uv_transform.d_scale;

	d_translate_u = parent_uv_transform.d_translate_u + u_offset * d_scale;
	d_translate_v = parent_uv_transform.d_translate_v + v_offset * d_scale;
}


void
GPlatesOpenGL::GLUtils::QuadTreeUVTransform::transform(
		  GLMatrix &matrix) const
{
	matrix
		.gl_translate(
				d_translate_u,
				d_translate_v,
				0.0)
		.gl_scale(
				d_scale,
				d_scale,
				1.0);
}
