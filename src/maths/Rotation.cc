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
