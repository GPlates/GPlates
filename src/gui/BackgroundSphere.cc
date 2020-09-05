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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <cmath>
#include <vector>
#include <utility>
#include <boost/utility/in_place_factory.hpp>
#include <QString>

#include "BackgroundSphere.h"

#include "FeedbackOpenGLToQPainter.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "opengl/GL.h"
#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLShader.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLVertexArray.h"

#include "presentation/ViewState.h"


namespace
{
	// Vertex and fragment shader source code to render background sphere in the 3D globe views (perspective and orthographic).
	const QString VERTEX_SHADER = ":/opengl/globe/render_background_sphere_vertex_shader.glsl";
	const QString FRAGMENT_SHADER = ":/opengl/globe/render_background_sphere_fragment_shader.glsl";


	void
	compile_link_program(
			GPlatesOpenGL::GL &gl,
			GPlatesOpenGL::GLProgram::shared_ptr_type program)
	{
		// Vertex shader source.
		GPlatesOpenGL::GLShaderSource vertex_shader_source;
		vertex_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
		vertex_shader_source.add_code_segment_from_file(VERTEX_SHADER);

		// Vertex shader.
		GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
		vertex_shader->shader_source(vertex_shader_source);
		vertex_shader->compile_shader();

		// Fragment shader source.
		GPlatesOpenGL::GLShaderSource fragment_shader_source;
		fragment_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
		fragment_shader_source.add_code_segment_from_file(FRAGMENT_SHADER);

		// Fragment shader.
		GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
		fragment_shader->shader_source(fragment_shader_source);
		fragment_shader->compile_shader();

		// Vertex-fragment program.
		program->attach_shader(vertex_shader);
		program->attach_shader(fragment_shader);
		program->link_program();
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
	compile_link_program(gl, d_program);

	// Set the background colour in the program object.
	d_program->uniform("background_color", d_background_colour);
}


void
GPlatesGui::BackgroundSphere::paint(
		GPlatesOpenGL::GL &gl,
		bool depth_writes_enabled)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// Bind the shader program.
	gl.UseProgram(d_program);

	// Set the viewport (so shader can convert 'gl_FragCoord' to normalised device coordinates (NDC).
	const GPlatesOpenGL::GLViewport &viewport = gl.get_viewport();
	glUniform4f(d_program->get_uniform_location("viewport"),
			viewport.x(), viewport.y(), viewport.width(), viewport.height());

	// If depth writes have been enabled then the shader program needs to output z-buffer depth.
	glUniform1i(d_program->get_uniform_location("write_depth"), depth_writes_enabled);

	// Check whether the view state's background colour has changed.
	if (d_view_state.get_background_colour() != d_background_colour)
	{
		d_background_colour = d_view_state.get_background_colour();

		// Change the background colour in the program object.
		d_program->uniform("background_color", d_background_colour);
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

	// Temporarily disable OpenGL feedback (eg, to SVG) until it's re-implemented using OpenGL 3...
#if 1
	// Draw the full screen quad.
	gl.BindVertexArray(d_full_screen_quad);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
	// Either render directly to the framebuffer, or render to a QImage and draw that to the
	// feedback paint device using a QPainter.
	//
	// NOTE: For feedback to a QPainter we render to an image instead of rendering a full-screen quad geometry.
	if (gl.rendering_to_context_framebuffer())
	{
		gl.apply_compiled_draw_state(*d_full_screen_quad);
	}
	else
	{
		FeedbackOpenGLToQPainter feedback_opengl;
		FeedbackOpenGLToQPainter::ImageScope image_scope(feedback_opengl, gl);

		// The feedback image tiling loop...
		do
		{
			GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type tile_projection =
					image_scope.begin_render_tile();

			// Adjust the current projection transform - it'll get restored before the next tile though.
			GPlatesOpenGL::GLMatrix projection_matrix(tile_projection->get_matrix());
			projection_matrix.gl_mult_matrix(gl.gl_get_matrix(GL_PROJECTION));
			gl.gl_load_matrix(GL_PROJECTION, projection_matrix);

			// Clear the main framebuffer (colour and depth) before rendering the image.
			// We also clear the stencil buffer in case it is used - also it's usually interleaved
			// with depth so it's more efficient to clear both depth and stencil.
			gl.gl_clear_color();  // Clear to transparent (alpha=0) so regions outside sphere are transparent.
			gl.gl_clear_depth();
			gl.gl_clear_stencil();
			gl.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// Render the background sphere.
			gl.apply_compiled_draw_state(*d_full_screen_quad);
		}
		while (image_scope.end_render_tile());

		// Draw final raster QImage to feedback QPainter.
		image_scope.end_render();
	}
#endif
}
