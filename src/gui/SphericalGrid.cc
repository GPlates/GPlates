/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <cmath>
#include "SphericalGrid.h"
#include "OpenGL.h"


namespace
{
	static const GLfloat WEIGHT = 1.0 / sqrt(2.0);

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


void
GPlatesGui::SphericalGrid::Paint() {

	glColor3fv(_colour);

	for (unsigned i = 0; i < _num_circles_lat; i++) {

		double lat = ((i + 1) * _lat_delta) - (pi / 2);
		drawLineOfLat(lat);
	}

	for (unsigned j = 0; j < _num_circles_lon; j++) {

		double lon = j * _lon_delta;
		drawLineOfLon(lon);
	}
}


void
GPlatesGui::SphericalGrid::drawLineOfLat(double lat) {

	/*
	 * We want to draw a small circle around the z-axis.
	 * Calculate the height (above z = 0) and radius of this circle.
	 */
	GLfloat height = sin(lat);
	GLfloat radius = cos(lat);

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

	_nurbs.drawCurve(KNOT_SIZE, &knots[0], STRIDE, &ctrl_points[0][0],
	 ORDER, GL_MAP1_VERTEX_4);
}


void
GPlatesGui::SphericalGrid::drawLineOfLon(double lon) {

	/*
	 * We want to draw a great circle which is bisected by the z-axis.
	 * 'p' is a point on the perimeter of the great circle.
	 */
	GLfloat p_x = cos(lon);
	GLfloat p_y = sin(lon);

	GLfloat u_p_x = WEIGHT * p_x;
	GLfloat u_p_y = WEIGHT * p_y;
	GLfloat u_p_z = WEIGHT * 1.0;

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

	_nurbs.drawCurve(KNOT_SIZE, &knots[0], STRIDE, &ctrl_points[0][0],
	 ORDER, GL_MAP1_VERTEX_4);
}


const double
GPlatesGui::SphericalGrid::pi = 3.14159265358979323846;
