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

#include <boost/optional.hpp>
#include <vector>

#include "PlateBoundaryStats.h"

#include "maths/PolylineOnSphere.h"


namespace GPlatesAppLogic
{
	namespace
	{
		void
		calculate_plate_boundary_stats_for_shared_sub_segment(
				std::vector<PlateBoundaryStat> &shared_sub_segment_plate_boundary_stats,
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &shared_sub_segment,
				const double &reconstruction_time,
				const double &uniform_point_spacing,
				const double &first_uniform_point_spacing,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type,
				VelocityUnits::Value velocity_units,
				const double &earth_radius_in_kms)
		{
			// Polyline geometry of the shared sub-segment.
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type shared_sub_segment_polyline =
					shared_sub_segment->get_shared_sub_segment_geometry();

			// Each point of shared sub-segment has a resolved vertex source info (used to calculate velocity at a point).
			resolved_vertex_source_info_seq_type shared_sub_segment_vertex_source_infos;
			shared_sub_segment->get_shared_sub_segment_point_source_infos(shared_sub_segment_vertex_source_infos);

			// Generate uniformly spaced points along the shared sub-segment.
			std::vector<GPlatesMaths::PointOnSphere> uniform_points;
			std::vector<std::pair<unsigned int/*segment index*/, double/*segment interpolation*/>> segment_informations;
			GPlatesMaths::uniformly_spaced_points(
					uniform_points,
					*shared_sub_segment_polyline,
					uniform_point_spacing,
					first_uniform_point_spacing,
					segment_informations);

			// Avoid unnecessary re-calculations for uniform points on the same segment (arc) of shared sub-segment polyline.
			boost::optional<unsigned int> last_segment_index;
			GPlatesMaths::Vector3D segment_start_absolute_velocity;
			GPlatesMaths::Vector3D segment_end_absolute_velocity;

			// Calculate statistics for each uniform point.
			const unsigned int num_uniform_points = uniform_points.size();
			for (unsigned int uniform_point_index = 0; uniform_point_index < num_uniform_points; ++uniform_point_index)
			{
				const GPlatesMaths::PointOnSphere &point = uniform_points[uniform_point_index];

				const unsigned int segment_index = segment_informations[uniform_point_index].first;
				const double &segment_interpolation = segment_informations[uniform_point_index].second;

				// If encountering a new segment (arc) of shared sub-segment, then calculate absolute velocities at its start/end points.
				if (segment_index != last_segment_index)
				{
					const GPlatesMaths::PointOnSphere &segment_start_point = shared_sub_segment_polyline->get_vertex(segment_index);
					const GPlatesMaths::PointOnSphere &segment_end_point = shared_sub_segment_polyline->get_vertex(segment_index + 1);

					const ResolvedVertexSourceInfo &segment_start_resolved_vertex_source = *shared_sub_segment_vertex_source_infos[segment_index];
					const ResolvedVertexSourceInfo &segment_end_resolved_vertex_source = *shared_sub_segment_vertex_source_infos[segment_index + 1];

					segment_start_absolute_velocity = segment_start_resolved_vertex_source.get_velocity_vector(
							segment_start_point,
							reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms);
					segment_end_absolute_velocity = segment_end_resolved_vertex_source.get_velocity_vector(
							segment_end_point,
							reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms);

					last_segment_index = segment_index;
				}

				// Interpolate the segment start and end velocity vectors.
				const GPlatesMaths::Vector3D absolute_velocity =
						(1.0 - segment_interpolation) * segment_start_absolute_velocity + segment_interpolation * segment_end_absolute_velocity;

				// Record the statistics for the current uniform point.
				shared_sub_segment_plate_boundary_stats.push_back(
						PlateBoundaryStat(
								point,
								absolute_velocity));
			}
		}
	}
}


void
GPlatesAppLogic::calculate_plate_boundary_stats(
		std::map<ResolvedTopologicalSharedSubSegment::non_null_ptr_type, std::vector<PlateBoundaryStat>> &plate_boundary_stats,
		const std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
		const double &reconstruction_time,
		const double &uniform_point_spacing,
		const double &first_uniform_point_spacing,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms)
{
	for (const auto &resolved_topological_section : resolved_topological_sections)
	{
		// Generate statistics at uniformly spaced points along the shared sub-segments of the current resolved topological section.
		for (const auto &shared_sub_segment : resolved_topological_section->get_shared_sub_segments())
		{
			// Calculate plate boundary statistics for the shared sub-segment.
			std::vector<PlateBoundaryStat> &shared_sub_segment_plate_boundary_stats = plate_boundary_stats[shared_sub_segment];
			calculate_plate_boundary_stats_for_shared_sub_segment(
					shared_sub_segment_plate_boundary_stats,
					shared_sub_segment,
					reconstruction_time,
					uniform_point_spacing,
					first_uniform_point_spacing,
					velocity_delta_time,
					velocity_delta_time_type,
					velocity_units,
					earth_radius_in_kms);
		}
	}
}
