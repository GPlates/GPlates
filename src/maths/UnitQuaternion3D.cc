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

#include <iostream>
#include <sstream>
#include <cmath>

#include "UnitQuaternion3D.h"
#include "IndeterminateResultException.h"
#include "ViolatedClassInvariantException.h"


GPlatesMaths::UnitQuaternion3D::RotationParams
GPlatesMaths::UnitQuaternion3D::calc_rotation_params() const {

	/*
	 * Ensure that the quaternion does not represent an identity rotation.
	 *
	 * In an identity rotation, the angle of rotation is (2 * n * PI),
	 * for some integer 'n':  this would later result in an evaluation of
	 * the sine of some (n * PI), which is always zero.  This, in turn,
	 * would result in a division by zero when attempting to calculate
	 * the rotation axis, which is geometrically equivalent to the fact
	 * that, in an identity rotation, the axis is indeterminate.
	 */
	if (represents_identity_rotation(*this)) {

		std::ostringstream oss;

		oss
		 << "Attempted to calculate the rotation parameters\n"
		 << "of a quaternion which represents the identity rotation:\n"
		 << (*this);

		throw IndeterminateResultException(oss.str().c_str());
	}

	/*
	 * We can now be sure that the angle of rotation ('theta') is not a
	 * multiple of two PI, and the axis of rotation is clearly determined.
	 */
	real_t theta_on_2 = acos(scalar_part());  // not a multiple of PI
	real_t sin_of_theta_on_2 = sin(theta_on_2);  // not zero

	Vector3D axis_vect = (1 / sin_of_theta_on_2) * vector_part();
	UnitVector3D axis_unit_vect = axis_vect.normalise();

	return RotationParams(axis_unit_vect, theta_on_2 * 2.0);
}


GPlatesMaths::UnitQuaternion3D
GPlatesMaths::UnitQuaternion3D::create_rotation(
 const UnitVector3D &axis,
 real_t angle) {

	real_t theta_on_two = angle / 2.0;

	real_t   scalar_part = cos(theta_on_two);
	Vector3D vector_part = sin(theta_on_two) * Vector3D(axis);

	return UnitQuaternion3D(scalar_part, vector_part);
}


void
GPlatesMaths::UnitQuaternion3D::assert_invariant() const {

	/*
	 * FIXME: (1) implement loose/tight comparisons policy, with automatic
	 * self-correction in the case of natural drift.
	 *
	 * FIXME: (2) once (1) is implemented, invoke this in the ctor.
	 */

	/*
	 * Calculate norm of quaternion to ensure that it actually _is_ 1.
	 * For efficiency, don't bother sqrting yet.
	 */
	real_t norm_sqrd = actual_norm_sqrd();
	if (norm_sqrd != 1.0) {

		// invariant has been violated
		std::ostringstream oss;

		oss
		 << "UnitQuaternion3D has magnitude "
		 << sqrt(norm_sqrd)
		 << ".";

		throw ViolatedClassInvariantException(oss.str().c_str());
	}
}


GPlatesMaths::UnitQuaternion3D
GPlatesMaths::operator*(
 const UnitQuaternion3D &q1,
 const UnitQuaternion3D &q2) {

	real_t s1 = q1.scalar_part();
	real_t s2 = q2.scalar_part();

	const Vector3D &v1 = q1.vector_part();
	const Vector3D &v2 = q2.vector_part();

	real_t   res_scalar_part = s1 * s2 - dot(v1, v2);
	Vector3D res_vector_part = s1 * v2 + s2 * v1 + cross(v1, v2);

	return UnitQuaternion3D(res_scalar_part, res_vector_part);
}


std::ostream &
GPlatesMaths::operator<<(
 std::ostream &os,
 const UnitQuaternion3D &q) {

	os
	 << "("
	 << q.w() << ", "
	 << q.x() << ", "
	 << q.y() << ", "
	 << q.z() << ")";

	return os;
}
