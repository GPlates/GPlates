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

#include "Stars.h"

#include "Colour.h"

#include "opengl/GLBlendState.h"
#include "opengl/GLCompositeDrawable.h"
#include "opengl/GLCompositeStateSet.h"
#include "opengl/GLPointLinePolygonState.h"
#include "opengl/GLRenderGraphDrawableNode.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/Vertex.h"

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

	typedef GPlatesOpenGL::Vertex vertex_type;

	boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type>
	create_stars_drawable(
			boost::function< double () > &rand,
			unsigned int num_stars,
			const GPlatesGui::rgba8_t &colour)
	{
		// Add points to a stream.
		GPlatesOpenGL::GLStreamPrimitives<vertex_type>::non_null_ptr_type stream =
			GPlatesOpenGL::create_stream<vertex_type>();
		GPlatesOpenGL::GLStreamPoints<vertex_type> stream_points(*stream);
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

			vertex_type vertex = { x * radius, y * radius, z * radius, colour };
			stream_points.add_vertex(vertex);

			++points_generated;
		}

		stream_points.end_points();

		boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> points_drawable =
			stream->get_drawable();
		return points_drawable;
	}


	GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
	create_state_set(
			GLfloat size)
	{
		GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type composite_state =
				GPlatesOpenGL::GLCompositeStateSet::create();

		// Set the alpha-blend state.
		GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
				GPlatesOpenGL::GLBlendState::create();
		blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		composite_state->add_state_set(blend_state);

		// Set the anti-aliased point state.
		GPlatesOpenGL::GLPointState::non_null_ptr_type point_state = GPlatesOpenGL::GLPointState::create();
		point_state->gl_enable_point_smooth(GL_TRUE).gl_hint_point_smooth(GL_NICEST).gl_point_size(size);
		composite_state->add_state_set(point_state);

		return composite_state;
	}
}


GPlatesGui::Stars::Stars(
		GPlatesPresentation::ViewState &view_state,
		const GPlatesGui::Colour &colour) :
	d_view_state(view_state),
	d_small_stars_state_set(create_state_set(SMALL_STARS_SIZE)),
	d_large_stars_state_set(create_state_set(LARGE_STARS_SIZE))
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

	d_small_stars_drawable = create_stars_drawable(rand, NUM_SMALL_STARS, rgba8_colour);
	d_large_stars_drawable = create_stars_drawable(rand, NUM_LARGE_STARS, rgba8_colour);
}


void
GPlatesGui::Stars::paint(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node)
{
	if (d_view_state.get_show_stars())
	{
		draw_stars(render_graph_parent_node, d_small_stars_drawable, d_small_stars_state_set);
		draw_stars(render_graph_parent_node, d_large_stars_drawable, d_large_stars_state_set);
	}
}


void
GPlatesGui::Stars::draw_stars(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
		const boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> &stars_drawable,
		const GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type &state_set)
{
	if (stars_drawable)
	{
		GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type small_stars_drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(stars_drawable.get());
		small_stars_drawable_node->set_state_set(state_set);

		render_graph_parent_node->add_child_node(small_stars_drawable_node);
	}
}

