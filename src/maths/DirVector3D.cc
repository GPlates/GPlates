/* $Id$ */

/**
 * \file 
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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <sstream>
#include "DirVector3D.h"
#include "UnitVector3D.h"


using namespace GPlatesMaths;

void
DirVector3D::AssertInvariantHolds() const {

	if (_mag == 0.0) {

		// invariant has been violated
		std::ostringstream oss("DirVector3D has magnitude ");
		oss << _mag;
		throw ViolatedDirVectorInvariantException(oss.str().c_str());
	}
}


UnitVector3D
DirVector3D::normalise() const {

	real_t x_comp = _x / _mag;
	real_t y_comp = _y / _mag;
	real_t z_comp = _z / _mag;

	return UnitVector3D(x_comp, y_comp, z_comp);
}


DirVector3D
GPlatesMaths::cross(DirVector3D v1, DirVector3D v2) {

	real_t x_comp = v1.y() * v2.z() - v1.z() * v2.y();
	real_t y_comp = v1.z() * v2.x() - v1.x() * v2.z();
	real_t z_comp = v1.x() * v2.y() - v1.y() * v2.x();

	return DirVector3D(x_comp, y_comp, z_comp);
}
