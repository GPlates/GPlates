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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include "GreatCircle.h"
#include "GridOnSphere.h"
#include "OperationsOnSphere.h"
#include "PointOnSphere.h"
#include "SmallCircle.h"
#include "UnitVector3D.h"
#include "ViolatedClassInvariantException.h"


using GPlatesMaths::OperationsOnSphere::convertPointOnSphereToLatLonPoint;


GPlatesMaths::GridOnSphere::GridOnSphere (const PointOnSphere &origin,
				const PointOnSphere &next_along_lat,
				const PointOnSphere &next_along_lon)
	: _line_of_lat (GPlatesMaths::NorthPole.unitvector (), next_along_lat),
	  _line_of_lon (origin, next_along_lon),
	  _origin (origin)
{
	LatLonPoint org = convertPointOnSphereToLatLonPoint (_origin);
	LatLonPoint lat = convertPointOnSphereToLatLonPoint (next_along_lat);
	LatLonPoint lon = convertPointOnSphereToLatLonPoint (next_along_lon);
	_delta_along_lat = lat.longitude () - org.longitude ();
	_delta_along_lon = lon.latitude () - org.latitude ();

	AssertInvariantHolds ();
}


//GPlatesMaths::PointOnSphere
//GPlatesMaths::GridOnSphere::resolve (index_t x, index_t y) const
//{
	// TODO
//}

void GPlatesMaths::GridOnSphere::AssertInvariantHolds () const
{
	// Check that the origin lies on the SC
	LatLonPoint org = convertPointOnSphereToLatLonPoint (_origin);
	if (degreesToRadians (org.latitude () + 90.0) != _line_of_lat.theta ()) {
		throw ViolatedClassInvariantException (
			"Origin and next_along_lat have different latitudes.");
	}

	// Check that SC and GC are perpendicular
	if (!perpendicular (_line_of_lat.axisvector (),
			    _line_of_lon.axisvector ())) {
		throw ViolatedClassInvariantException (
			"SmallCircle and GreatCircle are not perpendicular.");
	}
}
