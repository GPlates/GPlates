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
#include <cmath>

#include "Rotation.h"
#include "Vector3D.h"
#include "InvalidOperationException.h"


GPlatesMaths::Rotation
GPlatesMaths::Rotation::Create(const UnitVector3D &rotation_axis,
	const real_t &rotation_angle) {

	UnitQuaternion3D uq =
	 UnitQuaternion3D::CreateEulerRotation(rotation_axis, rotation_angle);

	// values used to optimise rotation of points on the sphere
	real_t   d = (uq.s() * uq.s()) - dot(uq.v(), uq.v());
	Vector3D e = (2.0 * uq.s()) * uq.v();

	return Rotation(rotation_axis, rotation_angle, uq, d, e);
}


GPlatesMaths::Rotation
GPlatesMaths::Rotation::Create(const UnitVector3D &initial,
	const UnitVector3D &final) {

	real_t dp = dot(initial, final);
	if (abs(dp) >= 1.0) {

		/*
		 * The unit-vectors 'initial' and 'final' are collinear.
		 * This means they do not define a unique plane, which means
		 * it is not possible to determine a unique axis of rotation.
		 * We will need to pick some arbitrary values out of the air.
		 */
		if (dp >= 1.0) {

			/*
			 * The unit-vectors are parallel.
			 * Hence, the rotation which transforms 'initial'
			 * into 'final' is the identity rotation.
			 * 
			 * The identity rotation is represented by a rotation
			 * of zero radians about any arbitrary axis. 
			 * 
			 * For simplicity, let's use 'initial' as the axis.
			 */
			return Rotation::Create(initial, 0.0);

		} else {

			/*
			 * The unit-vectors are anti-parallel.
			 * Hence, the rotation which transforms 'initial'
			 * into 'final' is the reflection rotation.
			 *
			 * The reflection rotation is represented by a rotation
			 * of PI radians about any arbitrary axis orthogonal to
			 * 'initial'.
			 */
			UnitVector3D axis = generatePerpendicular(initial);
			return Rotation::Create(axis, PI);
		}

	} else {

		/*
		 * The unit-vectors 'initial' and 'final' are NOT collinear.
		 * This means they DO define a unique plane, with a unique
		 * axis about which 'initial' may be rotated to become 'final'.
		 *
		 * Since the unit-vectors are not collinear, the result of
		 * their cross-product will be a vector of non-zero length,
		 * which we can safely normalise.
		 */
		UnitVector3D axis = cross(initial, final).normalise();
		real_t angle = acos(dp);
		return Rotation::Create(axis, angle);
	}
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Rotation::operator*(const UnitVector3D &uv) const {

	Vector3D v(uv);

	Vector3D v_rot = _d * v
	               + (2.0 * dot(_quat.v(), v)) * _quat.v()
	               + cross(_e, v);

	/*
	 * Assuming that these components do, in fact, represent a unit vector
	 * (thus avoiding any exception-throwing craziness), the cost of the
	 * creation of this unit vector should be about 26 clock cycles +
	 * the cost of a function call.
	 */
	return UnitVector3D(v_rot.x(), v_rot.y(), v_rot.z());
}


GPlatesMaths::Rotation
GPlatesMaths::Rotation::Create(const UnitQuaternion3D &uq,
	const UnitVector3D &rotation_axis,
	const real_t &rotation_angle) {

	// values used to optimise rotation of points on the sphere
	real_t   d = (uq.s() * uq.s()) - dot(uq.v(), uq.v());
	Vector3D e = (2.0 * uq.s()) * uq.v();

	return Rotation(rotation_axis, rotation_angle, uq, d, e);
}


GPlatesMaths::Rotation
GPlatesMaths::operator*(const Rotation &r1, const Rotation &r2) {

	UnitQuaternion3D resultant_uq = r1.quat() * r2.quat();
	if (resultant_uq.isIdentity()) {

		/*
		 * The identity rotation is represented by a rotation of
		 * zero radians about any arbitrary axis.
		 *
		 * For simplicity, let's use the axis of 'r1' as the axis.
		 */
		return Rotation::Create(resultant_uq, r1.axis(), 0.0);

	} else {

		/*
		 * The resultant quaternion has a clearly-defined axis
		 * and a non-zero angle of rotation.
		 */
		UnitQuaternion3D::RotationParams params =
		 resultant_uq.calcRotationParams();

		return
		 Rotation::Create(resultant_uq, params.axis, params.angle);
	}
}
