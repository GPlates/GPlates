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

using namespace GPlatesMaths;


FiniteRotation
FiniteRotation::CreateFiniteRotation(const PointOnSphere &euler_pole,
	const real_t &rotation_angle,
	const real_t &point_in_time) {

	const UnitVector3D &rotation_axis = euler_pole.unitvector();
	const UnitQuaternion3D &uq =
	 UnitQuaternion3D::CreateEulerRotation(rotation_axis, rotation_angle);

	return FiniteRotation(uq, point_in_time);
}

