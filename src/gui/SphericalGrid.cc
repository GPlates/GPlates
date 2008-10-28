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

#include "SphericalGrid.h"
#include "OpenGL.h"
#include "Colour.h"


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
	static GLfloat knots[KNOT_SIZE] = {

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
		const Colour &colour)
{
	glColor3fv(colour);
	glLineWidth(1.0f);

	for (unsigned i = 0; i < d_num_circles_lat; i++) {
		double lat = ((i + 1) * d_lat_delta) - (s_pi / 2);
		draw_line_of_lat(lat);
	}
	for (unsigned j = 0; j < d_num_circles_lon; j++) {
		double lon = j * d_lon_delta;
		draw_line_of_lon(lon);
	}
}

void
GPlatesGui::SphericalGrid::draw_line_of_lat(
		const double &lat)
{
	/*
	 * We want to draw a small circle around the z-axis.
	 * Calculate the height (above z = 0) and radius of this circle.
	 */
	GLfloat height = static_cast<GLfloat>(sin(lat));
	GLfloat radius = static_cast<GLfloat>(cos(lat));

	GLfloat u_radius = WEIGHT * radius;
	GLfloat u_height = WEIGHT * height;

	GLfloat ctrl_points[NUM_CONTROL_POINTS][STRIDE] = {

		{ radius, 0.0, height, 1.0 },
		{ u_radius, u_radius, u_height, WEIGHT },
		{ 0.0, radius, height, 1.0 },
		{ -u_radius, u_radius, u_height, WEIGHT },
		{ -radius, 0.0, height, 1.0 },
		{ -u_radius, -u_radius, u_height, WEIGHT },
		{ 0.0, -radius, height, 1.0 },
		{ u_radius, -u_radius, u_height, WEIGHT },
		{ radius, 0.0, height, 1.0 }
	};

	d_nurbs->draw_curve(KNOT_SIZE, &knots[0], STRIDE, &ctrl_points[0][0], ORDER, GL_MAP1_VERTEX_4);
}


void
GPlatesGui::SphericalGrid::draw_line_of_lon(
		const double &lon)
{
	using namespace GPlatesMaths;

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

	GLfloat ctrl_points[NUM_CONTROL_POINTS][STRIDE] = {

		{ 0.0, 0.0, 1.0, 1.0 },    // North pole
		{ u_p_x, u_p_y, u_p_z, WEIGHT },
		{ p_x, p_y, 0.0, 1.0 },
		{ u_p_x, u_p_y, -u_p_z, WEIGHT },
		{ 0.0, 0.0, -1.0, 1.0 },   // South pole
		{ -u_p_x, -u_p_y, -u_p_z, WEIGHT },
		{ -p_x, -p_y, 0.0, 1.0 },
		{ -u_p_x, -u_p_y, u_p_z, WEIGHT },
		{ 0.0, 0.0, 1.0, 1.0 }     // North pole again (to close loop).
	};

	d_nurbs->draw_curve(KNOT_SIZE, &knots[0], STRIDE, &ctrl_points[0][0], ORDER, GL_MAP1_VERTEX_4);
}


void
GPlatesGui::SphericalGrid::paint_circumference(
		const GPlatesGui::Colour &colour)
{
	glColor3fv(colour);

	draw_line_of_lon(s_pi/2.0);
	draw_line_of_lat(-s_pi/2.0);
}


const double
GPlatesGui::SphericalGrid::s_pi = 3.14159265358979323846;
