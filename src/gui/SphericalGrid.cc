/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 The University of Sydney, Australia
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
#include <algorithm>
#include <opengl/OpenGL.h>

#include "SphericalGrid.h"

#include "Colour.h"

#include "maths/MathsUtils.h"
#include "maths/UnitVector3D.h"

#include "opengl/GLBlendState.h"
#include "opengl/GLCompositeDrawable.h"
#include "opengl/GLCompositeStateSet.h"
#include "opengl/GLPointLinePolygonState.h"
#include "opengl/GLRenderGraphDrawableNode.h"
#include "opengl/GLUNurbsRenderer.h"


namespace
{
	const GLfloat WEIGHT = static_cast<GLfloat>(1.0 / sqrt(2.0));

	/*
	 * The offset between successive curve control points.
	 * [The number of coords in a control point.]
	 */
	const GLint STRIDE = 4;

	/*
	 * The degree of the curve + 1.
	 */
	const GLint ORDER = 3;


	/**
	 * Creates an OpenGL state set that defines the appearance of the grid lines.
	 */
	GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
	create_state_set()
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


	/**
	 * Draw a line of latitude for latitude @a lat.
	 * The angle is in radians.
	 */
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
	draw_line_of_lat(
			const GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type &nurbs,
			const double &lat,
			const GPlatesGui::Colour &colour)
	{
		// The number of control points.
		static const GLsizei NUM_CONTROL_POINTS = 9;

		// Number of knots = order + number of control points.
		static const GLfloat KNOTS[] = {
			0.0, 0.0, 0.0,
			0.25, 0.25,
			0.5, 0.5,
			0.75, 0.75,
			1.0, 1.0, 1.0
		};
		boost::shared_array<GLfloat> knots(new GLfloat[sizeof(KNOTS) / sizeof(GLfloat)]);
		std::copy(KNOTS, KNOTS + sizeof(KNOTS) / sizeof(GLfloat), knots.get());

		/*
		 * We want to draw a small circle around the z-axis.
		 * Calculate the height (above z = 0) and radius of this circle.
		 */
		GLfloat height = static_cast<GLfloat>(std::sin(lat));
		GLfloat radius = static_cast<GLfloat>(std::cos(lat));

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

		return nurbs->draw_curve(
				sizeof(KNOTS) / sizeof(GLfloat), knots, STRIDE, ctrl_points, ORDER, GL_MAP1_VERTEX_4, colour);
	}


	/**
	 * Draw a line of longitude for longitude @a lon from the north pole to the south pole.
	 * The angle is in radians.
	 */
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
	draw_line_of_lon(
			const GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type &nurbs,
			const double &lon,
			const GPlatesGui::Colour &colour)
	{
		using namespace GPlatesMaths;

		// The number of control points.
		static const GLsizei NUM_CONTROL_POINTS = 5;

		// Number of knots = order + number of control points.
		static const GLfloat KNOTS[] = {
			0.0, 0.0, 0.0,
			0.5, 0.5,
			1.0, 1.0, 1.0
		};
		boost::shared_array<GLfloat> knots(new GLfloat[sizeof(KNOTS) / sizeof(GLfloat)]);
		std::copy(KNOTS, KNOTS + sizeof(KNOTS) / sizeof(GLfloat), knots.get());

		/*
		 * We want to draw a great circle which is bisected by the z-axis.
		 * 'p' is a point on the perimeter of the great circle.
		 */
		GLfloat p_x = static_cast<GLfloat>(std::cos(lon));
		GLfloat p_y = static_cast<GLfloat>(std::sin(lon));

#if 0  // This does the same thing as the code below, but *much* slower.
		PointOnSphere equatorial_pt(
				Vector3D(p_x, p_y, 0.0).get_normalisation());

		d_nurbs->draw_great_circle_arc(GreatCircleArc::create(PointOnSphere::north_pole, equatorial_pt));
		d_nurbs->draw_great_circle_arc(GreatCircleArc::create(equatorial_pt, PointOnSphere::south_pole));
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

		return nurbs->draw_curve(
				sizeof(KNOTS) / sizeof(GLfloat), knots, STRIDE, ctrl_points, ORDER, GL_MAP1_VERTEX_4, colour);
	}


	GPlatesOpenGL::GLTransform::non_null_ptr_type
	undo_rotation(
			const GPlatesMaths::UnitVector3D &axis,
			double angle_in_deg)
	{
		GPlatesOpenGL::GLTransform::non_null_ptr_type transform =
				GPlatesOpenGL::GLTransform::create(GL_MODELVIEW);
		// Undo the rotation done in Globe so that the disk always faces the camera.
		transform->get_matrix().gl_rotate(-angle_in_deg, axis.x().dval(), axis.y().dval(), axis.z().dval());
		
		return transform;
	}


	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
	create_grid_drawable(
			const GPlatesMaths::Real &delta_lat,
			const GPlatesMaths::Real &delta_lon,
			const GPlatesGui::Colour &colour)
	{
		GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type nurbs =
			GPlatesOpenGL::GLUNurbsRenderer::create();
		GPlatesOpenGL::GLCompositeDrawable::non_null_ptr_type composite_drawable =
			GPlatesOpenGL::GLCompositeDrawable::create();

		if (delta_lat != 0.0)
		{
			// Add lines of latitude: go down from +PI/2 to -PI/2.
			GPlatesMaths::Real curr_lat = GPlatesMaths::HALF_PI - delta_lat;
			while (curr_lat > -GPlatesMaths::HALF_PI)
			{
				composite_drawable->add_drawable(
						draw_line_of_lat(nurbs, curr_lat.dval(), colour));
				curr_lat -= delta_lat;
			}
		}

		if (delta_lon != 0.0)
		{
			// Add lines of longitude; go east from -PI to +PI.
			GPlatesMaths::Real curr_lon = -GPlatesMaths::PI;
			while (curr_lon < GPlatesMaths::PI)
			{
				composite_drawable->add_drawable(
						draw_line_of_lon(nurbs, curr_lon.dval(), colour));
				curr_lon += delta_lon;
			}
		}

		return composite_drawable;
	}


	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
	create_circumference_drawable(
			const GPlatesGui::Colour &colour)
	{
		GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type nurbs =
			GPlatesOpenGL::GLUNurbsRenderer::create();
		return draw_line_of_lon(nurbs, GPlatesMaths::PI / 2.0, colour);
	}
}


GPlatesGui::SphericalGrid::SphericalGrid(
		const GraticuleSettings &graticule_settings) :
	d_graticule_settings(graticule_settings),
	d_state_set(create_state_set())
{  }


void
GPlatesGui::SphericalGrid::paint(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node)
{
	// Check whether we need to create the drawables.
	if (!d_last_seen_graticule_settings ||
		*d_last_seen_graticule_settings != d_graticule_settings)
	{
		d_grid_drawable = create_grid_drawable(
				d_graticule_settings.get_delta_lat(),
				d_graticule_settings.get_delta_lon(),
				d_graticule_settings.get_colour());
		d_last_seen_graticule_settings = d_graticule_settings;
	}

	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(*d_grid_drawable);
	drawable_node->set_state_set(d_state_set);

	render_graph_parent_node->add_child_node(drawable_node);
}


void
GPlatesGui::SphericalGrid::paint_circumference(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
		const GPlatesMaths::UnitVector3D &axis,
		double angle_in_deg)
{
	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(
					create_circumference_drawable(d_graticule_settings.get_colour()));
	drawable_node->set_transform(undo_rotation(axis, angle_in_deg));
	drawable_node->set_state_set(d_state_set);

	render_graph_parent_node->add_child_node(drawable_node);
}

