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
#include "Rotation.h"
#include "SmallCircle.h"
#include "UnitVector3D.h"
#include "ViolatedClassInvariantException.h"


void
GPlatesMaths::SmallCircle::AssertInvariantHolds() const
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


void
GPlatesMaths::tessellate(
		std::vector<PointOnSphere> &tessellation_points,
		const SmallCircle &small_circle,
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
			Rotation::create(small_circle.axis_vector(), segment_angular_extent);

	// Generate the segment points.
	const int num_initial_tessellation_points = tessellation_points.size();
	tessellation_points.reserve(num_initial_tessellation_points + num_segments + 1);

	// Generate the first point on the small circle - it could be anywhere along the small circle
	// so generate a vector perpendicular to its rotation axis and use that, in turn, to rotate
	// the axis point (on sphere) to a point on the small circle.
	const UnitVector3D start_point_rotation_axis = generate_perpendicular(small_circle.axis_vector());
	const Rotation start_point_rotation = Rotation::create(start_point_rotation_axis, small_circle.colatitude());
	const PointOnSphere start_point(start_point_rotation * small_circle.axis_vector());
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
