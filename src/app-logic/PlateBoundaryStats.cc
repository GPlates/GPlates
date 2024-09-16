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

#include "scribe/Scribe.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Get the normal to the *previous* (great circle arc) segment.
		 *
		 * If the previous segment is zero length then search its previous segment, etc, until a non-zero length
		 * segment is found (with found segment index returned in 'segment_index), or none are found.
		 */
		boost::optional<GPlatesMaths::UnitVector3D>
		get_prev_boundary_normal(
				const GPlatesMaths::PolylineOnSphere &shared_sub_segment_polyline,
				unsigned int &segment_index)
		{
			// Search through *previous* segments for a non-zero length segment and use its normal.
			for (int prev_segment_index = segment_index - 1; prev_segment_index >= 0; --prev_segment_index)
			{
				const GPlatesMaths::GreatCircleArc &prev_segment = shared_sub_segment_polyline.get_segment(prev_segment_index);
				if (!prev_segment.is_zero_length())
				{
					segment_index = prev_segment_index;

					return prev_segment.rotation_axis();
				}
			}

			return boost::none;
		}

		/**
		 * Get the normal to the *next* (great circle arc) segment.
		 *
		 * If the next segment is zero length then search its next segment, etc, until a non-zero length
		 * segment is found (with found segment index returned in 'segment_index), or none are found.
		 */
		boost::optional<GPlatesMaths::UnitVector3D>
		get_next_boundary_normal(
				const GPlatesMaths::PolylineOnSphere &shared_sub_segment_polyline,
				unsigned int &segment_index)
		{
			// Search through *next* segments for a non-zero length segment and use its normal.
			const unsigned int num_segments = shared_sub_segment_polyline.number_of_segments();
			for (unsigned int next_segment_index = segment_index + 1; next_segment_index < num_segments; ++next_segment_index)
			{
				const GPlatesMaths::GreatCircleArc &next_segment = shared_sub_segment_polyline.get_segment(next_segment_index);
				if (!next_segment.is_zero_length())
				{
					segment_index = next_segment_index;

					return next_segment.rotation_axis();
				}
			}

			return boost::none;
		}

		/**
		 * Get the normal to the specified (great circle arc) segment, and its adjacent normals.
		 *
		 * If the specified segment is zero length then search adjacent segments until a non-zero length
		 * segment is found. Otherwise returns none (since polyline is zero length).
		 */
		boost::optional<GPlatesMaths::UnitVector3D>
		get_boundary_normal(
				const GPlatesMaths::PolylineOnSphere &shared_sub_segment_polyline,
				unsigned int segment_index,
				boost::optional<GPlatesMaths::UnitVector3D> &prev_boundary_normal,
				boost::optional<GPlatesMaths::UnitVector3D> &next_boundary_normal)
		{
			boost::optional<GPlatesMaths::UnitVector3D> boundary_normal;

			// Get the normal to the segment at index 'segment_index'.
			const GPlatesMaths::GreatCircleArc &segment = shared_sub_segment_polyline.get_segment(segment_index);
			if (!segment.is_zero_length())
			{
				boundary_normal = segment.rotation_axis();
			}
			else
			{
				// Requested segment is zero length, so get a normal from a previous non-zero-length segment.
				boundary_normal = get_prev_boundary_normal(shared_sub_segment_polyline, segment_index);
				if (!boundary_normal)
				{
					// All previous segments are zero length (or there are no previous segments),
					// so get a normal from a next (subsequent) non-zero-length segment.
					boundary_normal = get_next_boundary_normal(shared_sub_segment_polyline, segment_index);
					if (!boundary_normal)
					{
						// Couldn't get any non-zero segments.
						// The entire polyline is zero length, so we can't find a normal.
						return boost::none;
					}
				}
				//
				// Note that 'segment_index' has been modified (to match the non-zero-length segment found).
			}

			// Get the previous and next normals (if any).

			unsigned int prev_segment_index = segment_index;
			prev_boundary_normal = get_prev_boundary_normal(shared_sub_segment_polyline, prev_segment_index);

			unsigned int next_segment_index = segment_index;
			next_boundary_normal = get_next_boundary_normal(shared_sub_segment_polyline, next_segment_index);

			return boundary_normal;
		}

		/**
		 * Calculate plate boundary statistics at uniformly spaced points along a shared sub-segment.
		 *
		 * Note: Here we consider the start/end of the topological section to be the start/end of ALL its
		 *       shared sub-segments (not the actual start/end of the topological section geometry).
		 *
		 * Returns false if the shared sub-segment geometry is zero length or if it's not long enough
		 * to contain any uniform points.
		 */
		bool
		calculate_plate_boundary_stats_for_shared_sub_segment(
				std::vector<PlateBoundaryStat> &shared_sub_segment_plate_boundary_stats,
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &shared_sub_segment,
				const double &signed_distance_from_start_of_topological_section_to_start_of_shared_sub_segment,
				const double &signed_distance_from_end_of_topological_section_to_start_of_shared_sub_segment,
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
			const unsigned int num_uniform_points = uniform_points.size();
			if (num_uniform_points == 0)
			{
				// Shared sub-segment geometry is not long enough to contain any uniform points.
				return false;
			}

			// Avoid unnecessary re-calculations for uniform points on the same segment (arc) of shared sub-segment polyline.
			boost::optional<unsigned int> last_segment_index;
			GPlatesMaths::Vector3D segment_start_boundary_velocity;
			GPlatesMaths::Vector3D segment_end_boundary_velocity;
			boost::optional<GPlatesMaths::UnitVector3D> boundary_normal;
			boost::optional<GPlatesMaths::UnitVector3D> prev_boundary_normal;
			boost::optional<GPlatesMaths::UnitVector3D> next_boundary_normal;

			// Calculate statistics for each uniform point.
			for (unsigned int uniform_point_index = 0; uniform_point_index < num_uniform_points; ++uniform_point_index)
			{
				const GPlatesMaths::PointOnSphere &point = uniform_points[uniform_point_index];

				const unsigned int segment_index = segment_informations[uniform_point_index].first;
				const double &segment_interpolation = segment_informations[uniform_point_index].second;

				// If encountering a new segment (arc) of shared sub-segment, then calculate boundary normal for segment,
				// and boundary velocities at segment start/end points.
				if (segment_index != last_segment_index)
				{
					// Get the boundary normal of current segment (and adjacent normals, if available).
					boundary_normal = get_boundary_normal(
							*shared_sub_segment_polyline, segment_index, prev_boundary_normal, next_boundary_normal);
					if (!boundary_normal)
					{
						// The shared sub-segment geometry is zero length.
						return false;
					}

					const GPlatesMaths::PointOnSphere &segment_start_point = shared_sub_segment_polyline->get_vertex(segment_index);
					const GPlatesMaths::PointOnSphere &segment_end_point = shared_sub_segment_polyline->get_vertex(segment_index + 1);

					const ResolvedVertexSourceInfo &segment_start_resolved_vertex_source = *shared_sub_segment_vertex_source_infos[segment_index];
					const ResolvedVertexSourceInfo &segment_end_resolved_vertex_source = *shared_sub_segment_vertex_source_infos[segment_index + 1];

					segment_start_boundary_velocity = segment_start_resolved_vertex_source.get_velocity_vector(
							segment_start_point,
							reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms);
					segment_end_boundary_velocity = segment_end_resolved_vertex_source.get_velocity_vector(
							segment_end_point,
							reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms);

					last_segment_index = segment_index;
				}

				// Interpolate the segment start and end velocity vectors.
				const GPlatesMaths::Vector3D boundary_velocity =
						(1.0 - segment_interpolation) * segment_start_boundary_velocity + segment_interpolation * segment_end_boundary_velocity;

				// Distance from start of shared sub-segment to current point (along shared sub-segment).
				const double distance_from_start_of_shared_sub_segment = first_uniform_point_spacing + uniform_point_index * uniform_point_spacing;

				// Distance from start of topological section to the current location.
				const double signed_distance_from_start_of_topological_section =
						signed_distance_from_start_of_topological_section_to_start_of_shared_sub_segment + distance_from_start_of_shared_sub_segment;
				// Distance from the current location to end of topological section.
				const double signed_distance_to_end_of_topological_section =
						signed_distance_from_end_of_topological_section_to_start_of_shared_sub_segment - distance_from_start_of_shared_sub_segment;

				// Record the statistics for the current uniform point.
				shared_sub_segment_plate_boundary_stats.push_back(
						PlateBoundaryStat(
								point,
								boundary_normal.get(),
								boundary_velocity,
								signed_distance_from_start_of_topological_section,
								signed_distance_to_end_of_topological_section));
			}

			return true;
		}

		/**
		 * Calculate distances from the start of the topological section geometry to the start and end of the span of shared sub-segments.
		 *
		 * Note: This excludes any rubber-band parts of shared sub-segments.
		 *       We're only considering the actual topological section geometry itself.
		 */
		void
		calculate_distances_from_start_of_topological_section_to_start_and_end_of_shared_sub_segments(
				const ResolvedTopologicalSection::non_null_ptr_type &resolved_topological_section,
				double &distance_to_start_of_topological_section,
				double &distance_to_end_of_topological_section)
		{
			// All shared sub-segments reference the same section geometry.
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry =
					resolved_topological_section->get_shared_sub_segments().front()->get_section_geometry();

			boost::optional<ResolvedSubSegmentRangeInSection::Intersection> start_of_topological_section;
			boost::optional<ResolvedSubSegmentRangeInSection::Intersection> end_of_topological_section;

			// Find the closest start (and closest end) of the shared sub-segments to the start (and end) of the topological section geometry.
			for (const auto &shared_sub_segment : resolved_topological_section->get_shared_sub_segments())
			{
				const ResolvedSubSegmentRangeInSection &shared_sub_segment_range = shared_sub_segment->get_shared_sub_segment();

				// Look at *start* of shared sub-segment.
				if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &start_of_shared_sub_segment =
					shared_sub_segment_range.get_start_intersection())
				{
					if (start_of_topological_section)
					{
						// See if start of current shared sub-segment is closer to the start of topological section geometry.
						if (start_of_shared_sub_segment.get() < start_of_topological_section.get())
						{
							start_of_topological_section = start_of_shared_sub_segment;
						}
					}
					else
					{
						// First shared sub-segment encountered.
						start_of_topological_section = shared_sub_segment_range.get_start_intersection();
					}
				}
				else
				{
					// Else the start of shared sub-segment is either a rubber band or exactly at start of topological section geometry.
					//
					// In this case we consider the start of the topological section to be the start of its section geometry
					// (because we're not considering rubber band sections to be part of a topological section for our purposes here).
					start_of_topological_section = ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
							*section_geometry, true/*at_start*/);
				}

				// Look at *end* of shared sub-segment.
				if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &end_of_shared_sub_segment =
					shared_sub_segment_range.get_end_intersection())
				{
					if (end_of_topological_section)
					{
						// See if end of current shared sub-segment is closer to the end of topological section geometry.
						if (end_of_shared_sub_segment.get() > end_of_topological_section.get())
						{
							end_of_topological_section = end_of_shared_sub_segment;
						}
					}
					else
					{
						// First shared sub-segment encountered.
						end_of_topological_section = shared_sub_segment_range.get_end_intersection();
					}
				}
				else
				{
					// Else the end of shared sub-segment is either a rubber band or exactly at end of topological section geometry.
					//
					// In this case we consider the end of the topological section to be the end of its section geometry
					// (because we're not considering rubber band sections to be part of a topological section for our purposes here).
					end_of_topological_section = ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
							*section_geometry, false/*at_start*/);
				}
			}

			// Range of topological section geometry from its first vertex to the closest start point of the shared sub-segments.
			const ResolvedSubSegmentRangeInSection range_to_start_of_shared_sub_segments(
					section_geometry,
					// No start intersection (or rubber band) means beginning of topological section geometry...
					boost::none,
					ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_of_topological_section.get()));
			distance_to_start_of_topological_section = range_to_start_of_shared_sub_segments.get_geometry()->get_arc_length().dval();

			// Range of topological section geometry from its first vertex to the farthest end point of the shared sub-segments.
			const ResolvedSubSegmentRangeInSection range_to_end_of_shared_sub_segments(
					section_geometry,
					// No start intersection (or rubber band) means beginning of topological section geometry...
					boost::none,
					ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_of_topological_section.get()));
			distance_to_end_of_topological_section = range_to_end_of_shared_sub_segments.get_geometry()->get_arc_length().dval();
		}

		/**
		 * Calculate the *signed* distance from the start of the shared sub-segment to the start of topological section geometry.
		 *
		 * It is negative if start of shared sub-segment is a rubber-band. That is, it's not on
		 * the actual resolved topological section geometry but on the part that rubber-bands (joins)
		 * the *start* of the resolved topological section geometry with an adjacent resolved topological section
		 * (that's also part of a plate boundary).
		 */
		double
		calculate_signed_distance_from_start_of_topological_section_to_start_of_shared_sub_segment(
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &shared_sub_segment)
		{
			const ResolvedSubSegmentRangeInSection &shared_sub_segment_range = shared_sub_segment->get_shared_sub_segment();

			// See if start of shared sub-segment is an intersection.
			const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &start_intersection_of_shared_sub_segment =
					shared_sub_segment_range.get_start_intersection();
			if (start_intersection_of_shared_sub_segment)
			{
				// Range from start of topological section geometry to start intersection of shared sub-segment.
				const ResolvedSubSegmentRangeInSection from_start_of_topological_section_range(
						shared_sub_segment->get_section_geometry(),
						// No start intersection (or rubber band) means beginning of topological section geometry...
						boost::none,
						ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_intersection_of_shared_sub_segment.get()));

				// Distance is positive since it's an intersection.
				return from_start_of_topological_section_range.get_geometry()->get_arc_length().dval();
			}

			// See if start of shared sub-segment is a rubber band.
			const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &start_rubber_band_of_shared_sub_segment =
					shared_sub_segment_range.get_start_rubber_band();
			if (start_rubber_band_of_shared_sub_segment)
			{
				// Range from start rubber band of shared sub-segment to start of topological section geometry.
				const ResolvedSubSegmentRangeInSection to_start_of_topological_section_range(
						shared_sub_segment->get_section_geometry(),
						ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band_of_shared_sub_segment.get()),
						// Start intersection at beginning of topological section geometry...
						ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(
								ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
										*shared_sub_segment->get_section_geometry(), true/*at_start*/)));

				// Distance is negative since it's a rubber band.
				return -to_start_of_topological_section_range.get_geometry()->get_arc_length().dval();
			}

			// Shared sub-segment has no start intersection or start rubber band, which means it starts exactly at the
			// start of the topological section geometry.
			return 0.0;
		}

		/**
		 * Returns true if the end of previous shared sub-segment is coincident with the start of the current shared sub-segment and
		 * they're both intersections that are *inside* the topological section geometry (ie, not *on* the geometry end points which
		 * would imply that one of the shared sub-segments is has rubber banding, or is zero length).
		 */
		bool
		adjacent_shared_sub_segments_join_inside_topological_section(
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &prev_shared_sub_segment,
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &curr_shared_sub_segment)
		{
			// See if end of previous shared sub-segment is an intersection.
			const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &end_intersection_of_prev_shared_sub_segment =
					prev_shared_sub_segment->get_shared_sub_segment().get_end_intersection();
			if (!end_intersection_of_prev_shared_sub_segment)
			{
				return false;
			}

			// See if start of current shared sub-segment is an intersection.
			const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &start_intersection_of_curr_shared_sub_segment =
					curr_shared_sub_segment->get_shared_sub_segment().get_start_intersection();
			if (!start_intersection_of_curr_shared_sub_segment)
			{
				return false;
			}

			// See if both intersections coincide.
			if (end_intersection_of_prev_shared_sub_segment->position != start_intersection_of_curr_shared_sub_segment->position)
			{
				return false;
			}

			// Both shared sub-segments reference the same section geometry.
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry = curr_shared_sub_segment->get_section_geometry();

			// See if both intersections are *inside* the section geometry (ie, not coincident with topological section geometry end points).
			if (start_intersection_of_curr_shared_sub_segment.get() <=
				ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(*section_geometry, true/*at_start*/))
			{
				return false;
			}
			if (end_intersection_of_prev_shared_sub_segment.get() >=
				ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(*section_geometry, false/*at_start*/))
			{
				return false;
			}

			return true;
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
		// Distances from the start of the topological section geometry to the start and end of the span of shared sub-segments
		// (the minimum/maximum range of topological section covered by its shared sub-segments, including any gaps between them).
		double distance_from_start_of_topological_section_to_start_of_shared_sub_segments;
		double distance_from_start_of_topological_section_to_end_of_shared_sub_segments;
		calculate_distances_from_start_of_topological_section_to_start_and_end_of_shared_sub_segments(
				resolved_topological_section,
				distance_from_start_of_topological_section_to_start_of_shared_sub_segments,
				distance_from_start_of_topological_section_to_end_of_shared_sub_segments);

		// Distance from the start of a shared sub-segment to the first uniform point in it.
		double first_uniform_point_spacing_in_shared_sub_segment = first_uniform_point_spacing;

		// Generate statistics at uniformly spaced points along the shared sub-segments of the current resolved topological section.
		boost::optional<ResolvedTopologicalSharedSubSegment::non_null_ptr_type> prev_shared_sub_segment;
		for (const auto &shared_sub_segment : resolved_topological_section->get_shared_sub_segments())
		{
			if (prev_shared_sub_segment)
			{
				// If the previous shared sub-segment joins the current shared sub-segment and the join point is
				// *inside* the topological section then continue the uniform spacing of points.
				// Otherwise there was either a gap between them or one (or both) shared sub-segments included rubber banding.
				if (adjacent_shared_sub_segments_join_inside_topological_section(prev_shared_sub_segment.get(), shared_sub_segment))
				{
					// Continue the uniform spacing of points from the previous shared sub-segment.
					//
					// The first uniform point offset in the *current* sub-segment depends on the offset of the first point in the
					// *previous* sub-segment and the number of uniform points added to the *previous* sub-segment (and its sub-segment length).
					first_uniform_point_spacing_in_shared_sub_segment += plate_boundary_stats[prev_shared_sub_segment.get()].size() * uniform_point_spacing -
							prev_shared_sub_segment.get()->get_shared_sub_segment_geometry()->get_arc_length().dval();
				}
				else
				{
					first_uniform_point_spacing_in_shared_sub_segment = first_uniform_point_spacing;
				}
			}

			// Signed distance from start of topological section geometry to the start of the current shared sub-segment.
			const double signed_distance_from_start_of_topological_section =
					calculate_signed_distance_from_start_of_topological_section_to_start_of_shared_sub_segment(shared_sub_segment);

			// Distance from the *start* of ALL shared sub-segments to the *start* of the CURRENT shared sub-segment.
			//
			// Note: This is NOT from the start of the *entire* topological section geometry.
			//       It only considers those parts (ie, the shared sub-segments) that contribute to resolved topological boundaries.
			//       Although gaps *between* shared sub-segments ARE considered.
			const double signed_distance_from_start_of_shared_sub_segments = signed_distance_from_start_of_topological_section -
					distance_from_start_of_topological_section_to_start_of_shared_sub_segments;
			const double signed_distance_to_end_of_shared_sub_segments = distance_from_start_of_topological_section_to_end_of_shared_sub_segments -
					signed_distance_from_start_of_topological_section;

			// Calculate plate boundary statistics for the current shared sub-segment.
			std::vector<PlateBoundaryStat> shared_sub_segment_plate_boundary_stats;
			if (calculate_plate_boundary_stats_for_shared_sub_segment(
					shared_sub_segment_plate_boundary_stats,
					shared_sub_segment,
					signed_distance_from_start_of_shared_sub_segments,
					signed_distance_to_end_of_shared_sub_segments,
					reconstruction_time,
					uniform_point_spacing,
					first_uniform_point_spacing_in_shared_sub_segment,
					velocity_delta_time,
					velocity_delta_time_type,
					velocity_units,
					earth_radius_in_kms))
			{
				// Successfully calculated statistics for the current shared sub-segment, so record them in the mapping.
				plate_boundary_stats[shared_sub_segment].swap(shared_sub_segment_plate_boundary_stats);
			}

			// Update previous shared sub-segment for next loop iteration.
			prev_shared_sub_segment = shared_sub_segment;
		}
	}
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::PlateBoundaryStat::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<PlateBoundaryStat> &plate_boundary_stat)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, plate_boundary_stat->d_point, "point");
		scribe.save(TRANSCRIBE_SOURCE, plate_boundary_stat->d_boundary_normal, "boundary_normal");
		scribe.save(TRANSCRIBE_SOURCE, plate_boundary_stat->d_boundary_velocity, "boundary_velocity");
		scribe.save(TRANSCRIBE_SOURCE, plate_boundary_stat->d_signed_distance_from_start_of_topological_section, "signed_distance_from_start_of_topological_section");
		scribe.save(TRANSCRIBE_SOURCE, plate_boundary_stat->d_signed_distance_to_end_of_topological_section, "signed_distance_to_end_of_topological_section");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesMaths::PointOnSphere> point_ = scribe.load<GPlatesMaths::PointOnSphere>(TRANSCRIBE_SOURCE, "point");
		if (!point_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesMaths::UnitVector3D> boundary_normal_ = scribe.load<GPlatesMaths::UnitVector3D>(TRANSCRIBE_SOURCE, "boundary_normal");
		if (!boundary_normal_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesMaths::Vector3D boundary_velocity_;
		GPlatesMaths::Real signed_distance_from_start_of_topological_section_;
		GPlatesMaths::Real signed_distance_to_end_of_topological_section_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, boundary_velocity_, "boundary_velocity") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, signed_distance_from_start_of_topological_section_, "signed_distance_from_start_of_topological_section") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, signed_distance_to_end_of_topological_section_, "signed_distance_to_end_of_topological_section"))
		{
			return scribe.get_transcribe_result();
		}

		plate_boundary_stat.construct_object(
				point_,
				boundary_normal_,
				boundary_velocity_,
				signed_distance_from_start_of_topological_section_.dval(),
				signed_distance_to_end_of_topological_section_.dval());
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::PlateBoundaryStat::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_point, "point") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_boundary_normal, "boundary_normal") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_boundary_velocity, "boundary_velocity") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_signed_distance_from_start_of_topological_section, "signed_distance_from_start_of_topological_section") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_signed_distance_to_end_of_topological_section, "signed_distance_to_end_of_topological_section"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
