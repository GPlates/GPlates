/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include <cmath>
#include <boost/function.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "Stars.h"

#include "Colour.h"
#include "FeedbackOpenGLToQPainter.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GLRenderer.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLVertex.h"
#include "opengl/GLVertexArray.h"

#include "presentation/ViewState.h"


namespace
{
	const GLfloat SMALL_STARS_SIZE = 1.4f;
	const GLfloat LARGE_STARS_SIZE = 2.1f;

	const unsigned int NUM_SMALL_STARS = 4250;
	const unsigned int NUM_LARGE_STARS = 3750;

	// Points sit on a sphere of this radius.
	// Note that ideally, we'd have these points at infinity - but we can't do
	// that, because we use an orthographic projection for the globe...
	const GLfloat RADIUS = 7.0f;

	typedef GPlatesOpenGL::GLColourVertex vertex_type;
	typedef GLushort vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;


	void
	stream_stars(
			stream_primitives_type &stream,
			boost::function< double () > &rand,
			unsigned int num_stars,
			const GPlatesGui::rgba8_t &colour)
	{
		bool ok = true;

		stream_primitives_type::Points stream_points(stream);
		stream_points.begin_points();

		unsigned int points_generated = 0;
		while (points_generated != num_stars)
		{
			// See http://mathworld.wolfram.com/SpherePointPicking.html for a discussion
			// of picking points uniformly on the surface of a sphere.
			// We use the method attributed to Marsaglia (1972).
			double x_1 = rand();
			double x_2 = rand();
			double x_1_sq = x_1 * x_1;
			double x_2_sq = x_2 * x_2;

			double stuff_under_sqrt = 1 - x_1_sq - x_2_sq;
			if (stuff_under_sqrt < 0)
			{
				continue;
			}
			double sqrt_part = std::sqrt(stuff_under_sqrt);

			double x = 2 * x_1 * sqrt_part;
			double y = 2 * x_2 * sqrt_part;
			double z = 1 - 2 * (x_1_sq + x_2_sq);

			// Randomising the distance to the stars gives a nicer 3D effect.
			double radius = RADIUS + rand();

			vertex_type vertex(x * radius, y * radius, z * radius, colour);
			ok = ok && stream_points.add_vertex(vertex);

			++points_generated;
		}

		stream_points.end_points();

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
	compile_stars_draw_state(
			GPlatesOpenGL::GLRenderer &renderer,
			GPlatesOpenGL::GLVertexArray &vertex_array,
			unsigned int &num_points,
			boost::function< double () > &rand,
			const GPlatesGui::rgba8_t &colour)
	{
		stream_primitives_type stream;

		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		stream_primitives_type::StreamTarget stream_target(stream);

		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		// Stream the small stars.
		stream_stars(stream, rand, NUM_SMALL_STARS, colour);

		const unsigned int num_small_star_vertices = stream_target.get_num_streamed_vertices();
		const unsigned int num_small_star_indices = stream_target.get_num_streamed_vertex_elements();

		stream_target.stop_streaming();

		// We re-start streaming so that we can get a separate stream count for the large stars.
		// However the large stars still get appended onto 'vertices' and 'vertex_elements'.
		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		// Stream the large stars.
		stream_stars(stream, rand, NUM_LARGE_STARS, colour);

		const unsigned int num_large_star_vertices = stream_target.get_num_streamed_vertices();
		const unsigned int num_large_star_indices = stream_target.get_num_streamed_vertex_elements();

		stream_target.stop_streaming();

		// Each point in GL_POINTS uses one vertex index (this is the total number of stars).
		num_points = vertex_elements.size();

		// We're using 16-bit indices (ie, 65536 vertices) so make sure we've not exceeded that many vertices.
		// Shouldn't get close really but check to be sure.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				vertices.size() - 1 <= GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
				GPLATES_ASSERTION_SOURCE);

		// Set up the vertex element buffer.
		GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_element_buffer = GPlatesOpenGL::GLBuffer::create(renderer);
		vertex_element_buffer->gl_buffer_data(
				renderer,
				GPlatesOpenGL::GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
				vertex_elements,
				GPlatesOpenGL::GLBuffer::USAGE_STATIC_DRAW);
		// Attached vertex element buffer to the vertex array.
		vertex_array.set_vertex_element_buffer(
				renderer,
				GPlatesOpenGL::GLVertexElementBuffer::create(renderer, vertex_element_buffer));

		// Set up the vertex buffer.
		GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_buffer = GPlatesOpenGL::GLBuffer::create(renderer);
		vertex_buffer->gl_buffer_data(
				renderer,
				GPlatesOpenGL::GLBuffer::TARGET_ARRAY_BUFFER,
				vertices,
				GPlatesOpenGL::GLBuffer::USAGE_STATIC_DRAW);
		// Attached vertex buffer to the vertex array.
		GPlatesOpenGL::bind_vertex_buffer_to_vertex_array<vertex_type>(
				renderer,
				vertex_array,
				GPlatesOpenGL::GLVertexBuffer::create(renderer, vertex_buffer));

		// Start compiling draw state that includes OpenGL states and vertex array draw commands.
		GPlatesOpenGL::GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

		// Set the alpha-blend state.
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Set the anti-aliased point state.
		renderer.gl_enable(GL_POINT_SMOOTH);
		renderer.gl_hint(GL_POINT_SMOOTH_HINT, GL_NICEST);

		// Bind the vertex array.
		vertex_array.gl_bind(renderer);

		// Small stars size.
		renderer.gl_point_size(SMALL_STARS_SIZE);

		// Draw the small stars.
		vertex_array.gl_draw_range_elements(
				renderer,
				GL_POINTS,
				0/*start*/,
				num_small_star_vertices - 1/*end*/,
				num_small_star_indices/*count*/,
				GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::type,
				0/*indices_offset*/);

		// Large stars size.
		renderer.gl_point_size(LARGE_STARS_SIZE);

		// Draw the large stars.
		// They come after the small stars in the vertex array.
		vertex_array.gl_draw_range_elements(
				renderer,
				GL_POINTS,
				num_small_star_vertices/*start*/,
				num_small_star_vertices + num_large_star_vertices - 1/*end*/,
				num_large_star_indices/*count*/,
				GPlatesOpenGL::GLVertexElementTraits<vertex_element_type>::type,
				num_small_star_indices * sizeof(vertex_element_type)/*indices_offset*/);

		return compile_draw_state_scope.get_compiled_draw_state();
	}
}


