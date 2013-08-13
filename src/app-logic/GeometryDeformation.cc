/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <utility>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "GeometryDeformation.h"

#include "GeometryUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTriangulationNetwork.h"

#include "global/AbortException.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/CalculateVelocity.h"
#include "maths/Centroid.h"
#include "maths/MathsUtils.h"
#include "maths/SmallCircleBounds.h"
#include "maths/types.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Predicate to test if the geometry *points* bounding small circle intersects the
		 * resolved network bounding small circle.
		 */
		class IntersectGeometryPointsAndResolvedNetworkSmallCircleBounds :
				public std::unary_function<ResolvedTopologicalNetwork::non_null_ptr_type, bool>
		{
		public:

			explicit
			IntersectGeometryPointsAndResolvedNetworkSmallCircleBounds(
					const GPlatesMaths::BoundingSmallCircle *geometry_points_bounding_small_circle) :
				d_geometry_points_bounding_small_circle(geometry_points_bounding_small_circle)
			{  }

			bool
			operator()(
					const ResolvedTopologicalNetwork::non_null_ptr_type &rtn) const
			{
				const GPlatesMaths::BoundingSmallCircle &network_bounding_small_circle =
						rtn->get_triangulation_network().get_boundary_polygon()
								->get_bounding_small_circle();

				return intersect(network_bounding_small_circle, *d_geometry_points_bounding_small_circle);
			}

		private:

			const GPlatesMaths::BoundingSmallCircle *d_geometry_points_bounding_small_circle;
		};


		/**
		 * Interpolates the stage rotations at the vertices of a delaunay face according to
		 * the barycentric coordinates of a point inside the face.
		 */
		GPlatesMaths::FiniteRotation
		get_interpolated_stage_rotation(
				const ResolvedTriangulation::Delaunay_2::Face_handle &delaunay_face,
				const ResolvedTriangulation::delaunay_coord_2_type &barycentric_coord_vertex_1,
				const ResolvedTriangulation::delaunay_coord_2_type &barycentric_coord_vertex_2,
				const ResolvedTriangulation::delaunay_coord_2_type &barycentric_coord_vertex_3,
				const double &initial_time,
				const double &final_time,
				const ReconstructionTreeCreator &reconstruction_tree_creator)
		{
			const ReconstructionTree::non_null_ptr_to_const_type initial_reconstruction_tree =
					reconstruction_tree_creator.get_reconstruction_tree(initial_time);
			const ReconstructionTree::non_null_ptr_to_const_type final_reconstruction_tree =
					reconstruction_tree_creator.get_reconstruction_tree(final_time);

			// Get the rigid finite rotation of vertex 1 of the delaunay face from
			// 'current_time' to 'next_time'.
			const GPlatesModel::integer_plate_id_type plate_id_1 =
					delaunay_face->vertex(0)->get_plate_id();
			const GPlatesMaths::FiniteRotation &finite_rotation_initial_time_1 =
					initial_reconstruction_tree->get_composed_absolute_rotation(plate_id_1).first;
			const GPlatesMaths::FiniteRotation &finite_rotation_final_time_1 =
					final_reconstruction_tree->get_composed_absolute_rotation(plate_id_1).first;
			const GPlatesMaths::FiniteRotation finite_rotation_initial_time_to_final_time_1 =
					GPlatesMaths::compose(
							finite_rotation_final_time_1,
							GPlatesMaths::get_reverse(finite_rotation_initial_time_1));

			// Get the rigid finite rotation of vertex 2 of the delaunay face from
			// 'current_time' to 'next_time'.
			const GPlatesModel::integer_plate_id_type plate_id_2 =
					delaunay_face->vertex(1)->get_plate_id();
			const GPlatesMaths::FiniteRotation &finite_rotation_initial_time_2 =
					initial_reconstruction_tree->get_composed_absolute_rotation(plate_id_2).first;
			const GPlatesMaths::FiniteRotation &finite_rotation_final_time_2 =
					final_reconstruction_tree->get_composed_absolute_rotation(plate_id_2).first;
			const GPlatesMaths::FiniteRotation finite_rotation_initial_time_to_final_time_2 =
					GPlatesMaths::compose(
							finite_rotation_final_time_2,
							GPlatesMaths::get_reverse(finite_rotation_initial_time_2));

			// Get the rigid finite rotation of vertex 1 of the delaunay face from
			// 'current_time' to 'next_time'.
			const GPlatesModel::integer_plate_id_type plate_id_3 =
					delaunay_face->vertex(2)->get_plate_id();
			const GPlatesMaths::FiniteRotation &finite_rotation_initial_time_3 =
					initial_reconstruction_tree->get_composed_absolute_rotation(plate_id_3).first;
			const GPlatesMaths::FiniteRotation &finite_rotation_final_time_3 =
					final_reconstruction_tree->get_composed_absolute_rotation(plate_id_3).first;
			const GPlatesMaths::FiniteRotation finite_rotation_initial_time_to_final_time_3 =
					GPlatesMaths::compose(
							finite_rotation_final_time_3,
							GPlatesMaths::get_reverse(finite_rotation_initial_time_3));

			// Interpolate the vertex stage rotations.
			return GPlatesMaths::interpolate(
					finite_rotation_initial_time_to_final_time_1,
					finite_rotation_initial_time_to_final_time_2,
					finite_rotation_initial_time_to_final_time_3,
					barycentric_coord_vertex_1,
					barycentric_coord_vertex_2,
					barycentric_coord_vertex_3);
		}


		/**
		 * Returns the stage rotation for the specified rigid block.
		 */
		GPlatesMaths::FiniteRotation
		get_rigid_block_stage_rotation(
				const ResolvedTriangulation::Network::RigidBlock &rigid_block,
				const double &initial_time,
				const double &final_time,
				const ReconstructionTreeCreator &reconstruction_tree_creator)
		{
			const ReconstructionTree::non_null_ptr_to_const_type initial_reconstruction_tree =
					reconstruction_tree_creator.get_reconstruction_tree(initial_time);
			const ReconstructionTree::non_null_ptr_to_const_type final_reconstruction_tree =
					reconstruction_tree_creator.get_reconstruction_tree(final_time);

			// Get the reconstruction plate id of rigid block.
			GPlatesModel::integer_plate_id_type reconstruction_plate_id = 0;
			if (rigid_block.get_reconstructed_feature_geometry()->reconstruction_plate_id())
			{
				reconstruction_plate_id =
						rigid_block.get_reconstructed_feature_geometry()->reconstruction_plate_id().get();
			}

			const GPlatesMaths::FiniteRotation &finite_rotation_initial_time =
					initial_reconstruction_tree->get_composed_absolute_rotation(reconstruction_plate_id).first;
			const GPlatesMaths::FiniteRotation &finite_rotation_final_time =
					final_reconstruction_tree->get_composed_absolute_rotation(reconstruction_plate_id).first;

			return GPlatesMaths::compose(
					finite_rotation_final_time,
					GPlatesMaths::get_reverse(finite_rotation_initial_time));
		}
	}
}


