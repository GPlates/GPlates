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

#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "Globe.h"
#include "maths/types.h"
#include "maths/GreatCircleArc.h"
#include "state/Layout.h"

using namespace GPlatesGui;
using namespace GPlatesMaths;
using namespace GPlatesState;

const GLfloat Globe::DEFAULT_RADIUS = 1.0;
const Colour Globe::DEFAULT_COLOUR = Colour::WHITE;

static void
QuadricError(GLenum error)
{
	std::cerr << "Quadric Error: " << gluErrorString(error) << std::endl;
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

static void
CallVertexWithPoint(const UnitVector3D& uv)
{
	glVertex3d(
		uv.x().dval(),
		uv.y().dval(),
		uv.z().dval()
	);
}

static void
CallVertexWithLine(const PolyLineOnSphere::const_iterator& begin, 
				   const PolyLineOnSphere::const_iterator& end)
{
	PolyLineOnSphere::const_iterator iter = begin;

	glBegin(GL_LINE_STRIP);
		CallVertexWithPoint(iter->startPoint());
		for ( ; iter != end; ++iter)
			CallVertexWithPoint(iter->endPoint());
	glEnd();
}

static void
PaintPointDataPos(const Layout::PointDataPos& pointdata)
{
	const PointOnSphere& point = pointdata.second;
	CallVertexWithPoint(point.unitvector());
}

static void
PaintLineDataPos(const Layout::LineDataPos& linedata)
{
	const PolyLineOnSphere& line = linedata.second;

	CallVertexWithLine(line.begin(), line.end());
}

static void
PaintPoints()
{
	Layout::PointDataLayout::const_iterator 
		points_begin = Layout::PointDataLayoutBegin(),
		points_end   = Layout::PointDataLayoutEnd();

	glBegin(GL_POINTS);
		for_each(points_begin, points_end, PaintPointDataPos);
	glEnd();
}


static void
PaintLines()
{
	Layout::LineDataLayout::const_iterator 
		lines_begin = Layout::LineDataLayoutBegin(),
		lines_end   = Layout::LineDataLayoutEnd();
	
	for_each(lines_begin, lines_end, PaintLineDataPos);
}

void
Globe::Paint()
{
	// NOTE: OpenGL rotations are *counter-clockwise* (API v1.4, p35).
	glPushMatrix();
		// Ensure that the meridian and elevation are in the acceptable 
		// range.
		NormaliseMeridianElevation();

		// rotate everything to get a nice almost-equatorial shot
//		glRotatef(-80.0, 1.0, 0.0, 0.0);

		// rotate everything (around x) according to elevation rotation
		// FIXME: This should be combined with the previous rotation so
		//  only one is submitted.
		glRotatef(_elevation, 0.0, 1.0, 0.0);

		// rotate everything (around z) according to meridian rotation
		glRotatef(_meridian, 0.0, 0.0, 1.0);

		// Set the globe's colour.
		glColor3fv(_colour);
		
		// Draw globe.
		// DepthRange calls push the globe back in the depth buffer a bit to
		// avoid Z-fighting with the NURBS.
		glDepthRange(0.1, 1.0);
		gluSphere(_sphere, _radius, _slices, _stacks);
		glDepthRange(0.0, 0.9);

		glPointSize(5.0f);
		
		/* 
		 * Paint the data.
		 */
		glColor3fv(Colour::GREEN);
		PaintPoints();  // Points are yellow.
		
		glColor3fv(Colour::RED);
		PaintLines();  // lines are red.

	glPopMatrix();
}
