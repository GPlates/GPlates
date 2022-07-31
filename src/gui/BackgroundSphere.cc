/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2010 The University of Sydney, Australia
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

#include <cmath>
#include <vector>
#include <utility>
#include <boost/utility/in_place_factory.hpp>
#include <QString>
#include <QtGlobal>

#include "BackgroundSphere.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLShader.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLViewProjection.h"

#include "presentation/ViewState.h"

#include "utils/CallStackTracker.h"


namespace
{
	//
	// Shader source code to render background sphere in the 3D globe views (perspective and orthographic).
	//

	const char *VERTEX_SHADER_SOURCE =
		R"(
			layout (location = 0) in vec4 position;

			void main()
			{
				gl_Position = position;
			}
		)";

	const QString FRAGMENT_SHADER_SOURCE_FILE_NAME = ":/opengl/globe/render_background_sphere_fragment_shader.glsl";


	void
	compile_link_program(
			GPlatesOpenGL::GL &gl,
			GPlatesOpenGL::GLProgram::shared_ptr_type program)
	{
		// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
		TRACK_CALL_STACK();

		// Vertex shader source.
		GPlatesOpenGL::GLShaderSource vertex_shader_source;
		vertex_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
		vertex_shader_source.add_code_segment(VERTEX_SHADER_SOURCE);

		// Vertex shader.
		GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
		vertex_shader->shader_source(gl, vertex_shader_source);
		vertex_shader->compile_shader(gl);

		// Fragment shader source.
		GPlatesOpenGL::GLShaderSource fragment_shader_source;
		fragment_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
		fragment_shader_source.add_code_segment_from_file(FRAGMENT_SHADER_SOURCE_FILE_NAME);

		// Fragment shader.
		GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
		fragment_shader->shader_source(gl, fragment_shader_source);
		fragment_shader->compile_shader(gl);

		// Vertex-fragment program.
		program->attach_shader(gl, vertex_shader);
		program->attach_shader(gl, fragment_shader);
		program->link_program(gl);
	}
}


GPlatesGui::BackgroundSphere::BackgroundSphere(
		GPlatesOpenGL::GL &gl,
		const GPlatesPresentation::ViewState &view_state) :
	d_view_state(view_state),
	d_background_colour(d_view_state.get_background_colour()),
	d_program(GPlatesOpenGL::GLProgram::create(gl)),
	d_full_screen_quad(gl.get_context().get_shared_state()->get_full_screen_quad(gl))
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	compile_link_program(gl, d_program);

	// Set the background colour in the program object.
	gl.UseProgram(d_program);
	gl.Uniform4f(
			d_program->get_uniform_location(gl, "background_color"),
			d_background_colour.red(), d_background_colour.green(), d_background_colour.blue(), d_background_colour.alpha());
}


void
GPlatesGui::BackgroundSphere::paint(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		bool depth_writes_enabled)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// Bind the shader program.
	gl.UseProgram(d_program);

	// Set view projection matrices in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	GLfloat view_inverse_float_matrix[16];
	GLfloat projection_inverse_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	if (!view_projection.get_inverse_view_transform() ||
		!view_projection.get_inverse_projection_transform())
	{
		// Failed to invert view or projection transform.
		// So log warning (only once) and don't render background sphere.
		// This shouldn't happen with typical view/projection matrices though.
		static bool warned = false;
		if (!warned)
		{
			qWarning() << "View or projection transform not invertible. So not rendering background sphere.";
			warned = true;
		}
		return;
	}
	view_projection.get_inverse_view_transform()->get_float_matrix(view_inverse_float_matrix);
	view_projection.get_inverse_projection_transform()->get_float_matrix(projection_inverse_float_matrix);
	gl.UniformMatrix4fv(
			d_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_program->get_uniform_location(gl, "view_inverse"),
			1, GL_FALSE/*transpose*/, view_inverse_float_matrix);
	gl.UniformMatrix4fv(
			d_program->get_uniform_location(gl, "projection_inverse"),
			1, GL_FALSE/*transpose*/, projection_inverse_float_matrix);

	// Set the viewport (so shader can convert 'gl_FragCoord' to normalised device coordinates (NDC).
	const GPlatesOpenGL::GLViewport &viewport = gl.get_viewport();
	gl.Uniform4f(
			d_program->get_uniform_location(gl, "viewport"),
			viewport.x(), viewport.y(), viewport.width(), viewport.height());

	// If depth writes have been enabled then the shader program needs to output z-buffer depth.
	gl.Uniform1i(
			d_program->get_uniform_location(gl, "write_depth"),
			depth_writes_enabled);

	// Check whether the view state's background colour has changed.
	if (d_view_state.get_background_colour() != d_background_colour)
	{
		d_background_colour = d_view_state.get_background_colour();

		// Change the background colour in the program object.
		gl.Uniform4f(
				d_program->get_uniform_location(gl, "background_color"),
				d_background_colour.red(), d_background_colour.green(), d_background_colour.blue(), d_background_colour.alpha());
	}

	// If the background colour is transparent then set up alpha blending.
	if (d_background_colour.alpha() < 1)
	{
		//
		// For alpha-blending we want:
		//
		//   RGB = A_src * RGB_src + (1-A_src) * RGB_dst
		//     A =     1 *   A_src + (1-A_src) *   A_dst
		//
		// ...so we need to use separate (src,dst) blend factors for the RGB and alpha channels...
		//
		//   RGB uses (A_src, 1 - A_src)
		//     A uses (    1, 1 - A_src)
		//
		// ...this enables the destination to be a texture that is subsequently blended into the final scene.
		// In this case the destination alpha must be correct in order to properly blend the texture into the final scene.
		// However if we're rendering directly into the scene (ie, no render-to-texture) then destination alpha is not
		// actually used (since only RGB in the final scene is visible) and therefore could use same blend factors as RGB.
		//
		gl.Enable(GL_BLEND);
		gl.BlendFuncSeparate(
				GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,  // RGB
				GL_ONE, GL_ONE_MINUS_SRC_ALPHA);       // Alpha
	}

	// Draw the full screen quad.
	gl.BindVertexArray(d_full_screen_quad);
	gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
