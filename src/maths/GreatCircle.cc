/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#include <sstream>
#include "GreatCircle.h"
#include "IndeterminateResultException.h"


GPlatesMaths::GreatCircle::GreatCircle (const PointOnSphere &p1,
 const PointOnSphere &p2)
 : _axis (calcNormal(p1.position_vector(), p2.position_vector())) {  }


#if 0
GPlatesMaths::UnitVector3D
GPlatesMaths::GreatCircle::intersection (const GreatCircle &other) const
{
	Vector3D v = cross(normal(), other.normal());

	real_t v_mag_sqrd = v.magSqrd();
	if (v_mag_sqrd <= 0) {

		// mag_sqrd equal to zero => magnitude is equal to zero =>
		// collinear vectors => PANIC!!!!one
		std::ostringstream oss;
		oss << "Attempted to calculate the intersection of "
		 << "equivalent great-circles.";
		throw IndeterminateResultException (oss.str().c_str());
	}
	return v.get_normalisation();
}
#endif


GPlatesMaths::UnitVector3D
GPlatesMaths::GreatCircle::calcNormal(const UnitVector3D &u1,
 const UnitVector3D &u2) {

	Vector3D v = cross(u1, u2);

	real_t v_mag_sqrd = v.magSqrd();
	if (v_mag_sqrd <= 0) {

		// mag_sqrd equal to zero => magnitude is equal to zero =>
		// collinear vectors => PANIC!!!!one
		std::ostringstream oss;
		oss << "Attempted to calculate a great-circle from "
		 << "collinear points "
		 << u1 << " and " << u2 << ".";
		throw IndeterminateResultException (oss.str().c_str());
	}
	return v.get_normalisation();
}
