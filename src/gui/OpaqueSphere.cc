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

#include "OpaqueSphere.h"

#include "FeedbackOpenGLToQPainter.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "opengl/GLMatrix.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLVertex.h"
#include "opengl/GLVertexArray.h"

#include "presentation/ViewState.h"


namespace
{
	static const double RADIUS = 1.0;
	static const unsigned int NUM_SLICES = 72;

	typedef GPlatesOpenGL::GLColourVertex vertex_type;
	typedef GLushort vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;

	typedef std::pair<double, double> double_pair;


	/**
	 * Computes the sin and cos of:
	 *     2 * PI * i / num_slices
	 * for 0 <= i <= num_slices.
	 *
	 * Note that the returned vector has (num_slices + 1) elements.
	 */
	std::vector<double_pair>
	compute_sin_cos_angles(
			unsigned int num_slices)
	{
		std::vector<double_pair> result;
		for (unsigned int i = 0; i <= num_slices; ++i)
		{
			double angle = 2 * GPlatesMaths::PI * i / num_slices;
			result.push_back(std::make_pair(std::sin(angle), std::cos(angle)));
		}
		return result;
	}


	/**
	 * Creates a donut-shaped drawable on the z = 0 plane.
	 */
	void
	stream_disk(
			stream_primitives_type &stream,
			double inner_radius,
			double outer_radius,
			const std::vector<double_pair> &sin_cos_angles,
			const GPlatesGui::rgba8_t &inner_colour,
			const GPlatesGui::rgba8_t &outer_colour)
	{
		static const GLfloat Z_VALUE = 0;

		bool ok = true;

		stream_primitives_type::TriangleStrips stream_triangle_strips(stream);
		stream_triangle_strips.begin_triangle_strip();

		for (std::vector<double_pair>::const_iterator iter = sin_cos_angles.begin();
				iter != sin_cos_angles.end(); ++iter)
		{
			const vertex_type outer_vertex(
				static_cast<GLfloat>(outer_radius * iter->first),
				static_cast<GLfloat>(outer_radius * iter->second),
				Z_VALUE,
				outer_colour);
			const vertex_type inner_vertex(
				static_cast<GLfloat>(inner_radius * iter->first),
				static_cast<GLfloat>(inner_radius * iter->second),
				Z_VALUE,
				inner_colour);

			ok = ok && stream_triangle_strips.add_vertex(outer_vertex);
			ok = ok && stream_triangle_strips.add_vertex(inner_vertex);
		}

		stream_triangle_strips.end_triangle_strip();

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	/**
	 * Evaluates the integral of sqrt(r^2 - x^2) with respect to x for a given
	 * value of r and x (ignoring the constant of integration).
	 */
	double
	eval_integral(
			double x,
			double r)
	{
		double sqrt_part = std::sqrt(r * r - x * x);
		return 0.5 * (x * sqrt_part + r * r * std::atan2(x, sqrt_part));
	}


	/**
	 * Draws a disk on the z = 0 plane with varying translucency from centre to
	 * edge, that simulates what a real translucent sphere would look like.
	 *
	 * Imagine a translucent balloon, and consider parallel light rays travelling
	 * from behind the balloon towards the viewer. A light ray going through the
	 * centre of the balloon has to go through less material than a light ray going
	 * through the balloon further away from the centre, where the balloon's
	 * surface is more slanted relative to the viewer.
	 *
	 * We model this using a 2D doughnut (for ease of calculation), with an outer
	 * radius of RADIUS and a very small thickness. We calculate the amount
	 * of doughnut in each equal slice from x = 0 to x = RADIUS. The alpha
	 * value of @a colour is used at the centre of the disk, and as we go outwards,
	 * the alpha value is modulated by the amount of doughnut in that slice.
	 */
	void
	stream_translucent_sphere(
			stream_primitives_type &stream,
			const GPlatesGui::rgba8_t &colour)
	{
		static const unsigned int STEPS = 150;
		static const double INNER_RADIUS = (STEPS - 0.5) / STEPS * RADIUS;

		double radii[STEPS + 1];
		double integral[STEPS + 1];
		double inner_integral[STEPS + 1];
		for (unsigned int i = 0; i <= STEPS; ++i)
		{
			double curr_radius = RADIUS * i / STEPS;
			radii[i] = curr_radius;
			integral[i] = eval_integral(curr_radius, RADIUS);
			inner_integral[i] = eval_integral(curr_radius > INNER_RADIUS ? INNER_RADIUS : curr_radius, INNER_RADIUS);
		}

		double thickness[STEPS];
		for (unsigned int i = 0; i < STEPS; ++i)
		{
			double outer_area = integral[i + 1] - integral[i];
			double inner_area = inner_integral[i + 1] - inner_integral[i];
			thickness[i] = outer_area - inner_area;
		}

		double base_thickness = thickness[0];
		double base_alpha = colour.alpha / 255.0;
		boost::uint8_t alphas[STEPS + 1];
		alphas[0] = colour.alpha;
		for (unsigned int i = 0; i < STEPS; ++i)
		{
			double alpha = 1.0 - std::pow(1.0 - base_alpha, thickness[i] / base_thickness);
			alphas[i + 1] = static_cast<boost::uint8_t>(alpha * 255.0);
		}

		std::vector<double_pair> sin_cos_angles = compute_sin_cos_angles(NUM_SLICES);

		for (unsigned int i = 0; i < STEPS; ++i)
		{
			GPlatesGui::rgba8_t inner_colour = colour;
			inner_colour.alpha = alphas[i];
			GPlatesGui::rgba8_t outer_colour = colour;
			outer_colour.alpha = alphas[i + 1];

			stream_disk(stream, radii[i], radii[i + 1], sin_cos_angles, inner_colour, outer_colour);
		}
	}


	/**
	 * Draws a disk on the z = 0 plane with a fixed @a colour.
	 */
	void
	stream_opaque_sphere(
			stream_primitives_type &stream,
			const GPlatesGui::rgba8_t &colour)
	{
		std::vector<double_pair> sin_cos_angles = compute_sin_cos_angles(NUM_SLICES);

		stream_disk(stream, 0.0, RADIUS, sin_cos_angles, colour, colour);
	}


	/**
	 * Creates a compiled draw state that renders the sphere to the screen.
	 *
	 * Note that this actually draws a flat disk on the z = 0 plane instead of an
	 * actual sphere. This has the advantage that, if we draw to the depth buffer
	 * while drawing the disk, we can use a depth test to occlude geometries on the
	 * far side of the globe, but since the disk cuts through the centre of the
	 * globe, we avoid any artifacts due to geometries dipping in and out of the
	 * surface of the globe.
	 */
	GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
	compile_sphere_draw_state(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesOpenGL::GLVertexArray &vertex_array,
			const GPlatesGui::rgba8_t &colour)
	{
		const bool transparent = (colour.alpha < 255);

		stream_primitives_type stream;

		// We'll stream vertices/indices into std::vector's since we don't know
		// how many vertices there will be.
		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		stream_primitives_type::StreamTarget stream_target(stream);
		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		if (transparent)
		{
			stream_translucent_sphere(stream, colour);
		}
		else
		{
			stream_opaque_sphere(stream, colour);
		}

		stream_target.stop_streaming();

		// We're using 16-bit indices (ie, 65536 vertices) so make sure we've not exceeded that many vertices.
		// Shouldn't get close really but check to be sure.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				vertices.size() - 1 <= GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
				GPLATES_ASSERTION_SOURCE);

		// Streamed triangle strips end up as indexed triangles.
		const GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type draw_vertex_array =
				compile_vertex_array_draw_state(
						renderer, vertex_array, vertices, vertex_elements, GL_TRIANGLES);

		// Start compiling draw state that includes alpha blend state and the vertex array draw command.
		GPlatesOpenGL::GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

		if (transparent)
		{
			renderer.gl_enable(GL_BLEND);
			renderer.gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		renderer.apply_compiled_draw_state(*draw_vertex_array);

		return compile_draw_state_scope.get_compiled_draw_state();
	}


	void
	undo_rotation(
			GPlatesOpenGL::GLMatrix &transform,
			const GPlatesMaths::UnitVector3D &axis,
			double angle_in_deg)
	{
		// Undo the rotation done in Globe so that the disk always faces the camera.
		transform.gl_rotate(-angle_in_deg, axis.x().dval(), axis.y().dval(), axis.z().dval());

		// Rotate the axes so that the z-axis is perpendicular to the screen.
		// This is because we draw the disk on the z = 0 plane.
		transform.gl_rotate(90, 0.0, 1.0, 0.0);
	}
}


GPlatesGui::OpaqueSphere::OpaqueSphere(
		GPlatesOpenGL::GLRenderer &renderer,
		const Colour &colour) :
	d_view_state(NULL),
	d_colour(colour),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer)),
	d_compiled_draw_state(compile_sphere_draw_state(renderer, *d_vertex_array, Colour::to_rgba8(d_colour)))
{  }


