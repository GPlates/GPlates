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
#include "PointOnSphere.h"
#include "SmallCircle.h"
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

unsigned int GPlatesMaths::SmallCircle::intersection (const GreatCircle &other,
				std::vector<PointOnSphere> &points) const
{
	// If small circle and great circle are parallel, no intersections
	if (collinear (_normal, other.normal ()))
		return 0;

	// Since the axes are not collinear, the planes that the circles live
	// on definitely intersect, in the form of a line.

	// A is one point on the line through the intersection points, and
	// B is the direction vector, so the line equation is: x = A + Bt
	Vector3D B = cross (other.normal (), _normal);
	real_t scale = cos (_theta) / B.magSqrd ();
	Vector3D A = cross (B, other.normal ()) * scale;

	// solve a quadratic equation to get the actual points
	real_t a, b, c;
	a = B.magnitude ();
	b = 2 * dot (A, B);
	c = A.magnitude () - 1;
	real_t discr = b * b - 4 * a * c;
	if (discr < 0.0)
		return 0;	// no intersection
	if (discr <= 0.0) {
		// only one intersection point
		real_t t = -b / (2 * a);
		points.push_back (PointOnSphere (A + B * t));
		return 1;
	}

	// two intersection points
	real_t pm = sqrt (discr);
	real_t t1 = (-b - pm) / (2 * a), t2 = (-b + pm) / (2 * a);
	points.push_back (PointOnSphere (A + B * t1));
	points.push_back (PointOnSphere (A + B * t2));

	return 2;
}

void GPlatesMaths::SmallCircle::AssertInvariantHolds () const
{
	// TODO: make message more detailed
	if ((_theta < 0.0) || (_theta > M_PI))
		throw ViolatedSmallCircleInvariantException (
						"Theta out of range");
}
