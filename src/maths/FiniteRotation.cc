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

#include "FiniteRotation.h"
#include "Vector3D.h"
#include "InvalidOperationException.h"


namespace {

	inline
	GPlatesMaths::real_t
	calculate_d_value(
	 const GPlatesMaths::UnitQuaternion3D &uq) {

		GPlatesMaths::real_t s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((s * s) - dot(v, v));
	}


	inline
	GPlatesMaths::Vector3D
	calculate_e_value(
	 const GPlatesMaths::UnitQuaternion3D &uq) {

		GPlatesMaths::real_t s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((2.0 * s) * v);
	}

}


GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::Create(
 const PointOnSphere &euler_pole,
 const real_t &rotation_angle,
 const real_t &point_in_time) {

	const UnitVector3D &rotation_axis = euler_pole.unitvector();
	UnitQuaternion3D uq =
	 UnitQuaternion3D::create_rotation(rotation_axis, rotation_angle);

	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return FiniteRotation(uq, point_in_time, d, e);
}


GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::Create(
 const UnitQuaternion3D &uq,
 const real_t &point_in_time) {

	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return FiniteRotation(uq, point_in_time, d, e);
}


GPlatesMaths::UnitVector3D
GPlatesMaths::FiniteRotation::operator*(
 const UnitVector3D &uv) const {

	Vector3D v(uv);

	Vector3D v_rot =
	 _d * v +
	 (2.0 * dot(_quat.vector_part(), v)) * _quat.vector_part() +
	 cross(_e, v);

	return UnitVector3D(v_rot.x(), v_rot.y(), v_rot.z());
}


GPlatesMaths::FiniteRotation
GPlatesMaths::operator*(
 const FiniteRotation &r1,
 const FiniteRotation &r2) {

	if (r1.time() != r2.time()) {

		// these FiniteRotations are not of the same point in time.
		std::ostringstream oss;
		oss << "Mismatched times in composition of FiniteRotations:\n"
		 << r1.time() << " vs. " << r2.time() << ".";

		throw InvalidOperationException(oss.str().c_str());
	}

	UnitQuaternion3D resultant_uq = r1.quat() * r2.quat();
	return FiniteRotation::Create(resultant_uq, r1.time());
}


GPlatesMaths::PolyLineOnSphere
GPlatesMaths::operator*(
 const FiniteRotation &r,
 const PolyLineOnSphere &p) {

	PolyLineOnSphere rot_p;

	for (PolyLineOnSphere::const_iterator it = p.begin();
	     it != p.end();
	     ++it) {

		rot_p.push_back(r * (*it));
	}
	return rot_p;
}


std::ostream &
GPlatesMaths::operator<<(
 std::ostream &os,
 const FiniteRotation &fr) {

	os << "(time = " << fr.time() << "; rot = ";

	UnitQuaternion3D uq = fr.quat();
	if (represents_identity_rotation(uq)) {

		os << "identity";

	} else {

		UnitQuaternion3D::RotationParams params =
		 uq.calc_rotation_params();
		os << "(axis = " << params.axis
		 << "; angle = " << params.angle << ")";
	}
	os << ")";

	return os;
}
