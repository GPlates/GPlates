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
#include "maths/types.h"
#include "maths/UnitVector3D.h"

using namespace GPlatesGui;
using namespace GPlatesMaths;

const GLfloat Globe::DEFAULT_RADIUS = 1.0;
const Colour Globe::DEFAULT_COLOUR = Colour::WHITE;

static void
QuadricError(GLenum error)
{
	std::cerr << "Quadric Error: " << gluErrorString(error) << std::endl;
	exit(1);
}

static void
NurbsError(GLenum error)
{
	std::cerr << "NURBS Error: " << gluErrorString(error) << std::endl;
	exit(1);
}

Globe::Globe(Colour colour, GLfloat radius, GLint slices, GLint stacks)
	: _colour(colour), _radius(radius), _slices(slices), 
	  _stacks(stacks), _meridian(0.0), _elevation(0.0)
{
	_sphere = gluNewQuadric();
	gluQuadricNormals(_sphere, GLU_NONE);	// Don't generate normals
	gluQuadricTexture(_sphere, GL_FALSE);	// Don't generate texture coords
	gluQuadricDrawStyle(_sphere, GLU_LINE);	// Draw wireframe
	gluQuadricCallback(_sphere, GLU_ERROR, 
		reinterpret_cast<void (*)()>(&QuadricError));
	
	_nurbs_renderer = gluNewNurbsRenderer();
	gluNurbsCallback(_nurbs_renderer, GLU_ERROR,
		reinterpret_cast<void (*)()>(&NurbsError));
}

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


// FIXME: Use GPlatesMaths::GreatCircleArc
static void
DrawArc(const UnitVector3D& a, const UnitVector3D& b, 
 GLUnurbsObj *renderer)
{
	using GPlatesMaths::sqrt;

	// The knot vector has (degree + length(ctrl_points) - 1)
	// elements.
	static const GLsizei KNOT_SIZE = 6;
	GLfloat knots[KNOT_SIZE] = {
		0.0, 0.0, 0.0, 1.0, 1.0, 1.0
	};

	// FIXME: There should be a way to avoid copying these values.
	GLfloat ctrl_points[3][4] = {
		{ a.x().dval(), a.y().dval(), a.z().dval(), 1.0 },
		{ 0.0, 0.0, 0.0, 0.0 },  // Dummy value, still to be calculated.
		{ b.x().dval(), b.y().dval(), b.z().dval(), 1.0 }
	};

	static const double ONE_DIV_ROOT_TWO = 1.0 / std::sqrt(2.0);

	// Calculate the weight of the control point.
	ctrl_points[1][3] = 
		static_cast<GLfloat>((ONE_DIV_ROOT_TWO * sqrt(1.0 + a*b)).dval());

	// Calculate the control point itself.
	// 1) a + b
	ctrl_points[1][0] = a.x().dval() + b.x().dval();
	ctrl_points[1][1] = a.y().dval() + b.y().dval();
	ctrl_points[1][2] = a.z().dval() + b.z().dval();
	// 2) scale = 1 / size
	GLfloat ctrlptscale = static_cast<GLfloat>(
		1.0 / std::sqrt(ctrl_points[1][0]*ctrl_points[1][0] +
		 ctrl_points[1][1]*ctrl_points[1][1] + 
		 ctrl_points[1][2]*ctrl_points[1][2])
	);
	ctrl_points[1][0] *= ctrlptscale;
	ctrl_points[1][1] *= ctrlptscale;
	ctrl_points[1][2] *= ctrlptscale;

	// FIXME: Change the colour depending on the line.
	glColor3fv(Colour::RED);
	
	// Draw the NURBS.
	gluBeginCurve(renderer);
		gluNurbsCurve(renderer, KNOT_SIZE, &knots[0], 4, 
		 &ctrl_points[0][0], 3, GL_MAP1_VERTEX_4);
	gluEndCurve(renderer);
}


void
Globe::Paint()
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
		gluSphere(_sphere, _radius, _slices, _stacks);

		// XXX: Draw NURBS (temporary measure)
		DrawArc(UnitVector3D(1.0, 0.0, 0.0), UnitVector3D(0.0, 0.0, 1.0), _nurbs_renderer);
		DrawArc(UnitVector3D(1.0, 0.0, 0.0), UnitVector3D(0.0, 1.0, 0.0), _nurbs_renderer);
		real_t tmp = real_t(1.0) / sqrt(real_t(2.0));
		DrawArc(UnitVector3D(tmp, tmp, 0.0), UnitVector3D(0.0, tmp, tmp), _nurbs_renderer);
		
	glPopMatrix();
}
