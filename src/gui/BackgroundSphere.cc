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

#include "BackgroundSphere.h"

#include "FeedbackOpenGLToQPainter.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLShaderProgramUtils.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexUtils.h"

#include "presentation/ViewState.h"


namespace
{
	// Vertex and fragment shader source code to render background sphere in the 3D globe views (perspective and orthographic).
	const QString VERTEX_SHADER = ":/opengl/globe/render_background_sphere_vertex_shader.glsl";
	const QString FRAGMENT_SHADER = ":/opengl/globe/render_background_sphere_fragment_shader.glsl";
}


GPlatesGui::BackgroundSphere::BackgroundSphere(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesPresentation::ViewState &view_state) :
	d_view_state(view_state),
	d_background_colour(d_view_state.get_background_colour()),
	d_full_screen_quad(renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer))
{
	// Vertex shader.
	GPlatesOpenGL::GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
	vertex_shader_source.add_code_segment_from_file(VERTEX_SHADER);

	// Fragment shader.
	GPlatesOpenGL::GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(GPlatesOpenGL::GLShaderSource::UTILS_FILE_NAME);
	fragment_shader_source.add_code_segment_from_file(FRAGMENT_SHADER);

	// Vertex-fragment program.
	d_program = GPlatesOpenGL::GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
			renderer,
			vertex_shader_source,
			fragment_shader_source);
	if (!d_program)
	{
		// If shader compilation/linking failed (eg, due to lack of runtime shader support) then
		// return early. This means no background sphere will get rendered later.
		return;
	}

	// Set the background colour in the program object.
	d_program.get()->gl_uniform4f(renderer, "background_color", d_background_colour);
}


void
GPlatesGui::BackgroundSphere::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		bool depth_writes_enabled)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind the shader program.
	renderer.gl_bind_program_object(d_program.get());

	// If depth writes have been enabled then the shader program needs to output z-buffer depth.
	d_program.get()->gl_uniform1i(renderer, "write_depth", depth_writes_enabled);

	// Check whether the view state's background colour has changed.
	if (d_view_state.get_background_colour() != d_background_colour)
	{
		d_background_colour = d_view_state.get_background_colour();

		// Change the background colour in the program object.
		d_program.get()->gl_uniform4f(renderer, "background_color", d_background_colour);
	}

	// If the background colour is transparent then set up alpha blending.
	if (d_background_colour.alpha() < 1)
	{
		// Set up alpha blending for pre-multiplied alpha.
		// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
		// This is where the RGB channels have already been multiplied by the alpha channel.
		// See class GLVisualRasterSource for why this is done.
		//
		// To generate pre-multiplied alpha we'll use separate alpha-blend (src,dst) factors for the alpha channel...
		//
		//   RGB uses (src_alpha, 1 - src_alpha)  ->  (R,G,B) = (Rs*As,Gs*As,Bs*As) + (1-As) * (Rd,Gd,Bd)
		//     A uses (1, 1 - src_alpha)          ->        A = As + (1-As) * Ad
		if (renderer.get_capabilities().framebuffer.gl_EXT_blend_func_separate)
		{
			renderer.gl_enable(GL_BLEND);
			renderer.gl_blend_func_separate(
					GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
					GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		else // otherwise resort to normal blending...
		{
			renderer.gl_enable(GL_BLEND);
			renderer.gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

	// Either render directly to the framebuffer, or render to a QImage and draw that to the
	// feedback paint device using a QPainter.
	//
	// NOTE: For feedback to a QPainter we render to an image instead of rendering a full-screen quad geometry.
	if (renderer.rendering_to_context_framebuffer())
	{
		renderer.apply_compiled_draw_state(*d_full_screen_quad);
	}
	else
	{
		FeedbackOpenGLToQPainter feedback_opengl;
		FeedbackOpenGLToQPainter::ImageScope image_scope(feedback_opengl, renderer);

		// The feedback image tiling loop...
		do
		{
			GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type tile_projection =
					image_scope.begin_render_tile();

			// Adjust the current projection transform - it'll get restored before the next tile though.
			GPlatesOpenGL::GLMatrix projection_matrix(tile_projection->get_matrix());
			projection_matrix.gl_mult_matrix(renderer.gl_get_matrix(GL_PROJECTION));
			renderer.gl_load_matrix(GL_PROJECTION, projection_matrix);

			// Clear the main framebuffer (colour and depth) before rendering the image.
			// We also clear the stencil buffer in case it is used - also it's usually interleaved
			// with depth so it's more efficient to clear both depth and stencil.
			renderer.gl_clear_color();  // Clear to transparent (alpha=0) so regions outside sphere are transparent.
			renderer.gl_clear_depth();
			renderer.gl_clear_stencil();
			renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// Render the background sphere.
			renderer.apply_compiled_draw_state(*d_full_screen_quad);
		}
		while (image_scope.end_render_tile());

		// Draw final raster QImage to feedback QPainter.
		image_scope.end_render();
	}
}
