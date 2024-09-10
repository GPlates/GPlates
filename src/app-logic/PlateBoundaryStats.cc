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

#include "ResolvedSubSegmentRangeInSection.h"

#include "maths/PolylineOnSphere.h"


namespace GPlatesAppLogic
{
	namespace
	{
		void
		calculate_plate_boundary_stats_for_shared_sub_segment(
				std::vector<PlateBoundaryStat> &shared_sub_segment_plate_boundary_stats,
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &shared_sub_segment,
				const double &distance_from_start_of_topological_section_to_start_of_shared_sub_segment,
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

				// Distance from start of topological section to the current location.
				//
				// This include distance from start of shared sub-segment to current point (along shared sub-segment).
				const double distance_from_start_of_topological_section = distance_from_start_of_topological_section_to_start_of_shared_sub_segment +
						first_uniform_point_spacing + uniform_point_index * uniform_point_spacing;

				// Record the statistics for the current uniform point.
				shared_sub_segment_plate_boundary_stats.push_back(
						PlateBoundaryStat(
								point,
								absolute_velocity,
								distance_from_start_of_topological_section));
			}
		}

		double
		distance_from_start_of_previous_shared_sub_segment(
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &prev_shared_sub_segment,
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &curr_shared_sub_segment,
				bool &is_gap_since_previous_shared_sub_segment)
		{
			double distance = 0;
			
			// Polyline geometry of the *previous* shared sub-segment.
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type prev_shared_sub_segment_polyline =
					prev_shared_sub_segment->get_shared_sub_segment_geometry();

			// Accumulate length of *previous* shared sub-segment.
			distance += prev_shared_sub_segment_polyline->get_arc_length().dval();

			//
			// If there's a gap between the current and previous shared sub-segments then accumulate the length of that gap.
			//

			// Last vertex of *previous* shared sub-segment.
			const GPlatesMaths::PointOnSphere &prev_shared_sub_segment_end_point =
					prev_shared_sub_segment_polyline->get_vertex(prev_shared_sub_segment_polyline->number_of_vertices() - 1);

			// Polyline geometry of the *current* shared sub-segment.
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type curr_shared_sub_segment_polyline =
					curr_shared_sub_segment->get_shared_sub_segment_geometry();

			// First vertex of *current* shared sub-segment.
			const GPlatesMaths::PointOnSphere &curr_shared_sub_segment_start_point = curr_shared_sub_segment_polyline->get_vertex(0);

			if (curr_shared_sub_segment_start_point != prev_shared_sub_segment_end_point)
			{
				// The gap between the end of the *previous* sub-segment and the start of the *current* sub-segment.
				const ResolvedSubSegmentRangeInSection gap_between_shared_sub_segments(
						curr_shared_sub_segment->get_section_geometry(),  // all shared sub-segments reference the same section geometry
						prev_shared_sub_segment->get_shared_sub_segment().get_end_intersection_or_rubber_band(),
						curr_shared_sub_segment->get_shared_sub_segment().get_start_intersection_or_rubber_band());

				// Accumulate the length of the gap between previous and current shared sub-segments (along the section geometry).
				distance += gap_between_shared_sub_segments.get_geometry()->get_arc_length().dval();

				is_gap_since_previous_shared_sub_segment = true;
			}
			else
			{
				is_gap_since_previous_shared_sub_segment = false;
			}

			return distance;
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
		// Distance from the start of a shared sub-segment to the first uniform point in it.
		double first_uniform_point_spacing_in_shared_sub_segment = first_uniform_point_spacing;

		// Track the distance from the *start* of the *first* shared sub-segment along the current resolved topological section.
		//
		// Note: This is NOT from the start of the *entire* topological section geometry.
		//       It only considers those parts (ie, the shared sub-segments) that contribute to resolved topological boundaries.
		//       Although gaps *between* shared sub-segments ARE considered.
		double distance_from_start_of_topological_section = 0;

		// Generate statistics at uniformly spaced points along the shared sub-segments of the current resolved topological section.
		boost::optional<ResolvedTopologicalSharedSubSegment::non_null_ptr_type> prev_shared_sub_segment;
		for (const auto &shared_sub_segment : resolved_topological_section->get_shared_sub_segments())
		{
			// Accumulate distance from start of previous sub-segment to start of current sub-segment.
			//
			// This can include a gap if the end of previous sub-segment is not coincident with the start of current sub-segment.
			if (prev_shared_sub_segment)
			{
				bool is_gap_since_previous_shared_sub_segment;
				distance_from_start_of_topological_section += distance_from_start_of_previous_shared_sub_segment(
						prev_shared_sub_segment.get(),
						shared_sub_segment,
						is_gap_since_previous_shared_sub_segment);

				// If there was a gap then reset the first uniform point spacing (to the default).
				if (is_gap_since_previous_shared_sub_segment)
				{
					first_uniform_point_spacing_in_shared_sub_segment = first_uniform_point_spacing;
				}
			}

			// Calculate plate boundary statistics for the current shared sub-segment.
			std::vector<PlateBoundaryStat> &shared_sub_segment_plate_boundary_stats = plate_boundary_stats[shared_sub_segment];
			calculate_plate_boundary_stats_for_shared_sub_segment(
					shared_sub_segment_plate_boundary_stats,
					shared_sub_segment,
					distance_from_start_of_topological_section,
					reconstruction_time,
					uniform_point_spacing,
					first_uniform_point_spacing_in_shared_sub_segment,
					velocity_delta_time,
					velocity_delta_time_type,
					velocity_units,
					earth_radius_in_kms);

			// Continue the uniform spacing of points *across* shared sub-segments (unless there's a gap - handled above).
			//
			// The first uniform point offset in the *next* sub-segment (if any) depends on the offset of the first point in the
			// *current* sub-segment and the number of uniform points added to the *current* sub-segment (and the sub-segment length).
			first_uniform_point_spacing_in_shared_sub_segment += shared_sub_segment_plate_boundary_stats.size() * uniform_point_spacing -
					shared_sub_segment->get_shared_sub_segment_geometry()->get_arc_length().dval();

			// Update previous shared sub-segment for next loop iteration.
			prev_shared_sub_segment = shared_sub_segment;
		}

		// Distance from the *start* of the *first* shared sub-segment to the *end* of the *last* shared sub-segment
		// (this includes any gaps between shared sub-segments).
		const double distance_from_start_of_first_to_end_of_last_shared_sub_segment = distance_from_start_of_topological_section +
				resolved_topological_section->get_shared_sub_segments().back()->get_shared_sub_segment_geometry()->get_arc_length().dval();

		// Now that we know the total distance from start of first to end of last shared sub-segments, go back through the uniformly
		// spaced points and set their distance to the *end* of the topological section (using their distance to *start*).
		for (const auto &shared_sub_segment : resolved_topological_section->get_shared_sub_segments())
		{
			// Plate boundary statistics for the current shared sub-segment.
			std::vector<PlateBoundaryStat> &shared_sub_segment_plate_boundary_stats = plate_boundary_stats[shared_sub_segment];
			for (auto &plate_boundary_stat : shared_sub_segment_plate_boundary_stats)
			{
				plate_boundary_stat.distance_to_end_of_topological_section = distance_from_start_of_first_to_end_of_last_shared_sub_segment -
						plate_boundary_stat.distance_from_start_of_topological_section;
			}
		}
	}
}
