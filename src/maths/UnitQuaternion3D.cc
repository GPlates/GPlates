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
#include "IndeterminateResultException.h"
#include "ViolatedUnitQuatInvariantException.h"


GPlatesMaths::UnitQuaternion3D::RotationParams
GPlatesMaths::UnitQuaternion3D::calcRotationParams() const {

	/*
	 * Ensure that the quaternion does not represent an identity rotation.
	 *
	 * In an identity rotation, the angle of rotation is (2 * n * pi),
	 * for some integer 'n':  this would later result in an evaluation of
	 * the sine of some (n * pi), which is always zero.  This, in turn,
	 * would result in a division by zero when attempting to calculate
	 * the rotation axis, which is geometrically equivalent to the fact
	 * that, in an identity rotation, the axis is indeterminate.
	 */
	if (isIdentity()) {

		std::ostringstream oss;
		oss << "Attempted to calculate the rotation parameters\n"
		 << "of a quaternion which represents the identity rotation:\n"
		 << (*this);
		throw IndeterminateResultException(oss.str().c_str());
	}

	/*
	 * We can now be sure that the angle of rotation ('theta') is not a
	 * multiple of two pi, and the axis of rotation is clearly determined.
	 */
	real_t theta_on_2 = acos(s());  // theta_on_2 is not a multiple of pi
	real_t sin_of_theta_on_2 = sin(theta_on_2);  // not zero

	Vector3D axis_v = (1 / sin_of_theta_on_2) * v();
	UnitVector3D axis_uv = axis_v.normalise();

	return RotationParams(axis_uv, theta_on_2 * 2);
}


GPlatesMaths::UnitQuaternion3D
GPlatesMaths::UnitQuaternion3D::CreateEulerRotation(const UnitVector3D &axis,
	const real_t &angle) {

	real_t theta_on_two = angle / 2.0;

	real_t   scalar_part = cos(theta_on_two);
	Vector3D vector_part = sin(theta_on_two) * Vector3D(axis);

	return UnitQuaternion3D(scalar_part, vector_part);
}


void
GPlatesMaths::UnitQuaternion3D::AssertInvariantHolds() const {

	/*
	 * Calculate magnitude of quaternion to ensure that it actually _is_ 1.
	 * For efficiency, don't bother sqrting yet.
	 */
	real_t mag_sqrd = (_s * _s) + dot(_v, _v);
	if (mag_sqrd != 1.0) {

		// invariant has been violated
		std::ostringstream oss;
		oss << "UnitQuaternion3D has magnitude "
		 << sqrt(mag_sqrd) << ".";
		throw ViolatedUnitQuatInvariantException(oss.str().c_str());
	}
}


GPlatesMaths::UnitQuaternion3D
GPlatesMaths::operator*(UnitQuaternion3D q1, UnitQuaternion3D q2) {

	real_t   s1 = q1.s();
	real_t   s2 = q2.s();
	Vector3D v1 = q1.v();
	Vector3D v2 = q2.v();

	real_t   scalar_part = s1 * s2 - dot(v1, v2);
	Vector3D vector_part = s1 * v2 + s2 * v1 + cross(v1, v2);

	return UnitQuaternion3D(scalar_part, vector_part);
}
