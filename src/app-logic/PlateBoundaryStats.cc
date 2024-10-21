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
#include <map>
#include <vector>

#include "PlateBoundaryStats.h"

#include "PlateVelocityUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedSubSegmentRangeInSection.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"

#include "maths/CartesianConvMatrix3D.h"
#include "maths/PointInPolygon.h"
#include "maths/PolylineOnSphere.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Get the resolved topological boundaries/networks sharing a shared sub-segment separated into
		 * those on the left and right of the shared sub-segment.
		 */
		void
		get_resolved_topologies_sharing_shared_sub_segment(
				const ResolvedTopologicalSharedSubSegment::non_null_ptr_type &shared_sub_segment,
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &left_resolved_topological_boundaries,
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &left_resolved_topological_networks,
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &right_resolved_topological_boundaries,
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &right_resolved_topological_networks)
		{
			for (const auto &resolved_topology_info : shared_sub_segment->get_sharing_resolved_topologies())
			{
				// See if a resolved topological boundary.
				if (boost::optional<const ResolvedTopologicalBoundary *> resolved_topological_boundary =
						ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
								const ResolvedTopologicalBoundary *>(resolved_topology_info.resolved_topology))
				{
					if (resolved_topology_info.is_resolved_topology_on_left())
					{
						left_resolved_topological_boundaries.push_back(resolved_topological_boundary.get());
					}
					else
					{
						right_resolved_topological_boundaries.push_back(resolved_topological_boundary.get());
					}
				}
				// Else it should be a resolved topological network.
				else if (boost::optional<const ResolvedTopologicalNetwork *> resolved_topological_network =
						ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
								const ResolvedTopologicalNetwork *>(resolved_topology_info.resolved_topology))
				{
					if (resolved_topology_info.is_resolved_topology_on_left())
					{
						left_resolved_topological_networks.push_back(resolved_topological_network.get());
					}
					else
					{
						right_resolved_topological_networks.push_back(resolved_topological_network.get());
					}
				}
			}
		}

		/**
		 * Get the normal to the specified (great circle arc) segment.
		 *
		 * If the specified segment is zero length then search adjacent segments until a non-zero length
		 * segment is found. Otherwise returns none (since polyline is zero length).
		 */
		boost::optional<GPlatesMaths::UnitVector3D>
		get_boundary_normal(
				const GPlatesMaths::PolylineOnSphere &shared_sub_segment_polyline,
				unsigned int segment_index)
		{
			boost::optional<GPlatesMaths::UnitVector3D> boundary_normal;

			// Get the normal to the segment at index 'segment_index'.
			const GPlatesMaths::GreatCircleArc &segment = shared_sub_segment_polyline.get_segment(segment_index);
			if (!segment.is_zero_length())
			{
				return segment.rotation_axis();
			}

			//
			// Requested segment is zero length, so get a normal from a previous (or next) non-zero-length segment.
			//

			// Search through *previous* segments for a non-zero length segment and use its normal.
			for (int prev_segment_index = segment_index - 1; prev_segment_index >= 0; --prev_segment_index)
			{
				const GPlatesMaths::GreatCircleArc &prev_segment = shared_sub_segment_polyline.get_segment(prev_segment_index);
				if (!prev_segment.is_zero_length())
				{
					return prev_segment.rotation_axis();
				}
			}

			// All previous segments are zero length (or there are no previous segments),
			// so get a normal from a next (subsequent) non-zero-length segment.
			// Search through *next* segments for a non-zero length segment and use its normal.
			const unsigned int num_segments = shared_sub_segment_polyline.number_of_segments();
			for (unsigned int next_segment_index = segment_index + 1; next_segment_index < num_segments; ++next_segment_index)
			{
				const GPlatesMaths::GreatCircleArc &next_segment = shared_sub_segment_polyline.get_segment(next_segment_index);
				if (!next_segment.is_zero_length())
				{
					return next_segment.rotation_axis();
				}
			}

			// Couldn't get any non-zero segments.
			// The entire polyline is zero length, so we can't find a normal.
			return boost::none;
		}

		/**
		 * Return the velocity of the plate or network (and associated plate/network) that @a point intersects.
		 */
		bool
		get_plate_velocity(
				const GPlatesMaths::PointOnSphere &point,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type,
				VelocityUnits::Value velocity_units,
				const double &earth_radius_in_kms,
				const std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &resolved_topological_boundaries,
				const std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &resolved_topological_networks,
				const PlateVelocityUtils::StageRotationCalculator &resolved_boundary_stage_rotation_calculator,
				boost::optional<GPlatesMaths::Vector3D> &plate_velocity,
				TopologyPointLocation &plate)  // location in plate (or network), if any
		{
			// Search topological networks first (deforming regions).
			for (const auto &resolved_topological_network : resolved_topological_networks)
			{
				// See if point is inside polygon boundary of deforming network and, if so, calculate its velocity.
				boost::optional<
						std::pair<
								GPlatesMaths::Vector3D,
								ResolvedTriangulation::Network::PointLocation> >
						velocity = resolved_topological_network->get_triangulation_network().calculate_velocity(
								point,
								velocity_delta_time,
								velocity_delta_time_type,
								velocity_units,
								earth_radius_in_kms);
				if (velocity)
				{
					plate_velocity = velocity->first;
					plate = TopologyPointLocation(resolved_topological_network, velocity->second);

					return true;
				}
			}

			// Then search topological boundaries (rigid plates).
			for (const auto &resolved_topological_boundary : resolved_topological_boundaries)
			{
				// See if point is inside polygon boundary of rigid plate.
				if (resolved_topological_boundary->resolved_topology_boundary()->is_point_in_polygon(
						point,
						GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
				{
					// Get the plate ID from resolved boundary.
					const boost::optional<GPlatesModel::integer_plate_id_type> resolved_boundary_plate_id =
							resolved_topological_boundary->plate_id();
					if (resolved_boundary_plate_id)
					{
						// Calculate the velocity of the point inside the resolved boundary.
						const GPlatesMaths::Vector3D velocity =
								PlateVelocityUtils::calculate_velocity_vector(
										point,
										resolved_boundary_plate_id.get(),
										resolved_topological_boundary->get_reconstruction_tree_creator(),
										resolved_boundary_stage_rotation_calculator,
										velocity_units,
										earth_radius_in_kms);

						plate_velocity = velocity;
						plate = TopologyPointLocation(resolved_topological_boundary);

						return true;
					}
				}
			}

			return false;
		}

		/**
		 * Return the velocities of the left and right plates at the specified point on the plate boundary.
		 *
		 * First the plates/networks sharing the shared sub-segment (that the boundary point is on) are tested for intersection
		 * for the boundary point. Then (if no intersection) the boundary point is moved slightly onto the plate and tested
		 * for intersection with ALL plates/networks - the point is moved slightly such that it doesn't land ON the plate/network outline
		 * (ie, we don't want it to land on a plate that's on the right when we're looking for the left plate velocity).
		 *
		 * Note: Could fail to find either left or right (or both) plate velocities.
		 */
		void
		get_left_and_right_plate_velocities(
				const GPlatesMaths::PointOnSphere &boundary_point,
				const GPlatesMaths::UnitVector3D &boundary_normal,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type,
				VelocityUnits::Value velocity_units,
				const double &earth_radius_in_kms,
				const std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &left_sharing_resolved_topological_boundaries,
				const std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &left_sharing_resolved_topological_networks,
				const std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &right_sharing_resolved_topological_boundaries,
				const std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &right_sharing_resolved_topological_networks,
				const std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &all_resolved_topological_boundaries,
				const std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &all_resolved_topological_networks,
				const PlateVelocityUtils::StageRotationCalculator &resolved_boundary_stage_rotation_calculator,
				boost::optional<GPlatesMaths::Vector3D> &left_plate_velocity,
				boost::optional<GPlatesMaths::Vector3D> &right_plate_velocity,
				TopologyPointLocation &left_plate,   // left plate (or network), if any
				TopologyPointLocation &right_plate)  // right plate (or network), if any
		{
			// Attempt to calculate left plate velocity using the *left* resolved topologies that *share* the shared sub-segment.
			if (!get_plate_velocity(
					// Note: Can use point ON the boundary because resolved plates/networks use a point-in-polygon boundary test that, in turn,
					//       includes points ON the polygon outline (if they're within a very small threshold distance from the polygon's outline).
					//       And we're only testing resolved plates/networks that are on the *left*...
					boundary_point,
					reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms,
					left_sharing_resolved_topological_boundaries, left_sharing_resolved_topological_networks, resolved_boundary_stage_rotation_calculator,
					left_plate_velocity, left_plate))
			{
				// That failed, so use *all* resolved topologies (eg, the full global set of topologies).
				//
				// Move the point a very small distance to the left.
				// This helps ensure that we don't accidentally sample the right plate when we only want to sample the left plate.
				//
				// Note: The rigid plates and deforming networks have polygon boundaries with a tiny threshold for detecting if a
				//       point is ON the outline of the polygon. So we want a distance that exceeds that threshold.
				//       That threshold is about 1.4e-6 radians (about 9 metres).
				//       So we multiply that by 10 to ensure enough clearance.
				const double offset_distance = 10 * GPlatesMaths::PointInPolygon::get_point_on_polygon_threshold_sine();  // ~90 metres
				const GPlatesMaths::PointOnSphere left_point(
						(GPlatesMaths::Vector3D(boundary_point.position_vector()) + offset_distance * boundary_normal).get_normalisation());

				get_plate_velocity(
						left_point,
						reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms,
						all_resolved_topological_boundaries, all_resolved_topological_networks, resolved_boundary_stage_rotation_calculator,
						left_plate_velocity, left_plate);
			}

			// Attempt to calculate right plate velocity using the *right* resolved topologies that *share* the shared sub-segment.
			if (!get_plate_velocity(
					// Note: Can use point ON the boundary because resolved plates/networks use a point-in-polygon boundary test that, in turn,
					//       includes points ON the polygon outline (if they're within a very small threshold distance from the polygon's outline)...
					//       And we're only testing resolved plates/networks that are on the *right*...
					boundary_point,
					reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms,
					right_sharing_resolved_topological_boundaries, right_sharing_resolved_topological_networks, resolved_boundary_stage_rotation_calculator,
					right_plate_velocity, right_plate))
			{
				// That failed, so use *all* resolved topologies (eg, the full global set of topologies).
				//
				// Move the point a very small distance to the right.
				// This helps ensure that we don't accidentally sample the left plate when we only want to sample the right plate.
				//
				// Note: The rigid plates and deforming networks have polygon boundaries with a tiny threshold for detecting if a
				//       point is ON the outline of the polygon. So we want a distance that exceeds that threshold.
				//       That threshold is about 1.4e-6 radians (about 9 metres).
				//       So we multiply that by 10 to ensure enough clearance.
				const double offset_distance = 10 * GPlatesMaths::PointInPolygon::get_point_on_polygon_threshold_sine();  // ~90 metres
				const GPlatesMaths::PointOnSphere right_point(
						(GPlatesMaths::Vector3D(boundary_point.position_vector()) - offset_distance * boundary_normal).get_normalisation());

				get_plate_velocity(
						right_point,
						reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms,
						all_resolved_topological_boundaries, all_resolved_topological_networks, resolved_boundary_stage_rotation_calculator,
						right_plate_velocity, right_plate);
			}
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
				const std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &all_resolved_topological_boundaries,
				const std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &all_resolved_topological_networks,
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
			// Arc length of polyline geometry (of the shared sub-segment).
			const double shared_sub_segment_polyline_arc_length = shared_sub_segment_polyline->get_arc_length().dval();

			// Each point of shared sub-segment has a resolved vertex source info (used to calculate velocity at a point).
			resolved_vertex_source_info_seq_type shared_sub_segment_vertex_source_infos;
			shared_sub_segment->get_shared_sub_segment_point_source_infos(shared_sub_segment_vertex_source_infos);

			// The resolved topological boundaries/networks sharing the shared sub-segment.
			std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> left_sharing_resolved_topological_boundaries;
			std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> left_sharing_resolved_topological_networks;
			std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> right_sharing_resolved_topological_boundaries;
			std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> right_sharing_resolved_topological_networks;
			get_resolved_topologies_sharing_shared_sub_segment(shared_sub_segment,
					left_sharing_resolved_topological_boundaries, left_sharing_resolved_topological_networks,
					right_sharing_resolved_topological_boundaries, right_sharing_resolved_topological_networks);

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

			// Avoid re-calculating stage rotations for resolved topological boundaries with the same plate ID.
			const PlateVelocityUtils::StageRotationCalculator resolved_boundary_stage_rotation_calculator(
					reconstruction_time, velocity_delta_time, velocity_delta_time_type);

			// Calculate statistics for each uniform point.
			for (unsigned int uniform_point_index = 0; uniform_point_index < num_uniform_points; ++uniform_point_index)
			{
				const GPlatesMaths::PointOnSphere &boundary_point = uniform_points[uniform_point_index];

				// The length of the shared sub-segment polyline represented by the current point.
				double point_boundary_length;
				if (num_uniform_points == 1)  // first and last point
				{
					point_boundary_length = shared_sub_segment_polyline_arc_length;  // entire length
				}
				else if (uniform_point_index == 0)  // first point
				{
					point_boundary_length = first_uniform_point_spacing + 0.5 * uniform_point_spacing;
				}
				else if (uniform_point_index == num_uniform_points - 1)  // last point
				{
					point_boundary_length = 0.5 * uniform_point_spacing +
							shared_sub_segment_polyline_arc_length -
							(first_uniform_point_spacing + (num_uniform_points - 1) * uniform_point_spacing);
				}
				else  // neither first nor last point
				{
					// Points other than the first and last have the same length (uniform point spacing).
					point_boundary_length = uniform_point_spacing;
				}

				const unsigned int segment_index = segment_informations[uniform_point_index].first;
				const double &segment_interpolation = segment_informations[uniform_point_index].second;

				// If encountering a new segment (arc) of shared sub-segment, then calculate boundary normal for segment,
				// and boundary velocities at segment start/end points.
				if (segment_index != last_segment_index)
				{
					// Get the boundary normal of current segment (or adjacent segments if current segment is zero length).
					boundary_normal = get_boundary_normal(*shared_sub_segment_polyline, segment_index);
					if (!boundary_normal)
					{
						// The entire shared sub-segment geometry is zero length.
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

				// Get the velocities on the plates to the left and right of the current point
				// (when following the order or points in the shared sub-segment).
				boost::optional<GPlatesMaths::Vector3D> left_plate_velocity;
				boost::optional<GPlatesMaths::Vector3D> right_plate_velocity;
				TopologyPointLocation left_plate;   // default is not located inside any plate/network
				TopologyPointLocation right_plate;  // default is not located inside any plate/network
				get_left_and_right_plate_velocities(
						boundary_point, boundary_normal.get(),
						reconstruction_time, velocity_delta_time, velocity_delta_time_type, velocity_units, earth_radius_in_kms,
						left_sharing_resolved_topological_boundaries, left_sharing_resolved_topological_networks,
						right_sharing_resolved_topological_boundaries, right_sharing_resolved_topological_networks,
						all_resolved_topological_boundaries, all_resolved_topological_networks,
						resolved_boundary_stage_rotation_calculator,
						left_plate_velocity, right_plate_velocity,
						left_plate, right_plate);

				//
				// Note: Distances to start/end of shared sub-segment include rubber banding since we're considering the entire shared sub-segment polyline.
				//       Whereas *signed* distances to start/end of topological section do NOT include rubber banding since we're only considering the
				//       actual topological section geometry itself.
				//

				// Distance from start of shared sub-segment to current point (along shared sub-segment).
				const double distance_from_start_of_shared_sub_segment = first_uniform_point_spacing + uniform_point_index * uniform_point_spacing;
				// Distance from current point to end of shared sub-segment (along shared sub-segment).
				const double distance_to_end_of_shared_sub_segment = shared_sub_segment_polyline_arc_length - distance_from_start_of_shared_sub_segment;

				// Signed distance from start of topological section to the boundary point.
				const double signed_distance_from_start_of_topological_section =
						signed_distance_from_start_of_topological_section_to_start_of_shared_sub_segment + distance_from_start_of_shared_sub_segment;
				// Signed distance from the boundary point to end of topological section.
				const double signed_distance_to_end_of_topological_section =
						signed_distance_from_end_of_topological_section_to_start_of_shared_sub_segment - distance_from_start_of_shared_sub_segment;

				// Record the statistics for the current uniform point.
				shared_sub_segment_plate_boundary_stats.push_back(
						PlateBoundaryStat(
								boundary_point,
								point_boundary_length,
								boundary_normal.get(),
								boundary_velocity,
								left_plate, right_plate,
								left_plate_velocity, right_plate_velocity,
								distance_from_start_of_shared_sub_segment, distance_to_end_of_shared_sub_segment,
								signed_distance_from_start_of_topological_section, signed_distance_to_end_of_topological_section));
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
		get_distances_from_start_of_topological_section_to_start_and_end_of_shared_sub_segments(
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
		get_signed_distance_from_start_of_topological_section_to_start_of_shared_sub_segment(
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
		const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &all_resolved_topological_boundaries,
		const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &all_resolved_topological_networks,
		const double &reconstruction_time,
		const double &uniform_point_spacing,
		boost::optional<double> first_uniform_point_spacing,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms)
{
	// If the *first* uniform point spacing was not specified then set it to half the uniform point spacing.
	if (!first_uniform_point_spacing)
	{
		first_uniform_point_spacing = 0.5 * uniform_point_spacing;
	}

	for (const auto &resolved_topological_section : resolved_topological_sections)
	{
		// Each ResolvedTopologicalSection should have at least one shared sub-segment.
		// But check just in case, and skip if empty.
		if (resolved_topological_section->get_shared_sub_segments().empty())
		{
			continue;
		}

		// Distances from the start of the topological section geometry to the start and end of the span of shared sub-segments
		// (the minimum/maximum range of topological section covered by its shared sub-segments, including any gaps between them).
		double distance_from_start_of_topological_section_to_start_of_shared_sub_segments;
		double distance_from_start_of_topological_section_to_end_of_shared_sub_segments;
		get_distances_from_start_of_topological_section_to_start_and_end_of_shared_sub_segments(
				resolved_topological_section,
				distance_from_start_of_topological_section_to_start_of_shared_sub_segments,
				distance_from_start_of_topological_section_to_end_of_shared_sub_segments);

		// Distance from the start of a shared sub-segment to the first uniform point in it.
		double first_uniform_point_spacing_in_shared_sub_segment = first_uniform_point_spacing.get();

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
					first_uniform_point_spacing_in_shared_sub_segment = first_uniform_point_spacing.get();
				}
			}

			// Signed distance from start of topological section geometry to the start of the current shared sub-segment.
			const double signed_distance_from_start_of_topological_section =
					get_signed_distance_from_start_of_topological_section_to_start_of_shared_sub_segment(shared_sub_segment);

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
					all_resolved_topological_boundaries,
					all_resolved_topological_networks,
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


double
GPlatesAppLogic::PlateBoundaryStat::get_boundary_normal_azimuth() const
{
	const boost::tuple<GPlatesMaths::Real, GPlatesMaths::Real, GPlatesMaths::Real> coords =
			GPlatesMaths::convert_from_geocentric_to_magnitude_azimuth_inclination(
					GPlatesMaths::CartesianConvMatrix3D(d_boundary_point),
					GPlatesMaths::Vector3D(d_boundary_normal));

	return boost::get<1>(coords).dval();
}


boost::optional<GPlatesMaths::Vector3D>
GPlatesAppLogic::PlateBoundaryStat::get_convergence_velocity() const
{
	if (!d_left_plate_velocity || !d_right_plate_velocity)
	{
		return boost::none;
	}

	return d_right_plate_velocity.get() - d_left_plate_velocity.get();
}


double
GPlatesAppLogic::PlateBoundaryStat::get_convergence_velocity_magnitude(
		bool return_signed_magnitude) const
{
	const boost::optional<GPlatesMaths::Vector3D> convergence_velocity = get_convergence_velocity();

	// Return NaN if there is no plate on the left or no plate on the right.
	if (!convergence_velocity)
	{
		return GPlatesMaths::quiet_nan<double>();
	}

	// Return zero if the convergence velocity magnitude is zero.
	//
	// This is to match the behaviour of the convergence obliquity.
	if (convergence_velocity->is_zero_magnitude())
	{
		return 0;
	}

	double convergence_velocity_magnitude = convergence_velocity->magnitude().dval();

	// Negate magnitude if returning *signed* magnitude and plates are *diverging*.
	if (return_signed_magnitude &&
		dot(convergence_velocity.get(), d_boundary_normal).dval() < 0)
	{
		convergence_velocity_magnitude = -convergence_velocity_magnitude;
	}

	return convergence_velocity_magnitude;
}


double
GPlatesAppLogic::PlateBoundaryStat::get_convergence_velocity_obliquity() const
{
	const boost::optional<GPlatesMaths::Vector3D> convergence_velocity = get_convergence_velocity();

	// Return NaN if there is no plate on the left or no plate on the right.
	if (!convergence_velocity)
	{
		return GPlatesMaths::quiet_nan<double>();
	}

	return get_velocity_obliquity(convergence_velocity.get());
}


double
GPlatesAppLogic::PlateBoundaryStat::get_velocity_magnitude(
		const GPlatesMaths::Vector3D &velocity) const
{
	// Return zero if the velocity magnitude is zero.
	//
	// This is to match the behaviour of the velocity obliquity.
	if (velocity.is_zero_magnitude())
	{
		return 0;
	}

	return velocity.magnitude().dval();
}


double
GPlatesAppLogic::PlateBoundaryStat::get_velocity_obliquity(
		const GPlatesMaths::Vector3D &velocity) const
{
	// Return zero if the velocity is zero.
	if (velocity.is_zero_magnitude())
	{
		return 0;
	}

	// Direction towards which we rotate from the boundary normal in a clockwise fashion.
	const GPlatesMaths::Vector3D clockwise_direction = cross(d_boundary_normal, d_boundary_point.position_vector());

	// Angle of the velocity relative to the boundary normal.
	double obliquity = acos(dot(velocity.get_normalisation(), d_boundary_normal)).dval();

	// Anti-clockwise direction is negative.
	if (dot(velocity, clockwise_direction).dval() < 0)
	{
		obliquity = -obliquity;
	}

	return obliquity;
}


GPlatesAppLogic::DeformationStrainRate
GPlatesAppLogic::PlateBoundaryStat::get_strain_rate(
		const TopologyPointLocation &topology_point_location) const
{
	boost::optional<TopologyPointLocation::network_location_type> network_location =
			topology_point_location.located_in_resolved_network();
	if (!network_location)
	{
		return DeformationStrainRate();  // zero deformation
	}

	boost::optional<ResolvedTriangulation::DeformationInfo> deformation_info = network_location->first->
			get_triangulation_network().calculate_deformation(d_boundary_point, network_location->second);
	if (!deformation_info)
	{
		return DeformationStrainRate();  // zero deformation
	}

	return deformation_info->get_strain_rate();
}
