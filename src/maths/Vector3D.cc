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

#include "IndeterminateResultException.h"
#include "Vector3D.h"
#include "UnitVector3D.h"


GPlatesMaths::UnitVector3D
GPlatesMaths::Vector3D::get_normalisation() const
{
	real_t mag_sqrd = (_x * _x) + (_y * _y) + (_z * _z);

	if (mag_sqrd <= 0.0)
		throw IndeterminateResultException
				("Can't normalise zero vectors!");
	real_t scale = 1 / sqrt (mag_sqrd);
	return UnitVector3D (_x * scale, _y * scale, _z * scale);
}


GPlatesMaths::Vector3D
GPlatesMaths::cross(const Vector3D &v1, const Vector3D &v2)
{
	real_t x_comp = v1.y () * v2.z () - v1.z () * v2.y ();
	real_t y_comp = v1.z () * v2.x () - v1.x () * v2.z ();
	real_t z_comp = v1.x () * v2.y () - v1.y () * v2.x ();

	return Vector3D (x_comp, y_comp, z_comp);
}
