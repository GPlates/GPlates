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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

#include <iostream>
#include <cstdlib>
#include "Globe.h"

using namespace GPlatesGui;

const GLfloat Globe::DEFAULT_RADIUS = 1.0;
const Colour Globe::DEFAULT_COLOUR = Colour::WHITE;

void
Globe::NormaliseMeridianElevation()
{
	if (_elevation < 0.0) {
		// get back into [0, 360) range 
		_elevation += 360.0;
	} else if (_elevation >= 360.0) {
		// cut back a revolution
		_elevation -= 360.0;
	}
	if (_meridian < 0.0) {
		// get back into [0, 360) range
		_meridian += 360.0;
	} else if (_meridian >= 360.0) {
		// cut back a revolution
		_meridian -= 360.0;
	}
}

class UnitVector
{
	public:
		GLfloat x, y, z;

		UnitVector(const GLfloat& xx, const GLfloat& yy,
		 const GLfloat& zz) : x(xx), y(yy), z(zz) {  }

		GLfloat dot(const UnitVector& o) const { 
			return o.x*x + o.y*y + o.z*z;
		}
};

static void
DrawArc(const UnitVector& a, const UnitVector& b, 
 GLUnurbsObj *renderer)
{
	// The knot vector has (degree + length(ctrl_points) - 1)
	// elements.
	static const GLsizei KNOT_SIZE = 6;
	GLfloat knots[KNOT_SIZE] = {
		0.0, 0.0, 0.0, 1.0, 1.0, 1.0
	};

	// FIXME: There should be a way to avoid copying these values.
	GLfloat ctrl_points[3][4] = {
		{ a.x, a.y, a.z, 1.0 },
		{ 0.0, 0.0, 0.0, 0.0 },  // Dummy value, still to be calculated.
		{ b.x, b.y, b.z, 1.0 }
	};

	static const GLfloat ONE_DIV_ROOT_TWO = 
	 1.0 / static_cast<GLfloat>(sqrt(2.0));

	// Calculate the weight of the control point.
	ctrl_points[1][3] = 
	 ONE_DIV_ROOT_TWO * static_cast<GLfloat>(sqrt(1.0 + a.dot(b)));

	// Calculate the control point itself.
	// 1) a + b
	ctrl_points[1][0] = a.x + b.x;
	ctrl_points[1][1] = a.y + b.y;
	ctrl_points[1][2] = a.z + b.z;
	// 2) scale = 1 / size
	GLfloat ctrlptscale = 1 / static_cast<GLfloat>(
		sqrt(ctrl_points[1][0]*ctrl_points[1][0] + 
		 ctrl_points[1][1]*ctrl_points[1][1] + 
		 ctrl_points[1][2]*ctrl_points[1][2]));
	ctrl_points[1][0] *= ctrlptscale;
	ctrl_points[1][1] *= ctrlptscale;
	ctrl_points[1][2] *= ctrlptscale;

	// FIXME: Change the colour depending on the line.
	glColor3fv(Colour::RED);
	glLineWidth(3.0);
	
	// Draw the NURBS.
	gluBeginCurve(renderer);
		gluNurbsCurve(renderer, KNOT_SIZE, &knots[0], 4, 
		 &ctrl_points[0][0], 3, GL_MAP1_VERTEX_4);
	gluEndCurve(renderer);
}

void
Globe::Draw()
{
	glPushMatrix();
		// Ensure that the meridian and elevation are in the acceptable 
		// range.
		NormaliseMeridianElevation();

		// rotate everything to get a nice almost-equatorial shot
		glRotatef(-80.0, 1.0, 0.0, 0.0);

		// rotate everything (around x) according to elevation rotation
		// FIXME: This should be combined with the previous rotation so
		//  only one is submitted.
		glRotatef(_elevation, 1.0, 0.0, 0.0);

		// rotate everything (around z) according to meridian rotation
		glRotatef(_meridian, 0.0, 0.0, 1.0);

		// Set the globe's colour.
		glColor3fv(_colour);

		// Draw globe.
		glLineWidth(0.01);
		glutWireSphere(_radius, _slices, _stacks);

		// XXX: Draw NURBS (temporary measure)
		DrawArc(UnitVector(1.0, 0.0, 0.0), UnitVector(0.0, 0.0, 1.0), _nurbs_renderer);
		DrawArc(UnitVector(1.0, 0.0, 0.0), UnitVector(0.0, 1.0, 0.0), _nurbs_renderer);
		DrawArc(UnitVector(0.707107, 0.707107, 0.0), UnitVector(0.0, 0.707107, 0.707107), _nurbs_renderer);

	glPopMatrix();
}
