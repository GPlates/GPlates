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

#include "OpaqueSphere.h"

namespace
{
	static const GLdouble RADIUS = 1.0;

	static const GLint NUM_SLICES = 36;
	static const GLint NUM_STACKS = 18;
}


GPlatesGui::OpaqueSphere::OpaqueSphere(const Colour &colour)
 : _colour(colour) {

	_quad.setNormals(GLU_SMOOTH);
	_quad.setOrientation(GLU_OUTSIDE);
	_quad.setGenerateTexture(GL_FALSE);
	_quad.setDrawStyle(GLU_FILL);
}


void
GPlatesGui::OpaqueSphere::Paint() {

	glColor3fv(_colour);
	_quad.drawSphere(RADIUS, NUM_SLICES, NUM_STACKS);
}
