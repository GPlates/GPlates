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
#include "SmallCircle.h"
#include "IndeterminateResultException.h"
#include "PointOnSphere.h"
#include "UnitVector3D.h"
#include "ViolatedSmallCircleInvariantException.h"


GPlatesMaths::SmallCircle::SmallCircle (const UnitVector3D &axis,
					const PointOnSphere &pt)
	: _normal (axis)
{
	UnitVector3D point = pt.unitvector ();
	real_t dp = dot (_normal, point);

	if (abs (dp) >= 1.0)
		throw IndeterminateResultException (
				"Tried to create degenerate small circle.");

	_theta = acos (dp);
}

void GPlatesMaths::SmallCircle::AssertInvariantHolds () const
{
	// TODO: make message more detailed
	if ((_theta < 0.0) || (_theta > M_PI))
		throw ViolatedSmallCircleInvariantException (
						"Theta out of range");
}