GPlatesAppLogic::GeometryDeformation::ResolvedNetworkTimeSpan::ResolvedNetworkTimeSpan(
		const double &begin_time,
		const double &end_time,
		const double &time_increment) :
	d_begin_time(begin_time),
	d_end_time(end_time),
	d_time_increment(time_increment)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			begin_time > end_time && time_increment > 0,
			GPLATES_ASSERTION_SOURCE);

	// Calculate the number of time slots ending at end_time and beginning at, or up to
	// one time increment before, begin_time.
	const unsigned int num_time_slots = calc_num_time_slots();

	// Modify begin time so it begins on an integer time slot.
	// Hence the begin time can be earlier in the past than the actual begin time passed by the caller.
	//
	// For example, for begin_time = 12.1 and end_time = 10.0 and time_increment = 1.0,
	// we get four time slots which are at times 13.0, 12.0, 11.0 and 10.0 and they bound the
	// three time intervals [13.0, 12.0], [12.0, 11.0] and [11.0, 10.0].
	// So 'd_begin_time' = 13.0 and 'd_end_time' = 10.0.
	d_begin_time = end_time + (num_time_slots - 1) * time_increment;

	// Pre-allocate our time span to the number of time slots.
	d_resolved_networks_time_sequence.resize(num_time_slots);
}


unsigned int
GPlatesAppLogic::GeometryDeformation::ResolvedNetworkTimeSpan::calc_num_time_slots() const
{
	// Casting to 'int' is optimised on Visual Studio compared to 'unsigned int'.
	// The '1' converts intervals to slots (eg, two intervals is bounded by three fence posts or slots).
	// The '1-1e-6' rounds up to the nearest integer.
	//
	// For example, for begin_time = 12.1 and end_time = 10.0 and time_increment = 1.0,
	// we get four time slots which are at times 13.0, 12.0, 11.0 and 10.0 and they bound the
	// three time intervals [13.0, 12.0], [12.0, 11.0] and [11.0, 10.0].
	const unsigned int num_time_slots = 1 + static_cast<int>(
			(1 - 1e-6) + (d_begin_time - d_end_time) / d_time_increment);

	return num_time_slots;
}


boost::optional<unsigned int>
GPlatesAppLogic::GeometryDeformation::ResolvedNetworkTimeSpan::get_time_slot(
		const double &time) const
{
	if (GPlatesMaths::are_geo_times_approximately_equal(time, d_begin_time))
	{
		return static_cast<unsigned int>(0);
	}

	if (GPlatesMaths::are_geo_times_approximately_equal(time, d_end_time))
	{
		// Note that there's always at least one time slot.
		return get_num_time_slots() - 1;
	}

	if (time > d_begin_time || time < d_end_time)
	{
		// Outside time span.
		return boost::none;
	}

	double delta_time_div_time_increment;
	const double delta_time_mod_time_increment =
			std::modf((d_begin_time - time) / d_time_increment, &delta_time_div_time_increment);

	// See if the specified time lies at an integer multiple of time increment.
	if (!GPlatesMaths::are_almost_exactly_equal(delta_time_mod_time_increment, 0.0))
	{
		return boost::none;
	}

	// Convert to integer (it's already integral - the 0.5 is just to avoid numerical precision issues).
	const unsigned int time_slot = static_cast<int>(delta_time_div_time_increment + 0.5);

	return time_slot;
}


const GPlatesAppLogic::GeometryDeformation::ResolvedNetworkTimeSpan::rtn_seq_type &
GPlatesAppLogic::GeometryDeformation::ResolvedNetworkTimeSpan::get_resolved_networks_time_slot(
		unsigned int time_slot) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			time_slot < d_resolved_networks_time_sequence.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_resolved_networks_time_sequence[time_slot];
}


