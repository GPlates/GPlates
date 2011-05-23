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

#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLUtils.h"

#include "OpenGLException.h"
#include "GLBindTextureState.h"
#include "GLCompositeStateSet.h"
#include "GLContext.h"
#include "GLMatrix.h"
#include "GLTextureEnvironmentState.h"
#include "GLTextureTransformState.h"
#include "GLVertex.h"
#include "GLVertexArray.h"
#include "GLVertexElementArray.h"

#include "global/GPlatesAssert.h"


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


GPlatesOpenGL::GLVertexArrayDrawable::non_null_ptr_type
GPlatesOpenGL::GLUtils::create_full_screen_2D_textured_quad()
{
	// Initialise the vertex array for the full-screen quad.
	static const GLTexturedVertex quad_vertices[4] =
	{  //  x,  y, z, u, v
		GLTexturedVertex(-1, -1, 0, 0, 0),
		GLTexturedVertex(1, -1, 0, 1, 0),
		GLTexturedVertex(1,  1, 0, 1, 1),
		GLTexturedVertex(-1,  1, 0, 0, 1)
	};

	// Create a regular vertex array that doesn't use vertex buffer objects because
	// there's only four vertices.
	GLVertexArray::shared_ptr_type full_screen_quad_vertex_array =
			GLVertexArray::create(quad_vertices, 4);

	// Initialise the vertex element array for the full-screen quad.
	GLVertexElementArray::shared_ptr_type full_screen_quad_vertex_element_array =	
			GLVertexElementArray::create();
	static const GLushort quad_indices[4] = { 0, 1, 2, 3 };
	full_screen_quad_vertex_element_array->set_array_data(quad_indices, 4);
	full_screen_quad_vertex_element_array->gl_draw_range_elements_EXT(
			GL_QUADS, 0/*start*/, 3/*end*/, 4/*count*/, 0 /*indices_offset*/);

	// The full-screen quad drawable.
	return GLVertexArrayDrawable::create(
			full_screen_quad_vertex_array,
			full_screen_quad_vertex_element_array);
}


const GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLUtils::get_clip_space_to_texture_space_transform()
{
	static GLMatrix matrix = GLMatrix().gl_translate(0.5, 0.5, 0.0).gl_scale(0.5, 0.5, 1.0);

	return matrix;
}


void
GPlatesOpenGL::GLUtils::set_full_screen_quad_texture_state(
		GLCompositeStateSet &state_set,
		const GLTexture::shared_ptr_to_const_type &texture,
		const unsigned int texture_unit,
		const GLint tex_env_mode,
		const boost::optional<const GLMatrix &> &texture_transform_matrix)
{
	// Initialise state set that binds the texture to texture unit 'texture_unit'.
	GLBindTextureState::non_null_ptr_type bind_texture = GLBindTextureState::create();
	bind_texture->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + texture_unit);
	bind_texture->gl_bind_texture(GL_TEXTURE_2D, texture);

	state_set.add_state_set(bind_texture);

	// Set the texture environment state on texture unit 'texture_unit'.
	GLTextureEnvironmentState::non_null_ptr_type tex_env_state = GLTextureEnvironmentState::create();
	tex_env_state->gl_active_texture_ARB(
			GLContext::TextureParameters::gl_texture0_ARB + texture_unit);
	tex_env_state->gl_enable_texture_2D(GL_TRUE);
	tex_env_state->gl_tex_env_mode(tex_env_mode);

	state_set.add_state_set(tex_env_state);

	if (texture_transform_matrix)
	{
		// Set texture transform state on texture unit 'texture_unit'.
		GLTextureTransformState::non_null_ptr_type texture_transform_state =
				GLTextureTransformState::create();
		texture_transform_state->gl_active_texture_ARB(
				GLContext::TextureParameters::gl_texture0_ARB + texture_unit);

		texture_transform_state->gl_load_matrix(texture_transform_matrix.get());

		state_set.add_state_set(texture_transform_state);
	}
}


void
GPlatesOpenGL::GLUtils::set_frustum_texture_state(
		GLCompositeStateSet &state_set,
		const GLTexture::shared_ptr_to_const_type &texture,
		const GLMatrix &projection_transform,
		const GLMatrix &view_transform,
		const unsigned int texture_unit,
		const GLint tex_env_mode,
		const boost::optional<const GLMatrix &> &texture_transform_matrix)
{
	// Initialise state set that binds the texture to texture unit 'texture_unit'.
	GLBindTextureState::non_null_ptr_type bind_texture = GLBindTextureState::create();
	bind_texture->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + texture_unit);
	bind_texture->gl_bind_texture(GL_TEXTURE_2D, texture);

	state_set.add_state_set(bind_texture);

	// Set the texture environment state on texture unit 'texture_unit'.
	GLTextureEnvironmentState::non_null_ptr_type tex_env_state = GLTextureEnvironmentState::create();
	tex_env_state->gl_active_texture_ARB(
			GLContext::TextureParameters::gl_texture0_ARB + texture_unit);
	tex_env_state->gl_enable_texture_2D(GL_TRUE);
	tex_env_state->gl_tex_env_mode(tex_env_mode);

	state_set.add_state_set(tex_env_state);

	// Set up texture coordinate generation from the vertices (x,y,z) and
	// set up a texture matrix to perform the model-view and projection transforms of the frustum.
	// Set it on same texture unit, ie texture unit 'texture_unit'.
	GLTextureTransformState::non_null_ptr_type texture_transform_state =
			GLTextureTransformState::create();
	texture_transform_state->gl_active_texture_ARB(
			GLContext::TextureParameters::gl_texture0_ARB + texture_unit);

	set_object_linear_tex_gen_state(*texture_transform_state);

	GLMatrix texture_matrix =
			texture_transform_matrix
			? texture_transform_matrix.get()
			: GLMatrix();
	texture_matrix.gl_mult_matrix(projection_transform);
	texture_matrix.gl_mult_matrix(view_transform);
	texture_transform_state->gl_load_matrix(texture_matrix);

	state_set.add_state_set(texture_transform_state);
}


void
GPlatesOpenGL::GLUtils::set_object_linear_tex_gen_state(
		GLTextureTransformState &texture_transform_state)
{
	GLTextureTransformState::TexGenCoordState tex_gen_state_s;
	GLTextureTransformState::TexGenCoordState tex_gen_state_t;
	GLTextureTransformState::TexGenCoordState tex_gen_state_r;
	GLTextureTransformState::TexGenCoordState tex_gen_state_q;

	tex_gen_state_s.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_t.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_r.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_q.gl_enable_texture_gen(GL_TRUE);

	tex_gen_state_s.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_t.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_r.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_q.gl_tex_gen_mode(GL_OBJECT_LINEAR);

	const GLdouble object_plane[4][4] =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	tex_gen_state_s.gl_object_plane(object_plane[0]);
	tex_gen_state_t.gl_object_plane(object_plane[1]);
	tex_gen_state_r.gl_object_plane(object_plane[2]);
	tex_gen_state_q.gl_object_plane(object_plane[3]);

	texture_transform_state.set_tex_gen_coord_state(GL_S, tex_gen_state_s);
	texture_transform_state.set_tex_gen_coord_state(GL_T, tex_gen_state_t);
	texture_transform_state.set_tex_gen_coord_state(GL_R, tex_gen_state_r);
	texture_transform_state.set_tex_gen_coord_state(GL_Q, tex_gen_state_q);
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
