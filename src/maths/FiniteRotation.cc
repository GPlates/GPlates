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


GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::Create(const PointOnSphere &euler_pole,
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


GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::Create(const UnitQuaternion3D &uq,
	const real_t &point_in_time) {

	// values used to optimise rotation of points on the sphere
	real_t   d = (uq.s() * uq.s()) - dot(uq.v(), uq.v());
	Vector3D e = (2.0 * uq.s()) * uq.v();

	return FiniteRotation(uq, point_in_time, d, e);
}


GPlatesMaths::UnitVector3D
GPlatesMaths::FiniteRotation::operator*(const UnitVector3D &uv) const {

	Vector3D v(uv);

	/*
	 * On a Pentium IV processor, the first line should cost about
	 * (7 + 2 * 2) = 11 clock cycles;  the second should cost about
	 * (17 + 7 + (7 + 2 * 2)) = 35 cycles (assuming an inlined Vector3D
	 * dot-product is 17 cycles);  the third line should cost about 26
	 * clock cycles + the cost of a function call (assuming a Vector3D
	 * cross-product costs 26 cycles).
	 *
	 * Adding the lines should cost about (5 + 5 * 2) = 15 clock cycles
	 * -- it's Vector3D addition, remember -- which gives an approximate
	 * total cost of (11 + 35 + 26 + 15) = 87 clock cycles + the cost of
	 * a function call.
	 */
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


GPlatesMaths::FiniteRotation
GPlatesMaths::operator*(const FiniteRotation &r1, const FiniteRotation &r2) {

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
GPlatesMaths::operator*(const FiniteRotation &r, const PolyLineOnSphere &p) {

	PolyLineOnSphere rot_p;

	for (PolyLineOnSphere::const_iterator it = p.begin();
	     it != p.end();
	     it++) {

		rot_p.push_back(r * (*it));
	}
	return rot_p;
}


std::ostream &
GPlatesMaths::operator<<(std::ostream &os, const FiniteRotation &fr) {

	os << "(time = " << fr.time() << "; rot = ";

	UnitQuaternion3D uq = fr.quat();
	if (uq.isIdentity()) {

		os << "identity";

	} else {

		UnitQuaternion3D::RotationParams params =
		 uq.calcRotationParams();
		os << "(axis = " << params.axis
		 << "; angle = " << params.angle << ")";
	}
	os << ")";

	return os;
}
