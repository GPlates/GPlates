/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008 The University of Sydney, Australia
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
#include <opengl/OpenGL.h>

#include "Colour.h"
#include "SphericalGrid.h"

#include "opengl/GLBlendState.h"
#include "opengl/GLCompositeDrawable.h"
#include "opengl/GLCompositeStateSet.h"
#include "opengl/GLPointLinePolygonState.h"
#include "opengl/GLRenderGraphDrawableNode.h"


namespace
{
	static const GLfloat WEIGHT = static_cast<GLfloat>(1.0 / sqrt(2.0));

	/*
	 * The offset between successive curve control points.
	 * [The number of coords in a control point.]
	 */
	static const GLint STRIDE = 4;

	/*
	 * The degree of the curve + 1.
	 */
	static const GLint ORDER = 3;

	/*
	 * The number of control points.
	 */
	static const GLsizei NUM_CONTROL_POINTS = 9;

	/*
	 * The knot vector has (degree + length(ctrl_points) - 1) elements.
	 * [KNOT_SIZE == ORDER + NUM_CONTROL_POINTS.]
	 */
	static const GLsizei KNOT_SIZE = 12;

	// Would have made this const but OpenGL doesn't allow it.
	static GLfloat KNOTS[KNOT_SIZE] = {

		0.0, 0.0, 0.0,
		0.25, 0.25,
		0.5, 0.5,
		0.75, 0.75,
		1.0, 1.0, 1.0
	};
}


GPlatesGui::SphericalGrid::SphericalGrid(
		unsigned num_circles_lat,
		unsigned num_circles_lon):
	d_nurbs(GPlatesOpenGL::GLUNurbsRenderer::create()),
	d_state_set(create_state_set()),
	d_num_circles_lat(num_circles_lat),
	d_num_circles_lon(num_circles_lon)
{
	// Subdivide into stacks.
	unsigned num_stacks = d_num_circles_lat + 1;
	d_lat_delta = s_pi / num_stacks;

	// Subdivide into slices.
	// Assumes (d_num_circles_lon > 0).
	if (d_num_circles_lon > 0) {
		d_lon_delta = s_pi / d_num_circles_lon;
	}
}


void
GPlatesGui::SphericalGrid::paint(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
		const Colour &colour)
{
	GPlatesOpenGL::GLCompositeDrawable::non_null_ptr_type composite_drawable =
			GPlatesOpenGL::GLCompositeDrawable::create();

	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(composite_drawable);

	drawable_node->set_state_set(d_state_set);

	render_graph_parent_node->add_child_node(drawable_node);

	for (unsigned i = 0; i < d_num_circles_lat; i++) {
		double lat = ((i + 1) * d_lat_delta) - (s_pi / 2);
		composite_drawable->add_drawable(
				draw_line_of_lat(lat, colour));
	}
	for (unsigned j = 0; j < d_num_circles_lon; j++) {
		double lon = j * d_lon_delta;
		composite_drawable->add_drawable(
				draw_line_of_lon(lon, colour));
	}
}

GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesGui::SphericalGrid::draw_line_of_lat(
		const double &lat,
		const Colour &colour)
{
	boost::shared_array<GLfloat> knots(new GLfloat[KNOT_SIZE]);
	for (GLsizei n = 0; n < KNOT_SIZE; ++n)
	{
		knots[n] = KNOTS[n];
	}

	/*
	 * We want to draw a small circle around the z-axis.
	 * Calculate the height (above z = 0) and radius of this circle.
	 */
	GLfloat height = static_cast<GLfloat>(sin(lat));
	GLfloat radius = static_cast<GLfloat>(cos(lat));

	GLfloat u_radius = WEIGHT * radius;
	GLfloat u_height = WEIGHT * height;

	boost::shared_array<GLfloat> ctrl_points(new GLfloat[NUM_CONTROL_POINTS * STRIDE]);
	ctrl_points[0 * STRIDE + 0] = radius;
	ctrl_points[0 * STRIDE + 1] = 0.0f;
	ctrl_points[0 * STRIDE + 2] = height;
	ctrl_points[0 * STRIDE + 3] = 1.0f;
	ctrl_points[1 * STRIDE + 0] = u_radius;
	ctrl_points[1 * STRIDE + 1] = u_radius;
	ctrl_points[1 * STRIDE + 2] = u_height;
	ctrl_points[1 * STRIDE + 3] = WEIGHT;
	ctrl_points[2 * STRIDE + 0] = 0.0f;
	ctrl_points[2 * STRIDE + 1] = radius;
	ctrl_points[2 * STRIDE + 2] = height;
	ctrl_points[2 * STRIDE + 3] = 1.0f;
	ctrl_points[3 * STRIDE + 0] = -u_radius;
	ctrl_points[3 * STRIDE + 1] = u_radius;
	ctrl_points[3 * STRIDE + 2] = u_height;
	ctrl_points[3 * STRIDE + 3] = WEIGHT;
	ctrl_points[4 * STRIDE + 0] = -radius;
	ctrl_points[4 * STRIDE + 1] = 0.0f;
	ctrl_points[4 * STRIDE + 2] = height;
	ctrl_points[4 * STRIDE + 3] = 1.0f;
	ctrl_points[5 * STRIDE + 0] = -u_radius;
	ctrl_points[5 * STRIDE + 1] = -u_radius;
	ctrl_points[5 * STRIDE + 2] = u_height;
	ctrl_points[5 * STRIDE + 3] = WEIGHT;
	ctrl_points[6 * STRIDE + 0] = 0.0f;
	ctrl_points[6 * STRIDE + 1] = -radius;
	ctrl_points[6 * STRIDE + 2] = height;
	ctrl_points[6 * STRIDE + 3] = 1.0f;
	ctrl_points[7 * STRIDE + 0] = u_radius;
	ctrl_points[7 * STRIDE + 1] = -u_radius;
	ctrl_points[7 * STRIDE + 2] = u_height;
	ctrl_points[7 * STRIDE + 3] = WEIGHT;
	ctrl_points[8 * STRIDE + 0] = radius;
	ctrl_points[8 * STRIDE + 1] = 0.0f;
	ctrl_points[8 * STRIDE + 2] = height;
	ctrl_points[8 * STRIDE + 3] = 1.0f;

	return d_nurbs->draw_curve(
			KNOT_SIZE, knots, STRIDE, ctrl_points, ORDER, GL_MAP1_VERTEX_4, colour);
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesGui::SphericalGrid::draw_line_of_lon(
		const double &lon,
		const Colour &colour)
{
	using namespace GPlatesMaths;

	boost::shared_array<GLfloat> knots(new GLfloat[KNOT_SIZE]);
	for (GLsizei n = 0; n < KNOT_SIZE; ++n)
	{
		knots[n] = KNOTS[n];
	}

	/*
	 * We want to draw a great circle which is bisected by the z-axis.
	 * 'p' is a point on the perimeter of the great circle.
	 */
	GLfloat p_x = static_cast<GLfloat>(cos(lon));
	GLfloat p_y = static_cast<GLfloat>(sin(lon));

#if 0  // This does the same thing as the code below, but *much* slower.
	PointOnSphere equatorial_pt(
			Vector3D(p_x, p_y, 0.0).get_normalisation());

	d_nurbs->draw_great_circle_arc(GreatCircleArc::create(PointOnSphere::north_pole, equatorial_pt));
	d_nurbs->draw_great_circle_arc(GreatCircleArc::create(equatorial_pt, PointOnSphere::south_pole));
	d_nurbs->draw_great_circle_arc(GreatCircleArc::create(PointOnSphere::south_pole, get_antipodal_point(equatorial_pt)));
	d_nurbs->draw_great_circle_arc(GreatCircleArc::create(get_antipodal_point(equatorial_pt), PointOnSphere::north_pole));
#endif

	GLfloat u_p_x = WEIGHT * p_x;
	GLfloat u_p_y = WEIGHT * p_y;
	GLfloat u_p_z = WEIGHT * static_cast<GLfloat>(1.0);

	boost::shared_array<GLfloat> ctrl_points(new GLfloat[NUM_CONTROL_POINTS * STRIDE]);
	// North pole
	ctrl_points[0 * STRIDE + 0] = 0.0;
	ctrl_points[0 * STRIDE + 1] = 0.0;
	ctrl_points[0 * STRIDE + 2] = 1.0;
	ctrl_points[0 * STRIDE + 3] = 1.0;
	ctrl_points[1 * STRIDE + 0] = u_p_x;
	ctrl_points[1 * STRIDE + 1] = u_p_y;
	ctrl_points[1 * STRIDE + 2] = u_p_z;
	ctrl_points[1 * STRIDE + 3] = WEIGHT;
	ctrl_points[2 * STRIDE + 0] = p_x;
	ctrl_points[2 * STRIDE + 1] = p_y;
	ctrl_points[2 * STRIDE + 2] = 0.0;
	ctrl_points[2 * STRIDE + 3] = 1.0;
	ctrl_points[3 * STRIDE + 0] = u_p_x;
	ctrl_points[3 * STRIDE + 1] = u_p_y;
	ctrl_points[3 * STRIDE + 2] = -u_p_z;
	ctrl_points[3 * STRIDE + 3] = WEIGHT;
	// South pole
	ctrl_points[4 * STRIDE + 0] = 0.0;
	ctrl_points[4 * STRIDE + 1] = 0.0;
	ctrl_points[4 * STRIDE + 2] = -1.0;
	ctrl_points[4 * STRIDE + 3] = 1.0;
	ctrl_points[5 * STRIDE + 0] = -u_p_x;
	ctrl_points[5 * STRIDE + 1] = -u_p_y;
	ctrl_points[5 * STRIDE + 2] = -u_p_z;
	ctrl_points[5 * STRIDE + 3] = WEIGHT;
	ctrl_points[6 * STRIDE + 0] = -p_x;
	ctrl_points[6 * STRIDE + 1] = -p_y;
	ctrl_points[6 * STRIDE + 2] = 0.0;
	ctrl_points[6 * STRIDE + 3] = 1.0;
	ctrl_points[7 * STRIDE + 0] = -u_p_x;
	ctrl_points[7 * STRIDE + 1] = -u_p_y;
	ctrl_points[7 * STRIDE + 2] = u_p_z;
	ctrl_points[7 * STRIDE + 3] = WEIGHT;
	// North pole again (to close loop)
	ctrl_points[8 * STRIDE + 0] = 0.0;
	ctrl_points[8 * STRIDE + 1] = 0.0;
	ctrl_points[8 * STRIDE + 2] = 1.0;
	ctrl_points[8 * STRIDE + 3] = 1.0;

	return d_nurbs->draw_curve(
			KNOT_SIZE, knots, STRIDE, ctrl_points, ORDER, GL_MAP1_VERTEX_4, colour);
}


void
GPlatesGui::SphericalGrid::paint_circumference(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
		const GPlatesGui::Colour &colour)
{
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type drawable =
			draw_line_of_lon(s_pi/2.0, colour);

	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(drawable);

	drawable_node->set_state_set(d_state_set);

	render_graph_parent_node->add_child_node(drawable_node);
}


GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
GPlatesGui::SphericalGrid::create_state_set()
{
	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type composite_state =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Set the alpha-blend state.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	composite_state->add_state_set(blend_state);

	// Set the anti-aliased line state.
	GPlatesOpenGL::GLLineState::non_null_ptr_type line_state = GPlatesOpenGL::GLLineState::create();
	line_state->gl_enable_line_smooth(GL_TRUE).gl_hint_line_smooth(GL_NICEST).gl_line_width(1.0f);
	composite_state->add_state_set(line_state);

	return composite_state;
}


const double
GPlatesGui::SphericalGrid::s_pi = 3.14159265358979323846;
