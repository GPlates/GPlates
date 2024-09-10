/**
 * Copyright (C) 2024 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_PLATE_BOUNDARY_STATS_H
#define GPLATES_APP_LOGIC_PLATE_BOUNDARY_STATS_H

#include <map>
#include <vector>

#include "ResolvedTopologicalSection.h"
#include "ResolvedTopologicalSharedSubSegment.h"

#include "VelocityDeltaTime.h"
#include "VelocityUnits.h"

#include "maths/Vector3D.h"


namespace GPlatesAppLogic
{
	/**
	 * Statistics at a point location on a plate boundary.
	 */
	class PlateBoundaryStat
	{
	public:
		PlateBoundaryStat(
				const GPlatesMaths::PointOnSphere &point_,
				const GPlatesMaths::Vector3D &absolute_velocity_) :
			point(point_),
			absolute_velocity(absolute_velocity_)
		{  }

		//! Point location on a plate boundary.
		GPlatesMaths::PointOnSphere point;
		//! Velocity of the plate boundary itself (at the point location).
		GPlatesMaths::Vector3D absolute_velocity;
	};

	/**
	 * Calculates statistics along the plate boundaries specified by the resolved topological sections
	 * (each is a list of sub-segments shared by resolved topology boundaries, optionally including networks).
	 *
	 * Generates a sequence of uniformly-spaced points along each resolved topological section and
	 * calculates plate boundary statistics at each point.
	 *
	 * For each resolved topological section, its first point is located @a first_uniform_point_spacing radians
	 * from its first vertex and each subsequent point is separated by @a uniform_point_spacing radians.
	 * Since a resolved topological section contains one or more shared sub-segments, the uniform spacing
	 * is continuous across boundaries between shared sub-segments (unless there's a gap between them,
	 * in which case the spacing is reset to @a first_uniform_point_spacing for the next shared sub-segment).
	 *
	 * Returns a mapping of shared sub-segments to their plate boundary statistics (at uniform points along them).
	 */
	void
	calculate_plate_boundary_stats(
			std::map<ResolvedTopologicalSharedSubSegment::non_null_ptr_type, std::vector<PlateBoundaryStat>> &plate_boundary_stats,
			const std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
			const double &reconstruction_time,
			const double &uniform_point_spacing,
			const double &first_uniform_point_spacing,
			const double &velocity_delta_time,
			VelocityDeltaTime::Type velocity_delta_time_type,
			VelocityUnits::Value velocity_units,
			const double &earth_radius_in_kms);
}

#endif // GPLATES_APP_LOGIC_PLATE_BOUNDARY_STATS_H