GPlatesGui::OpaqueSphere::OpaqueSphere(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesPresentation::ViewState &view_state) :
	d_view_state(&view_state),
	d_colour(view_state.get_background_colour()),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer)),
	d_compiled_draw_state(compile_sphere_draw_state(renderer, *d_vertex_array, Colour::to_rgba8(d_colour)))
{  }


void
GPlatesGui::OpaqueSphere::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesMaths::UnitVector3D &axis,
		double angle_in_deg)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

	// Check whether the view state's background colour has changed.
	if (d_view_state &&
		d_view_state->get_background_colour() != d_colour)
	{
		d_colour = d_view_state->get_background_colour();
		d_compiled_draw_state = compile_sphere_draw_state(renderer, *d_vertex_array, Colour::to_rgba8(d_colour));
	}

	GPlatesOpenGL::GLMatrix transform;
	undo_rotation(transform, axis, angle_in_deg);

	renderer.gl_mult_matrix(GL_MODELVIEW, transform);

	// Either render directly to the framebuffer, or render to a QImage and draw that to the
	// feedback paint device using a QPainter.
	//
	// NOTE: For feedback to a QPainter we render to an image instead of rendering vector geometries.
	// This is because, for SVG output, we don't want a large number of vector geometries due to this
	// opaque sphere - we really only want actual geological data and grid lines as SVG vector data.
	if (renderer.rendering_to_context_framebuffer())
	{
		renderer.apply_compiled_draw_state(*d_compiled_draw_state);
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
			renderer.gl_clear_color();
			renderer.gl_clear_depth();
			renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Render the actual opaque sphere.
			renderer.apply_compiled_draw_state(*d_compiled_draw_state);
		}
		while (image_scope.end_render_tile());

		// Draw final raster QImage to feedback QPainter.
		image_scope.end_render();
	}
}
