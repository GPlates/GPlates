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

#include "FiniteRotation.h"
#include "Vector3D.h"
#include "InvalidOperationException.h"


using namespace GPlatesMaths;


FiniteRotation
FiniteRotation::CreateFiniteRotation(const PointOnSphere &euler_pole,
	const real_t &rotation_angle,
	const real_t &point_in_time) {

	UnitVector3D rotation_axis = euler_pole.unitvector();
	UnitQuaternion3D uq =
	 UnitQuaternion3D::CreateEulerRotation(rotation_axis, rotation_angle);

	// values used to optimise rotation of points on the sphere
	real_t   d = (uq.s() * uq.s()) - dot(uq.v(), uq.v());
	Vector3D e = (2.0 * uq.s()) * uq.v();

	return FiniteRotation(uq, point_in_time, d, e);
}


FiniteRotation
FiniteRotation::CreateFiniteRotation(const UnitQuaternion3D &uq,
	const real_t &point_in_time) {

	// values used to optimise rotation of points on the sphere
	real_t   d = (uq.s() * uq.s()) - dot(uq.v(), uq.v());
	Vector3D e = (2.0 * uq.s()) * uq.v();

	return FiniteRotation(uq, point_in_time, d, e);
}


UnitVector3D
FiniteRotation::operator*(const UnitVector3D &uv) const {

	Vector3D v(uv);
	Vector3D v_rot = _d * v
	               + (2.0 * dot(_quat.v(), v)) * _quat.v()
	               + cross(_e, v);

	UnitVector3D uv_rot(v_rot.x(), v_rot.y(), v_rot.z());
}


FiniteRotation
operator*(const FiniteRotation &r1, const FiniteRotation &r2) {

	if (r1.time() != r2.time()) {

		// these FiniteRotations are not of the same point in time.
		std::ostringstream oss(
		 "Mismatched times in composition of FiniteRotations: ");
		oss << r1.time()
		 << " vs. "
		 << r2.time();

		throw InvalidOperationException(oss.str().c_str());
	}

	UnitQuaternion3D resultant_uq = r1.quat() * r2.quat();
	return FiniteRotation::CreateFiniteRotation(resultant_uq, r1.time());
}