GPlatesGui::Stars::Stars(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesPresentation::ViewState &view_state,
		const GPlatesGui::Colour &colour) :
	d_view_state(view_state),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(renderer)),
	d_num_points(0)
{
	// Set up the random number generator.
	// It generates doubles uniformly from -1.0 to 1.0 inclusive.
	// Note that we use a fixed seed (0), so that the pattern of stars does not
	// change between sessions. This is useful when trying to reproduce screenshots
	// between sessions.
	boost::mt19937 gen(0);
	boost::uniform_real<> dist(-1.0, 1.0);
	boost::function< double () > rand(
			boost::variate_generator<boost::mt19937&, boost::uniform_real<> >(gen, dist));

	rgba8_t rgba8_colour = Colour::to_rgba8(colour);

	d_compiled_draw_state = compile_stars_draw_state(renderer, *d_vertex_array, d_num_points, rand, rgba8_colour);
}


void
GPlatesGui::Stars::paint(
		GPlatesOpenGL::GLRenderer &renderer)
{
	if (d_view_state.get_show_stars())
	{
		if (d_compiled_draw_state)
		{
			// Make sure we leave the OpenGL state the way it was.
			GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(renderer);

			// Either render directly to the framebuffer, or use OpenGL feedback to render to the
			// QPainter's paint device.
			if (renderer.rendering_to_context_framebuffer())
			{
				renderer.apply_compiled_draw_state(*d_compiled_draw_state.get());
			}
			else
			{
				// Create an OpenGL feedback buffer large enough to capture the primitives we're about to render.
				// We are rendering to the QPainter attached to GLRenderer.
				FeedbackOpenGLToQPainter feedback_opengl;
				FeedbackOpenGLToQPainter::VectorGeometryScope vector_geometry_scope(
						feedback_opengl, renderer, d_num_points, 0, 0);

				renderer.apply_compiled_draw_state(*d_compiled_draw_state.get());
			}
		}
	}
}
