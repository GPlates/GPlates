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

#include "FiniteRotation.h"
#include "Vector3D.h"


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


PointOnSphere
FiniteRotation::operator*(const PointOnSphere &p) const {

	Vector3D pv(p.unitvector());
	Vector3D pv_rot = _d * pv
	                + (2.0 * dot(_quat.v(), pv)) * _quat.v()
	                + cross(_e, pv);

	UnitVector3D puv_rot(pv_rot.x(), pv_rot.y(), pv_rot.z());
	return PointOnSphere(puv_rot);
}
