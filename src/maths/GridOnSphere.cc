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

#include <sstream>
#include <vector>
#include "GreatCircle.h"
#include "IndeterminateResultException.h"
#include "GridOnSphere.h"
#include "OperationsOnSphere.h"
#include "PointOnSphere.h"
#include "SmallCircle.h"
#include "UnitVector3D.h"


GPlatesMaths::GridOnSphere::GridOnSphere (const PointOnSphere &origin,
				const PointOnSphere &next_along_lon,
				const PointOnSphere &next_along_lat)
	: _line_of_lon (origin, next_along_lon),
	  _line_of_lat (GPlatesMaths::NorthPole.unitvector (), next_along_lat),
	  _origin (origin)
{
	// TODO: finish me
	//	- need to check that GC and SC are perpendicular,
	//	  that the origin lies on the SC, and ... ?
#if 0
	UnitVector3D point = pt.unitvector ();
	real_t dp = dot (normal(), point);

	if (abs (dp) > 1.0)
		throw IndeterminateResultException (
			"Tried to create small circle with imaginary radius.");

	_theta = acos (dp);
#endif
}


//GPlatesMaths::PointOnSphere
//GPlatesMaths::GridOnSphere::resolve (index_t x, index_t y) const
//{
	// TODO
//}

void GPlatesMaths::GridOnSphere::AssertInvariantHolds () const
{
	// TODO
}