boost::optional<const GPlatesAppLogic::GeometryDeformation::ResolvedNetworkTimeSpan::rtn_seq_type &>
GPlatesAppLogic::GeometryDeformation::ResolvedNetworkTimeSpan::get_nearest_resolved_networks_time_slot(
		const double &time) const
{
	if (time > d_begin_time || time < d_end_time)
	{
		// Outside time span.
		return boost::none;
	}

	// The 0.5 is to round to the nearest integer.
	// Casting to 'int' is optimised on Visual Studio compared to 'unsigned int'.
	const unsigned int time_slot = static_cast<int>(0.5 + (d_begin_time - time) / d_time_increment);

	return get_resolved_networks_time_slot(time_slot);
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometryTimeSpan(
		const ResolvedNetworkTimeSpan &resolved_network_time_span,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &feature_present_day_geometry,
		GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id) :
	d_reconstruction_plate_id(feature_reconstruction_plate_id),
	d_geometry_type(GPlatesMaths::GeometryType::NONE),
	d_have_initialised_forward_time_accumulated_deformation_information(false)
{
	// Get the points of the present day geometry.
	std::vector<GPlatesMaths::PointOnSphere> present_day_geometry_points;
	d_geometry_type = GeometryUtils::get_geometry_points(
			*feature_present_day_geometry,
			present_day_geometry_points);

	initialise_time_windows(
			resolved_network_time_span,
			reconstruction_tree_creator,
			present_day_geometry_points);

	// We should have at least one time window (containing the present day geometry).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_time_windows.empty(),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::initialise_time_windows(
		const ResolvedNetworkTimeSpan &resolved_network_time_span,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const std::vector<GPlatesMaths::PointOnSphere> &present_day_geometry_points)
{
	//PROFILE_FUNC();

	// Create the last (most recent) time window whose end time is present day.
	// This time window will handle rigid rotations from present day back to the end of the
	// most recent deformation period affecting the geometry.
	d_time_windows.push_back(
			TimeWindow(0/*end_time*/, resolved_network_time_span.get_time_increment()));
	// Add a geometry sample for the present day geometry.
	TimeWindow &present_day_time_window = d_time_windows.back();
	present_day_time_window.add_geometry_sample(present_day_geometry_points);

	// A reference to the current time window.
	// This gets set to boost::none when a time slot is encountered where the geometry does not
	// intersect any resolved networks - in which case the next time slot will create a new time window.
	boost::optional<TimeWindow &> current_time_window;
	// If the resolved network time span exists at present day then we can add to the present day time window.
	if (GPlatesMaths::are_geo_times_approximately_equal(resolved_network_time_span.get_end_time(), 0.0))
	{
		current_time_window = present_day_time_window;
	}

	// Iterate over the resolved network time span going *backwards* in time from the end of the
	// resolved networks time span (most recent) to the beginning (least recent).
	const unsigned int num_time_slots = resolved_network_time_span.get_num_time_slots();
	for (/*signed*/ int time_slot = num_time_slots - 1; time_slot >= 0; --time_slot)
	{
		const double current_time = resolved_network_time_span.get_time(time_slot);
		const double next_time = current_time + resolved_network_time_span.get_time_increment();

		// NOTE: If the feature does not exist at the current time slot we still deform it.
		// This is an issue to do with storing feature geometry in present day coordinates.
		// We need to be able to change the feature's end time without having it change the position
		// of the feature's deformed geometry prior to the feature's end (disappearance) time.
#if 0
		if (GPlatesPropertyValues::GeoTimeInstant(current_time).is_strictly_earlier_than(feature_begin_time) ||
			GPlatesPropertyValues::GeoTimeInstant(current_time).is_strictly_later_than(feature_end_time))
		{
			// Finished with the current time window (if any).
			current_time_window = boost::none;
			continue;
		}
#endif

		// Get the resolved networks for the current time slot.
		ResolvedNetworkTimeSpan::rtn_seq_type resolved_networks =
				resolved_network_time_span.get_resolved_networks_time_slot(time_slot);

		// If there are no networks for the current time slot then continue to the next time slot.
		// Geometry will not be stored for the current time.
		if (resolved_networks.empty())
		{
			// Finished with the current time window (if any).
			current_time_window = boost::none;
			continue;
		}

		// Get the geometry points for the current time.
		std::vector<GPlatesMaths::PointOnSphere> current_geometry_points;
		// If we have a current time window then it contains the current geometry sample.
		if (current_time_window)
		{
			// Copy the points in the first (least recent) geometry sample of the current time window.
			const GeometrySample &current_geometry_sample =
					current_time_window->get_geometry_sample_at_begin_time();
			current_geometry_points.reserve(current_geometry_sample.points.size());
			std::copy(
					current_geometry_sample.points.begin(),
					current_geometry_sample.points.end(),
					std::back_inserter(current_geometry_points));
		}
		else // rotate the geometry sample from the previous time window...
		{
			// Get the previous (least recent) time window.
			const TimeWindow &prev_time_window = d_time_windows.front();

			// Rotate the first (least recent) geometry sample in the previous time window.
			const GeometrySample &geometry_sample = prev_time_window.get_geometry_sample_at_begin_time();
			rigid_reconstruct(
					current_geometry_points,
					geometry_sample.points,
					reconstruction_tree_creator,
					prev_time_window.get_begin_time()/*initial_time*/,
					current_time/*final_time*/);
		}

		//
		// As an optimisation, remove those networks that the current geometry points do not intersect.
		//
		GPlatesMaths::BoundingSmallCircleBuilder current_geometry_points_small_circle_bounds_builder(
				GPlatesMaths::Centroid::calculate_points_centroid(
						current_geometry_points.begin(),
						current_geometry_points.end()));
		// Note that we don't need to worry about adding great circle arcs (if the geometry type is a
		// polyline or polygon) because we only test if the points intersect the resolved networks.
		// If interior arc sub-segment of a great circle arc (polyline/polygon edge) intersects
		// resolved network it doesn't matter (only the arc end points matter).
		std::vector<GPlatesMaths::PointOnSphere>::iterator curr_geometry_points_iter =
				current_geometry_points.begin();
		std::vector<GPlatesMaths::PointOnSphere>::iterator curr_geometry_points_end =
				current_geometry_points.end();
		for ( ; curr_geometry_points_iter != curr_geometry_points_end; ++curr_geometry_points_iter)
		{
			current_geometry_points_small_circle_bounds_builder.add(*curr_geometry_points_iter);
		}
		const GPlatesMaths::BoundingSmallCircle current_geometry_points_small_circle_bounds =
				current_geometry_points_small_circle_bounds_builder.get_bounding_small_circle();
		resolved_networks.erase(
				std::remove_if(
						resolved_networks.begin(),
						resolved_networks.end(),
						std::not1(IntersectGeometryPointsAndResolvedNetworkSmallCircleBounds(
								&current_geometry_points_small_circle_bounds))),
				resolved_networks.end());

		// If none of the resolved networks intersect the geometry points at the current time then
		// continue to the next time slot.
		// Geometry will not be stored for the current time.
		if (resolved_networks.empty())
		{
			// Finished with the current time window (if any).
			current_time_window = boost::none;
			continue;
		}

		// We've excluded those resolved networks that can't possibly intersect the current geometry points.
		// This doesn't mean the remaining networks will definitely intersect though - they might not.

		// An array to store deformed geometry points for the next time slot.
		// Starts out with all points being boost::none - and only deformed points will get filled.
		std::vector< boost::optional<GPlatesMaths::PointOnSphere> >
				deformed_geometry_points(current_geometry_points.size());

		// An array to store the delaunay triangulation faces that the current geometry points are in.
		// Starts out with all points being boost::none - and only points in deforming regions will
		// get filled - and not all deformed points will get filled either (due to interior rigid blocks).
		std::vector< boost::optional<ResolvedTriangulation::Delaunay_2::Face_handle> >
				current_delaunay_faces(current_geometry_points.size());

		// Keep track of number of deformed geometry points for the current time.
		unsigned int num_deformed_geometry_points = 0;

		// Iterate over the current geometry points and attempt to deform them.
		for (unsigned int current_geometry_point_index = 0;
			current_geometry_point_index < current_geometry_points.size();
			++current_geometry_point_index)
		{
			const GPlatesMaths::PointOnSphere &current_geometry_point =
					current_geometry_points[current_geometry_point_index];

			// Iterate over the resolved networks for the current time.
			ResolvedNetworkTimeSpan::rtn_seq_type::const_iterator resolved_networks_iter = resolved_networks.begin();
			ResolvedNetworkTimeSpan::rtn_seq_type::const_iterator resolved_networks_end = resolved_networks.end();
			for ( ; resolved_networks_iter != resolved_networks_end; ++resolved_networks_iter)
			{
				const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_network = *resolved_networks_iter;

				// Find the delaunay triangulation face containing the current geometry point (if any)
				// and calculate the barycentric coordinates of its vertices.
				ResolvedTriangulation::delaunay_coord_2_type barycentric_coord_vertex_1;
				ResolvedTriangulation::delaunay_coord_2_type barycentric_coord_vertex_2;
				ResolvedTriangulation::delaunay_coord_2_type barycentric_coord_vertex_3;
				boost::optional<ResolvedTriangulation::Delaunay_2::Face_handle> delaunay_face =
						resolved_network->get_triangulation_network().calc_delaunay_barycentric_coordinates(
								barycentric_coord_vertex_1,
								barycentric_coord_vertex_2,
								barycentric_coord_vertex_3,
								current_geometry_point);
				if (delaunay_face)
				{
					// Record the delaunay face.
					// It might get used later if deformation information is requested by our clients.
					// But we don't want to calculate deformation info if it's never needed.
					current_delaunay_faces[current_geometry_point_index] = delaunay_face.get();

					// Interpolate the stage rotations at the face vertices according to the barycentric weights.
					const GPlatesMaths::FiniteRotation interpolated_stage_rotation =
							get_interpolated_stage_rotation(
									delaunay_face.get(),
									barycentric_coord_vertex_1,
									barycentric_coord_vertex_2,
									barycentric_coord_vertex_3,
									current_time/*initial_time*/,
									next_time/*final_time*/,
									reconstruction_tree_creator);

					// Deform the current geometry point.
					const GPlatesMaths::PointOnSphere deformed_geometry_point =
							interpolated_stage_rotation * current_geometry_point;

					// Record the deformed point.
					deformed_geometry_points[current_geometry_point_index] = deformed_geometry_point;
					++num_deformed_geometry_points;

					// Finished searching resolved networks for the current geometry point.
					break;
				}

				// The current geometry point is not in the *deforming region* of the network
				// but it could still be inside a rigid block in the interior of the network.
				boost::optional<const ResolvedTriangulation::Network::RigidBlock &> rigid_block =
						resolved_network->get_triangulation_network().is_point_in_a_rigid_block(
								current_geometry_point);
				if (rigid_block)
				{
					const GPlatesMaths::FiniteRotation rigid_block_stage_rotation =
							get_rigid_block_stage_rotation(
									rigid_block.get(),
									current_time/*initial_time*/,
									next_time/*final_time*/,
									reconstruction_tree_creator);

					// Deform the current geometry point.
					const GPlatesMaths::PointOnSphere deformed_geometry_point =
							rigid_block_stage_rotation * current_geometry_point;

					// Record the deformed point.
					deformed_geometry_points[current_geometry_point_index] = deformed_geometry_point;
					++num_deformed_geometry_points;

					// Finished searching resolved networks for the current geometry point.
					break;
				}

				// The current geometry point is outside the network so continue searching
				// the next resolved network.
			}
		}

		// If none of the resolved networks intersect the current geometry points then
		// continue to the next time slot.
		// Geometry will not be stored for the current time.
		if (num_deformed_geometry_points == 0)
		{
			// Finished with the current time window (if any).
			current_time_window = boost::none;
			continue;
		}

		// If we get here then at least one geometry point was deformed.

		// The geometry points for the next geometry sample.
		std::vector<GPlatesMaths::PointOnSphere> next_geometry_points;
		next_geometry_points.reserve(current_geometry_points.size());

		// If not all geometry points were deformed then rigidly rotate those that were not.
		if (num_deformed_geometry_points < current_geometry_points.size())
		{
			// Get the rigid finite rotation from 'current_time' to 'next_time' - used for those geometry
			// points that did not intersect any resolved networks and hence must be rigidly rotated.
			const GPlatesMaths::FiniteRotation &finite_rotation_current_time =
					reconstruction_tree_creator.get_reconstruction_tree(current_time)
							->get_composed_absolute_rotation(d_reconstruction_plate_id).first;
			const GPlatesMaths::FiniteRotation &finite_rotation_next_time =
					reconstruction_tree_creator.get_reconstruction_tree(next_time)
							->get_composed_absolute_rotation(d_reconstruction_plate_id).first;
			const GPlatesMaths::FiniteRotation finite_rotation_current_time_to_next_time =
					GPlatesMaths::compose(
							finite_rotation_next_time,
							GPlatesMaths::get_reverse(finite_rotation_current_time));

			for (unsigned int geometry_point_index = 0;
				geometry_point_index < current_geometry_points.size();
				++geometry_point_index)
			{
				if (deformed_geometry_points[geometry_point_index])
				{
					// Add deformed geometry point.
					next_geometry_points.push_back(
							deformed_geometry_points[geometry_point_index].get());
				}
				else
				{
					// Add rigidly rotated geometry point.
					next_geometry_points.push_back(
							finite_rotation_current_time_to_next_time *
									current_geometry_points[geometry_point_index]);
				}
			}
		}
		else // all geometry points were deformed...
		{
			// Just copy the deformed points into the next geometry sample.
			for (unsigned int geometry_point_index = 0;
				geometry_point_index < current_geometry_points.size();
				++geometry_point_index)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						deformed_geometry_points[geometry_point_index],
						GPLATES_ASSERTION_SOURCE);

				next_geometry_points.push_back(
						deformed_geometry_points[geometry_point_index].get());
			}
		}

		// Create a new time window or continue an existing one.
		if (!current_time_window)
		{
			// Add to the front because we are going backwards in time.
			d_time_windows.push_front(
					TimeWindow(current_time/*end_time*/, resolved_network_time_span.get_time_increment()));
			current_time_window = d_time_windows.front();

			// Add the current geometry sample at the end of the time window.
			current_time_window->add_geometry_sample(current_geometry_points);
		}

		// Set the recorded delaunay faces on the current geometry sample.
		current_time_window->get_geometry_sample_at_begin_time().set_delaunay_faces(current_delaunay_faces);

		// Add a new geometry sample for the next time slot.
		current_time_window->add_geometry_sample(next_geometry_points);
	}
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::initialise_forward_time_accumulated_deformation_information()
{
	d_have_initialised_forward_time_accumulated_deformation_information = true;

	boost::optional<const GeometrySample &> prev_geometry_sample;

	// Iterate through the time windows going forward in time.
	// Note that the time windows are ordered moving forward in time.
	// In other words pretty much everything is going forward in time from earliest (or least recent)
	// to latest (or most recent).
	time_window_seq_type::iterator time_windows_iter = d_time_windows.begin();
	time_window_seq_type::iterator time_windows_end = d_time_windows.end();
	for ( ; time_windows_iter != time_windows_end; ++time_windows_iter)
	{
		TimeWindow &time_window = *time_windows_iter;

		const unsigned int num_geometry_samples = time_window.get_num_geometry_samples();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					num_geometry_samples != 0,
					GPLATES_ASSERTION_SOURCE);

		unsigned int geometry_sample_index;
		if (!prev_geometry_sample)
		{
			// Start the accumulation at the first geometry sample of the first time window.
			prev_geometry_sample = time_window.get_geometry_sample(0);

			// It's possible there's only one window with *one* geometry sample representing the
			// present day geometry - this happens if there's no deformation of the geometry.
			if (num_geometry_samples < 2)
			{
				continue;
			}
			geometry_sample_index = 1;
		}
		else
		{
			geometry_sample_index = 0;
		}

		// Iterate over the geometry samples in the current time window (forward in time).
		for ( ; geometry_sample_index < num_geometry_samples; ++geometry_sample_index)
		{
			GeometrySample &geometry_sample = time_window.get_geometry_sample(geometry_sample_index);

			// The number of points in each geometry sample should be the same.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						prev_geometry_sample->get_deformation_infos().size() ==
							geometry_sample.get_deformation_infos().size(),
						GPLATES_ASSERTION_SOURCE);

			// Iterate over the previous and current geometry sample points.
			const unsigned int num_points = geometry_sample.get_deformation_infos().size();
			for (unsigned int point_index = 0; point_index < num_points; ++point_index)
			{
				const DeformationInfo &prev_deformation_info =
						prev_geometry_sample->get_deformation_infos()[point_index];
				DeformationInfo &deformation_info = geometry_sample.get_deformation_infos()[point_index];

				// Compute new strain for the current geometry sample using the strains at the
				// previous sample and the strain rates at the current sample.
				// NOTE: 3.15569e13 seconds in 1 My
				// The assumption being that the velocities 
				const double S22 = prev_deformation_info.S22 + deformation_info.SR22 * 3.15569e13;
				const double S33 = prev_deformation_info.S33 + deformation_info.SR33 * 3.15569e13;
				const double S23 = prev_deformation_info.S23 + deformation_info.SR23 * 3.15569e13;

				// Set the new strain at the current geometry sample.
				deformation_info.S22 = S22;
				deformation_info.S33 = S33;
				deformation_info.S23 = S23;

	  			// Compute the Dilitational and Second invariant strain for the current geometry sample.
				const double dilitation_strain = S22 + S33;
				const double second_invariant_strain = std::sqrt(S22 * S22 + S33 * S33 + 2.0 * S23 * S23);

				// Principle strains.
				const double S_DIR = 0.5 * std::atan2(2.0 * S23, S33 - S22);
				const double S_variation = std::sqrt(S23*S23 + 0.25*(S33-S22)*(S33-S22));
				const double S1 = 0.5*(S22+S33) + S_variation;
				const double S2 = 0.5*(S22+S33) - S_variation;

				// Set the new values.
				deformation_info.dilitation_strain = dilitation_strain;
				deformation_info.second_invariant_strain = second_invariant_strain;
				deformation_info.S1 = S1;
				deformation_info.S2 = S2;
				deformation_info.S_DIR = S_DIR;
			}

			prev_geometry_sample = geometry_sample;
		}
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_geometry(
		const double &reconstruction_time,
		const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	// Get the deformed (or rigidly rotated) geometry points.
	std::vector<GPlatesMaths::PointOnSphere> geometry_points;
	get_geometry_sample_data(reconstruction_time, reconstruction_tree_creator, geometry_points);

	// Return as a GeometryOnSphere.
	return create_geometry_on_sphere(geometry_points);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_geometry_and_deformation_information(
		const double &reconstruction_time,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		std::vector<DeformationInfo> &deformation_info_points)
{
	// Get the deformed (or rigidly rotated) geometry points.
	std::vector<GPlatesMaths::PointOnSphere> geometry_points;
	get_geometry_sample_data(
			reconstruction_time,
			reconstruction_tree_creator,
			geometry_points,
			deformation_info_points);

	// Return as a GeometryOnSphere.
	return create_geometry_on_sphere(geometry_points);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_deformation_information(
		std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		std::vector<DeformationInfo> &deformation_info_points,
		const double &reconstruction_time,
		const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	// Get the deformed (or rigidly rotated) geometry points and per-point deformation information.
	get_geometry_sample_data(
			reconstruction_time,
			reconstruction_tree_creator,
			geometry_points,
			deformation_info_points);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_velocities(
		std::vector<GPlatesMaths::PointOnSphere> &domain_points,
		std::vector<GPlatesMaths::Vector3D> &velocities,
		std::vector< boost::optional<const ReconstructionGeometry *> > &surfaces,
		const double &reconstruction_time,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ResolvedNetworkTimeSpan &resolved_network_time_span)
{
	// Get the geometry (domain) points.
	get_geometry_sample_data(reconstruction_time, reconstruction_tree_creator, domain_points);

	//
	// Calculate the velocities at the geometry (domain) points.
	//

	velocities.reserve(domain_points.size());
	surfaces.reserve(domain_points.size());

	// Get the resolved topological networks if the reconstruction time is within the time span.
	boost::optional<PlateVelocityUtils::TopologicalNetworksVelocities> resolved_networks_query;
	boost::optional<const ResolvedNetworkTimeSpan::rtn_seq_type &> resolved_networks =
			resolved_network_time_span.get_nearest_resolved_networks_time_slot(reconstruction_time);
	if (resolved_networks)
	{
		resolved_networks_query = boost::in_place(resolved_networks.get());
	}

	// The finite rotations used to calculate velocities by rigid rotation.
	// It's optional because we only create if needed.
	boost::optional< std::pair<GPlatesMaths::FiniteRotation, GPlatesMaths::FiniteRotation> > finite_rotations;

	// Iterate over the domain points and calculate their velocities (and surfaces).
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator domain_points_iter = domain_points.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator domain_points_end = domain_points.end();
	for ( ; domain_points_iter != domain_points_end; ++domain_points_iter)
	{
		const GPlatesMaths::PointOnSphere &domain_point = *domain_points_iter;

		// Check whether domain point is inside any topological networks.
		// This includes points inside interior rigid blocks in the networks.
		if (resolved_networks_query)
		{
			boost::optional< std::pair<const ReconstructionGeometry *, GPlatesMaths::Vector3D > >
					network_velocity = resolved_networks_query->calculate_velocity(domain_point);
			if (network_velocity)
			{
				// The network 'component' could be the network's deforming region or an interior
				// rigid block in the network.
				const ReconstructionGeometry *network_component = network_velocity->first;
				const GPlatesMaths::Vector3D &velocity_vector = network_velocity->second;

				velocities.push_back(velocity_vector);
				surfaces.push_back(network_component);

				// Continue to the next domain point.
				continue;
			}
		}

		// Domain point was not in a resolved network (or there were no resolved networks).
		// So calculate velocity using rigid rotation.

		if (!finite_rotations)
		{
			finite_rotations = boost::in_place(
					reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time)
							->get_composed_absolute_rotation(d_reconstruction_plate_id).first,
					// FIXME:  Should this '1' should be user controllable? ...
					reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time + 1)
							->get_composed_absolute_rotation(d_reconstruction_plate_id).first);
		}

		// Calculate the velocity.
		const GPlatesMaths::Vector3D velocity_vector =
				GPlatesMaths::calculate_velocity_vector(
						domain_point,
						finite_rotations->first,   // at reconstruction_time
						finite_rotations->second); // at reconstruction_time + 1

		// Add the velocity - there was no surface (ie, resolved network) intersection though.
		velocities.push_back(velocity_vector);
		surfaces.push_back(boost::none/*surface*/);
	}
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_geometry_sample_data(
		const double &reconstruction_time,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		boost::optional<std::vector<DeformationInfo> &> deformation_info_points)
{
	// If deformation information has been requested then first generate the information if
	// it hasn't already been generated.
	if (deformation_info_points)
	{
		if (!d_have_initialised_forward_time_accumulated_deformation_information)
		{
			initialise_forward_time_accumulated_deformation_information();
		}
	}

	// Iterate through the time windows.
	// Note that the time windows are ordered moving forward in time.
	// In other words pretty much everything is going forward in time from earliest (or least recent)
	// to latest (or most recent).
	//
	// Also note that the first (least recent) time window captures all reconstruction times in the
	// very distant past and the last (least recent) time window has an end time at present day.
	// So the time windows should capture all possible (non-negative) reconstruction times.
	//
	time_window_seq_type::const_iterator time_windows_iter = d_time_windows.begin();
	time_window_seq_type::const_iterator time_windows_end = d_time_windows.end();
	for ( ; time_windows_iter != time_windows_end; ++time_windows_iter)
	{
		const TimeWindow &time_window = *time_windows_iter;

		// If we've found the right time window....
		// Note the we use '>=' to capture a reconstruction time of zero with the
		// last (most recent) time window (which has an end time of present day).
		if (reconstruction_time >= time_window.get_end_time())
		{
			const double time_window_begin_time = time_window.get_begin_time();

			// See whether we are within the current time window.
			if (reconstruction_time <= time_window_begin_time)
			{
				const GeometrySample &geometry_sample =
						time_window.get_geometry_sample(reconstruction_time);

				// Copy the geometry sample points to the caller's array.
				geometry_points.reserve(geometry_sample.points.size());
				std::copy(
						geometry_sample.points.begin(),
						geometry_sample.points.end(),
						std::back_inserter(geometry_points));

				// Also copy the per-point deformation information if it was requested.
				if (deformation_info_points)
				{
					deformation_info_points->reserve(geometry_sample.get_deformation_infos().size());
					std::copy(
							geometry_sample.get_deformation_infos().begin(),
							geometry_sample.get_deformation_infos().end(),
							std::back_inserter(deformation_info_points.get()));
				}

				return;
			}
			else // the reconstruction time is in a rigid time span...
			{
				// Get the geometry sample at the begin time of the current time window.
				const GeometrySample &geometry_sample = time_window.get_geometry_sample_at_begin_time();

				// Reconstruct the geometry sample from the begin time to the reconstruction time
				// (which is earlier than the current time window).
				rigid_reconstruct(
						geometry_points,
						geometry_sample.points,
						reconstruction_tree_creator,
						time_window_begin_time/*initial_time*/,
						reconstruction_time/*final_time*/);

				// Also copy the per-point deformation information if it was requested.
				// There is no deformation during rigid time spans so the deformation at the end
				// of the previous (earlier) time window is the same as at the beginning of the
				// next (later) time window.
				if (deformation_info_points)
				{
					deformation_info_points->reserve(geometry_sample.get_deformation_infos().size());
					std::copy(
							geometry_sample.get_deformation_infos().begin(),
							geometry_sample.get_deformation_infos().end(),
							std::back_inserter(deformation_info_points.get()));
				}

				return;
			}
		}
	}

	// We shouldn't be able to get here since the begin time of the first time window (the time
	// window in the most distant past) is set to the distant past and so should capture all
	// reconstruction times not caught by later (more recent) time windows.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::rigid_reconstruct(
		std::vector<GPlatesMaths::PointOnSphere> &rotated_geometry_points,
		const std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time) const
{
	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);

	const GPlatesMaths::FiniteRotation &rotation =
			reconstruction_tree->get_composed_absolute_rotation(
					d_reconstruction_plate_id).first;

	std::vector<GPlatesMaths::PointOnSphere>::const_iterator geometry_points_iter = geometry_points.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator geometry_points_end = geometry_points.end();
	rotated_geometry_points.reserve(geometry_points.size());
	for ( ; geometry_points_iter != geometry_points_end; ++geometry_points_iter)
	{
		rotated_geometry_points.push_back(
				GPlatesMaths::PointOnSphere(
						rotation * geometry_points_iter->position_vector()));
	}
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::rigid_reconstruct(
		std::vector<GPlatesMaths::PointOnSphere> &rotated_geometry_points,
		const std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &initial_time,
		const double &final_time) const
{
	const ReconstructionTree::non_null_ptr_to_const_type initial_reconstruction_tree =
			reconstruction_tree_creator.get_reconstruction_tree(initial_time);
	const ReconstructionTree::non_null_ptr_to_const_type final_reconstruction_tree =
			reconstruction_tree_creator.get_reconstruction_tree(final_time);

	const GPlatesMaths::FiniteRotation &present_day_to_initial_rotation =
			initial_reconstruction_tree->get_composed_absolute_rotation(
					d_reconstruction_plate_id).first;
	const GPlatesMaths::FiniteRotation &present_day_to_final_rotation =
			final_reconstruction_tree->get_composed_absolute_rotation(
					d_reconstruction_plate_id).first;

	const GPlatesMaths::FiniteRotation initial_to_final_rotation =
			GPlatesMaths::compose(
					present_day_to_final_rotation,
					GPlatesMaths::get_reverse(present_day_to_initial_rotation));

	std::vector<GPlatesMaths::PointOnSphere>::const_iterator geometry_points_iter = geometry_points.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator geometry_points_end = geometry_points.end();
	rotated_geometry_points.reserve(geometry_points.size());
	for ( ; geometry_points_iter != geometry_points_end; ++geometry_points_iter)
	{
		rotated_geometry_points.push_back(
				GPlatesMaths::PointOnSphere(
						initial_to_final_rotation * geometry_points_iter->position_vector()));
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::create_geometry_on_sphere(
		const std::vector<GPlatesMaths::PointOnSphere> &geometry_points) const
{
	// Create a GeometryOnSphere from the geometry points.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity geometry_validity;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
			geometry_on_sphere = GPlatesUtils::create_geometry_on_sphere(
					d_geometry_type,
					geometry_points,
					geometry_validity);

	// It's possible that the geometry no longer satisfies geometry on sphere construction validity
	// (eg, has no arc segments with antipodal end points) although it's *very* unlikely this will
	// happen since the number of points doesn't change (ie, should not fail due to having less
	// than three points for a polygon).
	// If it fails because of great circle arc antipodal end points then log a console warning message.
	if (!geometry_on_sphere)
	{
		if (geometry_validity == GPlatesUtils::GeometryConstruction::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS)
		{
			qWarning() << "GeometryDeformation: Deformed polyline/polygon has antipodal end points "
				"on one or more of its edges (arcs).";
		}
	}

	// FIXME: Find a way to recover from this.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			geometry_on_sphere,
			GPLATES_ASSERTION_SOURCE);

	return geometry_on_sphere.get();
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample::calc_instantaneous_deformation_information() const
{
	d_have_initialised_instantaneous_deformation_information = true;

	// Return early if none of the geometry points are inside deforming regions.
	if (!d_delaunay_faces)
	{
		return;
	}

	// Iterate over the delaunay faces and calculate instantaneous deformation information.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_deformation_infos.size() == d_delaunay_faces->size(),
			GPLATES_ASSERTION_SOURCE);
	const unsigned int num_points = d_deformation_infos.size();
	const delaunay_face_opt_seq_type &delaunay_faces = d_delaunay_faces.get();
	for (unsigned int point_index = 0; point_index < num_points; ++point_index)
	{
		DeformationInfo &point_deformation_info = d_deformation_infos[point_index];

		// If the current geometry point is inside a deforming region then copy the deformation info
		// from the delaunay face it lies within.
		if (delaunay_faces[point_index])
		{
			const ResolvedTriangulation::Delaunay_2::Face::DeformationInfo &face_deformation_info =
					delaunay_faces[point_index].get()->get_deformation_info();
					
			point_deformation_info.SR22 = face_deformation_info.SR22;
			point_deformation_info.SR33 = face_deformation_info.SR33;
			point_deformation_info.SR23 = face_deformation_info.SR23;
		}
	}
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample &
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::TimeWindow::add_geometry_sample(
		const std::vector<GPlatesMaths::PointOnSphere> &geometry_points)
{
	// Add a new geometry sample to the front of the window.
	d_geometry_sample_time_span.push_front(GeometrySample());
	GeometrySample &geometry_sample = d_geometry_sample_time_span.front();

	// Add the geometry points to the geometry sample.
	geometry_sample.points.reserve(geometry_points.size());
	std::copy(
			geometry_points.begin(),
			geometry_points.end(),
			std::back_inserter(geometry_sample.points));

	// Allocate deformation information - it'll get initialised later.
	geometry_sample.d_deformation_infos.resize(geometry_points.size());

	return geometry_sample;
}


double
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::TimeWindow::get_begin_time() const
{
	// Should have at least one geometry sample.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_geometry_sample_time_span.empty(),
			GPLATES_ASSERTION_SOURCE);

	// For example, three geometry samples (fence posts) are separated by two time intervals.
	const unsigned int num_geometry_sample_intervals = d_geometry_sample_time_span.size() - 1;

	return d_end_time + d_time_increment * num_geometry_sample_intervals;
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample &
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::TimeWindow::get_geometry_sample(
		unsigned int geometry_sample_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			geometry_sample_index < d_geometry_sample_time_span.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_geometry_sample_time_span[geometry_sample_index];
}


const GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample &
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::TimeWindow::get_geometry_sample_at_begin_time() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_geometry_sample_time_span.empty(),
			GPLATES_ASSERTION_SOURCE);

	return d_geometry_sample_time_span.front();
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample &
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::TimeWindow::get_geometry_sample_at_begin_time()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_geometry_sample_time_span.empty(),
			GPLATES_ASSERTION_SOURCE);

	return d_geometry_sample_time_span.front();
}


unsigned int
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::TimeWindow::get_geometry_sample_index(
		const double &reconstruction_time) const
{
	// The 0.5 is to round to the nearest integer.
	// Casting to 'int' is optimised on Visual Studio compared to 'unsigned int'.
	const int geometry_sample_index = static_cast<int>(
			0.5 + (get_begin_time() - reconstruction_time) / d_time_increment);

	// This should succeed if 'reconstruction_time' was inside the geometry sample time span range.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			geometry_sample_index >= 0 &&
				static_cast<unsigned int>(geometry_sample_index) < d_geometry_sample_time_span.size(),
			GPLATES_ASSERTION_SOURCE);

	return geometry_sample_index;
}
