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
 */
#include "SphericalGrid.h"
#include "maths/types.h"
#include <cmath>

using namespace GPlatesMaths;
using namespace GPlatesGui;

// Catch errors
static void
NurbsError(GLenum error)
{
	std::cerr << "NURBS Error: " << gluErrorString(error) << std::endl;
	exit(1);
}

namespace
{
	// The knot vector has (degree + length(ctrl_points) - 1)
	// elements.
	static const GLsizei KNOT_SIZE = 12;
	static GLfloat knots[KNOT_SIZE] = {  // would make this const but
		0.0, 0.0, 0.0,                   // opengl doesn't allow it. 
		0.25, 0.25,
		0.5, 0.5,
		0.75, 0.75,
		1.0, 1.0, 1.0
	};
}

static void
DrawSmallCircle(GLUnurbsObj* nurbs_renderer, const real_t& latitude)
{
	const real_t rad_lat = degreesToRadians(latitude);
	const GLfloat radius = cos(rad_lat).dval();
	const GLfloat height = sin(rad_lat).dval();

	GLfloat P = radius;

	static const GLfloat WEIGHT = 1.0f / std::sqrt(2.0f);
	GLfloat uP = P*WEIGHT;
	GLfloat uheight = height*WEIGHT;

	static const GLsizei NUM_CONTROL_POINTS = 9;
	static const GLint   STRIDE = 4; // num of coords in a ctrl_point
	GLfloat ctrl_points[NUM_CONTROL_POINTS][STRIDE] = {
		{ radius, 0.0, height, 1.0 },
		{ uP, uP, uheight, WEIGHT },
		{ 0.0, radius, height, 1.0 },
		{ -uP, uP, uheight, WEIGHT },
		{ -radius, 0.0, height, 1.0 },
		{ -uP, -uP, uheight, WEIGHT },
		{ 0.0, -radius, height, 1.0 },
		{ uP, -uP, uheight, WEIGHT },
		{ radius, 0.0, height, 1.0 }
	};
	static const GLint ORDER = 3; // XXX: I forget what this does.

	/*
	 * Draw the NURBS.
	 * Draw one quarter of the circle at a time.  Do this
	 * because the performance cost increases (non-linearly)
	 * with the number of control points; each quarter has 3 
	 * control points, compared to 8 for the whole circle.
	 * XXX: for reasons that elude me, *each* of the calls to
	 *   gluNurbsCurve needs to be wrapped in a glu{Begin,End}Curve
	 *   block; it would be preferable to have the four calls 
	 *   inside a single block.
	 */
//	glNewList(display_list_id, GL_COMPILE_AND_EXECUTE);

	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[0][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);
#if 0
	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[2][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);

	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[4][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);

	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[6][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);
#endif
//	glEndList();
}

static void
DrawGreatCircle(GLUnurbsObj* nurbs_renderer, const real_t& longitude)
{
	/*
	 * P is a point on the perimeter of the great circle.
	 */
	const real_t rad_lon = degreesToRadians(longitude);
	GLfloat P_x = cos(rad_lon).dval();
	GLfloat P_y = sin(rad_lon).dval();
	GLfloat P_z = 1.0;

	static const GLfloat WEIGHT = 1.0f / std::sqrt(2.0f);

	GLfloat uP_x = P_x*WEIGHT,
		uP_y = P_y*WEIGHT,
		uP_z = P_z*WEIGHT;

	static const GLsizei NUM_CONTROL_POINTS = 9;
	static const GLint   STRIDE = 4; // num of coords in a ctrl_point
	GLfloat ctrl_points[NUM_CONTROL_POINTS][STRIDE] = {
		{ 0.0, 0.0, 1.0, 1.0 },                // North pole
		{ uP_x, uP_y, uP_z, WEIGHT },
		{ P_x, P_y, 0.0, 1.0 },
		{ uP_x, uP_y, -uP_z, WEIGHT },
		{ 0.0, 0.0, -1.0, 1.0 },               // South pole
		{ -uP_x, -uP_y, -uP_z, WEIGHT },
		{ -P_x, -P_y, 0.0, 1.0 },
		{ -uP_x, -uP_y, uP_z, WEIGHT },
		{ 0.0, 0.0, 1.0, 1.0 }     // North pole (repeated to close loop).
	};


	static const GLint ORDER = 3; // XXX: I forget what this does.

	/*
	 * Draw the NURBS.
	 * Draw one quarter of the circle at a time.  Do this
	 * because the performance cost increases (non-linearly)
	 * with the number of control points; each quarter has 3 
	 * control points, compared to 8 for the whole circle.
	 * XXX: for reasons that elude me, *each* of the calls to
	 *   gluNurbsCurve needs to be wrapped in a glu{Begin,End}Curve
	 *   block; it would be preferable to have the four calls 
	 *   inside a single block.
	 */
//	glNewList(display_list_id, GL_COMPILE_AND_EXECUTE);

	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[0][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);
#if 0
	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[2][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);

	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[4][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);

	gluBeginCurve(nurbs_renderer);
		gluNurbsCurve(nurbs_renderer, KNOT_SIZE, &knots[0], STRIDE,
		 &ctrl_points[6][0], ORDER, GL_MAP1_VERTEX_4);
	gluEndCurve(nurbs_renderer);
#endif
//	glEndList();
}

namespace 
{
	GLUnurbsObj* nurbs_renderer;
}

static void
CompileGrid(const real_t& degrees_per_lat,
			const real_t& degrees_per_lon,
			const Colour& colour, int)
{
	glColor3fv(colour);

	GLfloat incr = degrees_per_lon.dval();
	for (GLfloat lon = 0.0; lon < 180.0f; lon += incr)
		DrawGreatCircle(nurbs_renderer, lon);

	incr = degrees_per_lat.dval();
	for (GLfloat lat = 90.0 - incr; lat > -90.0; lat -= incr)
		DrawSmallCircle(nurbs_renderer, lat);
}

SphericalGrid::SphericalGrid(const real_t& degrees_per_lat,
	const real_t& degrees_per_lon,
	const Colour& colour)
{
	// Create renderer.
	nurbs_renderer = gluNewNurbsRenderer();
	gluNurbsCallback(nurbs_renderer, GLU_ERROR,
		reinterpret_cast<void (*)()>(&NurbsError));

	CompileGrid(degrees_per_lat, degrees_per_lon, colour, GRID);
}


SphericalGrid::~SphericalGrid()
{
	gluDeleteNurbsRenderer(nurbs_renderer);
}

void
SphericalGrid::Paint()
{
//	glColor3fv(_colour);
//	glCallList(GRID);
	CompileGrid(30, 30, Colour::WHITE, 0);
}
