/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sstream>
#include <cmath>

#include "Rotation.h"
#include "Vector3D.h"
#include "InvalidOperationException.h"


namespace {

	inline
	GPlatesMaths::real_t
	calculate_d_value(
	 const GPlatesMaths::UnitQuaternion3D &uq) {

		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((s * s) - dot(v, v));
	}


	inline
	GPlatesMaths::Vector3D
	calculate_e_value(
	 const GPlatesMaths::UnitQuaternion3D &uq) {

		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((2.0 * s) * v);
	}

}


GPlatesMaths::Rotation
GPlatesMaths::Rotation::Create(
 const UnitVector3D &rotation_axis,
 const real_t &rotation_angle) {

	UnitQuaternion3D uq =
	 UnitQuaternion3D::create_rotation(rotation_axis, rotation_angle);

	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return Rotation(rotation_axis, rotation_angle, uq, d, e);
}


GPlatesMaths::Rotation
GPlatesMaths::Rotation::Create(
 const UnitVector3D &initial,
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
			UnitVector3D axis = generate_perpendicular(initial);
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
		UnitVector3D axis = cross(initial, final).get_normalisation();
		real_t angle = acos(dp);
		return Rotation::Create(axis, angle);
	}
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Rotation::operator*(const UnitVector3D &uv) const {

	Vector3D v(uv);

	Vector3D v_rot =
	 _d * v +
	 (2.0 * dot(_quat.vector_part(), v)) * _quat.vector_part() +
	 cross(_e, v);

	return UnitVector3D(v_rot.x(), v_rot.y(), v_rot.z());
}


GPlatesMaths::Rotation
GPlatesMaths::Rotation::Create(
 const UnitQuaternion3D &uq,
 const UnitVector3D &rotation_axis,
 const real_t &rotation_angle) {

	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return Rotation(rotation_axis, rotation_angle, uq, d, e);
}


GPlatesMaths::Rotation
GPlatesMaths::operator*(const Rotation &r1, const Rotation &r2) {

	UnitQuaternion3D resultant_uq = r1.quat() * r2.quat();
	if (represents_identity_rotation(resultant_uq)) {

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
		 resultant_uq.get_rotation_params();

		return
		 Rotation::Create(resultant_uq, params.axis, params.angle);
	}
}
