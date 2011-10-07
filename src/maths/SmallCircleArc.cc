/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "SmallCircleArc.h"

#include "PointOnSphere.h"
#include "Rotation.h"
#include "UnitVector3D.h"
#include "ViolatedClassInvariantException.h"



void
GPlatesMaths::SmallCircleArc::AssertInvariantHolds() const
{
	if (d_angular_extent < 0 || d_angular_extent > 2 * PI)
	{
		// an invalid angular extent
		std::ostringstream oss;
		oss << "Small circle arc has angular extent '" << d_angular_extent.dval()
			<< "' outside the range [0,2*PI].";
		throw ViolatedClassInvariantException(GPLATES_EXCEPTION_SOURCE, oss.str().c_str());
	}
}


void
GPlatesMaths::tessellate(
		std::vector<PointOnSphere> &tessellation_points,
		const SmallCircleArc &small_circle_arc,
		const real_t &max_segment_angular_extent)
{
	// The angular extent of the small circle arc being subdivided.
	const double &angular_extent = small_circle_arc.angular_extent().dval();

	//
	// Note: Using static_cast<int> instead of static_cast<unsigned int> since
	// Visual Studio optimises for 'int' and not 'unsigned int'.
	//
	// The '+1' is to round up instead of down.
	// It also means we don't need to test for the case of only one segment.
	const int num_segments = 1 + static_cast<int>(angular_extent / max_segment_angular_extent.dval());
	const double segment_angular_extent = angular_extent / num_segments;

	// Create the rotation to generate segment points.
	const Rotation segment_rotation =
			Rotation::create(small_circle_arc.axis_vector(), segment_angular_extent);

	// Generate the segment points.
	const int num_initial_tessellation_points = tessellation_points.size();
	tessellation_points.reserve(num_initial_tessellation_points + num_segments + 1);
	tessellation_points.push_back(small_circle_arc.start_point());
	for (int n = 0; n < num_segments; ++n)
	{
		const PointOnSphere &segment_start_point = tessellation_points[num_initial_tessellation_points + n];
		const PointOnSphere segment_end_point(segment_rotation * segment_start_point.position_vector());

		tessellation_points.push_back(segment_end_point);
	}
}
