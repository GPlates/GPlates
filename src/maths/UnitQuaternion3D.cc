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
#include "UnitQuaternion3D.h"
#include "ViolatedUnitQuatInvariantException.h"


using namespace GPlatesMaths;


UnitQuaternion3D
UnitQuaternion3D::CreateEulerRotation(const UnitVector3D &euler_pole,
	const real_t &rotation_angle) {

	real_t theta_on_two = rotation_angle / 2.0;

	real_t   scalar_part = cos(theta_on_two);
	Vector3D vector_part = sin(theta_on_two) * Vector3D(euler_pole);

	return UnitQuaternion3D(scalar_part, vector_part);
}


void
UnitQuaternion3D::AssertInvariantHolds() const {

	/*
	 * Calculate magnitude of quaternion to ensure that it actually _is_ 1.
	 * For efficiency, don't bother sqrting yet.
	 */
	real_t mag_sqrd = (_s * _s) + dot(_v, _v);
	if (mag_sqrd != 1.0) {

		// invariant has been violated
		std::ostringstream oss("UnitQuaternion3D has magnitude ");
		oss << sqrt(mag_sqrd);
		throw ViolatedUnitQuatInvariantException(oss.str().c_str());
	}
}


UnitQuaternion3D
GPlatesMaths::operator*(UnitQuaternion3D q1, UnitQuaternion3D q2) {

	real_t   s1 = q1.s();
	real_t   s2 = q2.s();
	Vector3D v1 = q1.v();
	Vector3D v2 = q2.v();

	real_t   scalar_part = s1 * s2 - dot(v1, v2);
	Vector3D vector_part = s1 * v2 + s2 * v1 + cross(v1, v2);

	return UnitQuaternion3D(scalar_part, vector_part);
}
