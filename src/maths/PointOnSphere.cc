/* $Id$ */

/**
 * @file 
 * Implementation of PointOnSphere.  This code was originally
 * in maths/OperationsOnSphere.cc:convertPointOnSphereToUnitVector.
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#include "PointOnSphere.h"

using namespace GPlatesMaths;


static UnitVector3D
convertLatLongToUnitVector(const real_t& lat, const real_t& lon) {

	real_t lat_angle = degreesToRadians(lat);
	real_t long_angle = degreesToRadians(lon);
	
	real_t radius_of_small_circle_of_latitude = cos(lat_angle);

	real_t x_comp = radius_of_small_circle_of_latitude * cos(long_angle);
	real_t y_comp = radius_of_small_circle_of_latitude * sin(long_angle);
	real_t z_comp = sin(lat_angle);  // height above equator

	return UnitVector3D(x_comp, y_comp, z_comp);
}


PointOnSphere::PointOnSphere(const real_t& lat, const real_t& lon)
	: _uv(convertLatLongToUnitVector(lat, lon))
{  }

#if 0
real_t
PointOnSphere::GetLatitude() const
{
	return 0.0;
}


real_t
PointOnSphere::GetLongitude() const
{
	return 0.0;
}
#endif
