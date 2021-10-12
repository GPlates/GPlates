/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sstream>
#include <vector>
#include "GreatCircle.h"
#include "PointOnSphere.h"
#include "SmallCircle.h"
#include "UnitVector3D.h"
#include "ViolatedClassInvariantException.h"


unsigned int GPlatesMaths::SmallCircle::intersection (const GreatCircle &other,
				std::vector<PointOnSphere> &points) const
{
	// If small circle and great circle are parallel, no intersections
	if (collinear (d_axis, other.axis_vector ()))
		return 0;

	// Since the axes are not collinear, the planes that the circles live
	// on definitely intersect, in the form of a line.

	// A is one point on the line through the intersection points, and
	// B is the direction vector, so the line equation is: x = A + Bt
	Vector3D B = cross (other.axis_vector (), d_axis);
	real_t scale = d_cos_colat / B.magSqrd ();
	Vector3D A = cross (B, other.axis_vector ()) * scale;

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
		UnitVector3D u(A + B * t);
		points.push_back (PointOnSphere (u));
		return 1;
	}

	// two intersection points
	real_t pm = sqrt (discr);
	real_t t1 = (-b - pm) / (2 * a), t2 = (-b + pm) / (2 * a);
	UnitVector3D u1(A + B * t1);
	points.push_back (PointOnSphere (u1));
	UnitVector3D u2(A + B * t2);
	points.push_back (PointOnSphere (u2));

	return 2;
}


void
GPlatesMaths::SmallCircle::AssertInvariantHolds () const
{
	if (abs (d_cos_colat) > 1.0) {

		// an invalid cos(colatitude)
		std::ostringstream oss;
		oss << "Small circle has invalid cos(colatitude) of "
		 << d_cos_colat << ".";
		throw ViolatedClassInvariantException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}
}
