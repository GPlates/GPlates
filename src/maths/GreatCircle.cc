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

#include "GreatCircle.h"
#include "IndeterminateResultException.h"
#include "Rotation.h"


GPlatesMaths::GreatCircle::GreatCircle (const PointOnSphere &p1,
 const PointOnSphere &p2)
 : d_axis (calc_normal(p1.position_vector(), p2.position_vector())) {  }


#if 0
GPlatesMaths::UnitVector3D
GPlatesMaths::GreatCircle::intersection (const GreatCircle &other) const
{
	Vector3D v = cross(normal(), other.normal());

	real_t v_mag_sqrd = v.magSqrd();
	if (v_mag_sqrd <= 0) {

		// mag_sqrd equal to zero => magnitude is equal to zero =>
		// collinear vectors => PANIC!!!!one
		std::ostringstream oss;
		oss << "Attempted to calculate the intersection of "
		 << "equivalent great-circles.";
		throw IndeterminateResultException (oss.str().c_str());
	}
	return v.get_normalisation();
}
#endif


GPlatesMaths::UnitVector3D
GPlatesMaths::GreatCircle::calc_normal(const UnitVector3D &u1,
 const UnitVector3D &u2) {

	Vector3D v = cross(u1, u2);

	real_t v_mag_sqrd = v.magSqrd();
	if (v_mag_sqrd <= 0) {

		// mag_sqrd equal to zero => magnitude is equal to zero =>
		// collinear vectors => PANIC!!!!one
		std::ostringstream oss;
		oss << "Attempted to calculate a great-circle from "
		 << "collinear points "
		 << u1 << " and " << u2 << ".";
		throw IndeterminateResultException (GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}
	return v.get_normalisation();
}


void
GPlatesMaths::tessellate(
		std::vector<PointOnSphere> &tessellation_points,
		const GreatCircle &great_circle,
		const real_t &max_segment_angular_extent)
{
	//
	// Note: Using static_cast<int> instead of static_cast<unsigned int> since
	// Visual Studio optimises for 'int' and not 'unsigned int'.
	//
	// The '+1' is to round up instead of down.
	// It also means we don't need to test for the case of only one segment.
	const int num_segments = 1 + static_cast<int>(2 * PI / max_segment_angular_extent.dval());
	const double segment_angular_extent = 2 * PI / num_segments;

	// Create the rotation to generate segment points.
	const Rotation segment_rotation =
			Rotation::create(great_circle.axis_vector(), segment_angular_extent);

	// Generate the segment points.
	const int num_initial_tessellation_points = tessellation_points.size();
	tessellation_points.reserve(num_initial_tessellation_points + num_segments + 1);
	// Generate the first point on the great circle - it could be anywhere along the great circle
	// so generate a point perpendicular to the great circle's rotation axis.
	const PointOnSphere start_point(generate_perpendicular(great_circle.axis_vector()));
	tessellation_points.push_back(start_point);
	for (int n = 0; n < num_segments - 1; ++n)
	{
		const PointOnSphere &segment_start_point = tessellation_points[num_initial_tessellation_points + n];
		const PointOnSphere segment_end_point(segment_rotation * segment_start_point.position_vector());

		tessellation_points.push_back(segment_end_point);
	}

	// The final point is the same as the initial point.
	// It is implicit - we don't actually add it.
	// If the caller needs a closed loop they can close it explicitly.
}
