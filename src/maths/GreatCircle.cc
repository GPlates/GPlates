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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include <sstream>
#include "GreatCircle.h"
#include "IndeterminateResultException.h"


GPlatesMaths::GreatCircle::GreatCircle (const PointOnSphere &p1,
 const PointOnSphere &p2)
 : _normal (calcNormal(p1.unitvector(), p2.unitvector())) {  }


GPlatesMaths::UnitVector3D
GPlatesMaths::GreatCircle::intersection (const GreatCircle &other) const
{
	Vector3D x = cross(normal(), other.normal());

	real_t x_mag_sqrd = x.magSqrd();
	if (x_mag_sqrd <= 0) {

		// mag_sqrd equal to zero => magnitude is equal to zero =>
		// collinear vectors => PANIC!!!!one
		std::ostringstream oss;
		oss << "Attempted to calculate the intersection of "
		 << "equivalent great-circles.";
		throw IndeterminateResultException (oss.str().c_str());
	}
	return x.normalise();
}


GPlatesMaths::UnitVector3D
GPlatesMaths::GreatCircle::calcNormal(const UnitVector3D &v1,
 const UnitVector3D &v2) {

	Vector3D x = cross(v1, v2);

	real_t x_mag_sqrd = x.magSqrd();
	if (x_mag_sqrd <= 0) {

		// mag_sqrd equal to zero => magnitude is equal to zero =>
		// collinear vectors => PANIC!!!!one
		std::ostringstream oss;
		oss << "Attempted to calculate a great-circle from "
		 << "collinear points "
		 << v1 << " and " << v2 << ".";
		throw IndeterminateResultException (oss.str().c_str());
	}
	return x.normalise();
}
