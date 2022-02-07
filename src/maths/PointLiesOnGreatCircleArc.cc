/* $Id$ */

/**
 * @file
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 *
 * Copyright (C) 2005, 2006, 2008 The University of Sydney, Australia
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

#include "PointLiesOnGreatCircleArc.h"

#include "Real.h"
#include "UnitVector3D.h"
#include "Vector3D.h"


bool
GPlatesMaths::PointLiesOnGreatCircleArc::operator()(
		const PointOnSphere &test_point) const
{
	// How we determine whether the point lies on the arc, will depend upon whether the arc has
	// zero length (ie, is pointlike) or not.
	if (!d_arc_normal)
	{
		// The arc has zero length, and hence, is pointlike.
		return (points_are_coincident(d_arc_start, test_point));
	}

	// The arc has non-zero length.

	const UnitVector3D &test_point_vector = test_point.position_vector();
	const UnitVector3D &arc_start_uv = d_arc_start.position_vector();
	const UnitVector3D &arc_end_uv = d_arc_end.position_vector();

	// See if the point lies within the lune of the great circle arc - the lune is the surface
	// of the globe in the wedge region of space formed by two planes (great circles) that
	// touch the arc's start and end points and are perpendicular to the arc.
	//
	// This happens if its endpoints are on opposite sides of the dividing plane *and*
	// the edge start point is on the positive side of the dividing plane.
	const Vector3D test_point_cross_arc_normal = cross(test_point_vector, d_arc_normal.get());
	if (dot(test_point_cross_arc_normal, arc_start_uv).dval() > 0 &&
		dot(test_point_cross_arc_normal, arc_end_uv).dval() < 0)
	{
		// The test point lies inside the arc's lune so it also lies on the arc if it's perpendicular
		// to the arc's normal.
		return perpendicular(test_point_vector, d_arc_normal.get());
	}

	// The test point lies outside the arc's lune so it cannot lie between the arc's end points.
	// So we just need to test closeness to the arc's end points.
	return points_are_coincident(d_arc_start, test_point) ||
			points_are_coincident(d_arc_end, test_point);
}
