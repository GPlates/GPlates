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
#include <cfloat>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <utility>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include <iostream>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>

#include "TopologyReconstruct.h"

#include "GeometryUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "ResolvedTriangulationNetwork.h"

#include "global/AbortException.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/AngularExtent.h"
#include "maths/CalculateVelocity.h"
#include "maths/GeometryDistance.h"
#include "maths/MathsUtils.h"
#include "maths/Rotation.h"
#include "maths/SmallCircleBounds.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/Profile.h"


#ifdef _MSC_VER
#ifndef copysign
#define copysign _copysign
#endif
#endif


namespace GPlatesAppLogic
{
	namespace
	{
		// Inverse of Earth radius (Kms).
		const double INVERSE_EARTH_EQUATORIAL_RADIUS_KMS = 1.0 / GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS;

		/**
		 * Predicate to test if the geometry *points* bounding small circle intersects the
		 * resolved boundary bounding small circle.
		 */
		class IntersectGeometryPointsAndResolvedBoundarySmallCircleBounds :
				public std::unary_function<ResolvedTopologicalBoundary::non_null_ptr_type, bool>
		{
		public:

			explicit
			IntersectGeometryPointsAndResolvedBoundarySmallCircleBounds(
					const GPlatesMaths::BoundingSmallCircle *geometry_points_bounding_small_circle) :
				d_geometry_points_bounding_small_circle(geometry_points_bounding_small_circle)
			{  }

			bool
			operator()(
					const ResolvedTopologicalBoundary::non_null_ptr_type &rtb) const
			{
				const GPlatesMaths::BoundingSmallCircle &rtb_bounding_small_circle =
						rtb->resolved_topology_boundary()->get_bounding_small_circle();

				return intersect(rtb_bounding_small_circle, *d_geometry_points_bounding_small_circle);
			}

		private:

			const GPlatesMaths::BoundingSmallCircle *d_geometry_points_bounding_small_circle;
		};

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
				const GPlatesMaths::BoundingSmallCircle &rtn_bounding_small_circle =
						rtn->get_triangulation_network().get_boundary_polygon()
								->get_bounding_small_circle();

				return intersect(rtn_bounding_small_circle, *d_geometry_points_bounding_small_circle);
			}

		private:

			const GPlatesMaths::BoundingSmallCircle *d_geometry_points_bounding_small_circle;
		};


		/**
		 * Get the rigid rotation from @a initial_time to @a final_time.
		 */
		GPlatesMaths::FiniteRotation
		get_stage_rotation(
				GPlatesModel::integer_plate_id_type reconstruction_plate_id,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &initial_time,
				const double &final_time)
		{
			//
			// Delegate to 'PlateVelocityUtils::calculate_stage_rotation()' since it adjusts the
			// stage rotation time interval if one of the times goes negative or if the rotation
			// file only has rotations up to time 't', but not time 't+dt'.
			//

			if (initial_time > final_time) // forward in time ...
			{
				// Forward stage rotation from 'initial_time' to 'final_time'.
				return PlateVelocityUtils::calculate_stage_rotation(
						reconstruction_plate_id,
						reconstruction_tree_creator,
						initial_time/*reconstruction_time*/,
						// Must be positive...
						initial_time - final_time/*velocity_delta_time*/,
						VelocityDeltaTime::T_TO_T_MINUS_DELTA_T/*velocity_delta_time_type*/);
			}
			else // backward in time ...
			{
				// Backward stage rotation from 'initial_time' to 'final_time'.
				// Note: Need to reverse rotation from forward-in-time to backward-in-time.
				return GPlatesMaths::get_reverse(
						PlateVelocityUtils::calculate_stage_rotation(
								reconstruction_plate_id,
								reconstruction_tree_creator,
								initial_time/*reconstruction_time*/,
								// Must be positive...
								final_time - initial_time/*velocity_delta_time*/,
								VelocityDeltaTime::T_PLUS_DELTA_T_TO_T/*velocity_delta_time_type*/));
			}
		}
	}
}


const GPlatesAppLogic::TopologyReconstruct::ActivePointParameters
GPlatesAppLogic::TopologyReconstruct::DEFAULT_ACTIVE_POINT_PARAMETERS(
		0.7, // cms/yr
		10.0); // kms/my


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::create_geometry_time_span(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id,
		const double &geometry_import_time,
		boost::optional<double> max_poly_segment_angular_extent_radians,
		boost::optional<ActivePointParameters> active_point_parameters,
		bool deformation_uses_natural_neighbour_interpolation) const
{
	PROFILE_FUNC();

	return GeometryTimeSpan::non_null_ptr_type(
			new GeometryTimeSpan(
					this,
					geometry,
					feature_reconstruction_plate_id,
					geometry_import_time,
					max_poly_segment_angular_extent_radians,
					active_point_parameters,
					deformation_uses_natural_neighbour_interpolation));
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometryTimeSpan(
		TopologyReconstruct::non_null_ptr_to_const_type topology_reconstruct,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id,
		const double &geometry_import_time,
		boost::optional<double> max_poly_segment_angular_extent_radians,
		boost::optional<ActivePointParameters> active_point_parameters,
		bool deformation_uses_natural_neighbour_interpolation) :
	d_topology_reconstruct(topology_reconstruct),
	d_time_range(topology_reconstruct->get_time_range()),
	d_pool_allocator(PoolAllocator::create()),
	d_reconstruction_plate_id(feature_reconstruction_plate_id),
	d_geometry_import_time(geometry_import_time),
	d_deformation_uses_natural_neighbour_interpolation(deformation_uses_natural_neighbour_interpolation),
	d_time_window_span(
			time_window_span_type::create(
					topology_reconstruct->get_time_range(),
					// The function to create geometry samples in rigid regions...
					boost::bind(
							&GeometryTimeSpan::create_rigid_geometry_sample,
							this,
							_1,
							_2,
							_3),
					// The function to interpolate geometry samples...
					boost::bind(
							&GeometryTimeSpan::interpolate_geometry_sample,
							this,
							_1,
							_2,
							_3,
							_4,
							_5),
					// The present day geometry points.
					// Note that we'll need to modify this if 'geometry_import_time' is earlier
					// than the end of the time range since might be affected by time range...
					create_import_sample(
							d_interpolate_original_points,
							*geometry,
							d_pool_allocator,
							max_poly_segment_angular_extent_radians))),
	d_active_point_parameters(active_point_parameters),
	d_accessing_strain_rates(0),
	d_accessing_strains(0),
	d_have_initialised_strains(false)
{
	initialise_time_windows();
}


void
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::initialise_time_windows()
{
	// The time range of both the resolved boundary/network topologies and the topology-reconstructed geometry samples.
	const unsigned int num_time_slots = d_time_range.get_num_time_slots();

	// Find the nearest time slot to the geometry import time (if it's inside the time range).
	boost::optional<unsigned int> geometry_import_time_slot = d_time_range.get_nearest_time_slot(d_geometry_import_time);
	if (geometry_import_time_slot)
	{
		//
		// The geometry import time is within the time range.
		//

		// First adjust the geometry import time to match the nearest time slot.
		//
		// Ideally we should probably get resolved boundaries/networks at the actual geometry import time
		// and reconstruct the imported geometry to the nearest time slot, but if the user has chosen a
		// large time increment in their time range then the time slots will be spaced far apart and
		// the resulting accuracy will suffer (and this is a part of that).
		d_geometry_import_time = d_time_range.get_time(geometry_import_time_slot.get());

		// The geometry at the import time is just the rigid rotation of present day geometry to the import time.
		GeometrySample::non_null_ptr_type import_geometry_sample =
				rigid_reconstruct(
						d_time_window_span->get_present_day_sample(),
						d_geometry_import_time,
						false/*reverse_reconstruct*/,
						// We're storing the imported geometry sample in our time span so use our allocator...
						d_pool_allocator);

		// Store the imported geometry in the geometry import time slot.
		d_time_window_span->set_sample_in_time_slot(
				import_geometry_sample,
				geometry_import_time_slot.get()/*time_slot*/);

		// Iterate over the time range going *backwards* in time from the geometry import time (most recent)
		// to the beginning of the time range (least recent).
		reconstruct_time_steps(
				import_geometry_sample,
				geometry_import_time_slot.get()/*start_time_slot*/,
				0/*end_time_slot*/);

		// Iterate over the time range going *forward* in time from the geometry import time (least recent)
		// to the end of the time range (most recent).
		boost::optional<GeometrySample::non_null_ptr_type> end_geometry_sample =
				reconstruct_time_steps(
						import_geometry_sample,
						geometry_import_time_slot.get()/*start_time_slot*/,
						num_time_slots - 1/*end_time_slot*/);

		// If the end geometry sample is active then use it to set the present day geometry sample.
		if (end_geometry_sample)
		{
			GeometrySample::non_null_ptr_type present_day_geometry_sample =
					// Rigidly reconstruct from the end of the time range to present day if necessary...
					(d_time_range.get_end_time() > GPlatesMaths::Real(0.0))
					? rigid_reconstruct(
							end_geometry_sample.get(),
							d_time_range.get_end_time(),
							true/*reverse_reconstruct*/,
							// We're storing the imported geometry sample in our time span so use our allocator...
							d_pool_allocator)
					: end_geometry_sample.get();

			// Reset the present day geometry points.
			// They will have been affected by the topologies within the time range.
			d_time_window_span->get_present_day_sample() = present_day_geometry_sample;
		}
	}
	else if (d_geometry_import_time > d_time_range.get_begin_time())
	{
		// The geometry import time is older than the beginning of the time range.
		// The geometry at the import time is just the rigid rotation of present day geometry
		// to the import time. And since there's rigid rotation from geometry import time to the
		// beginning of the time range, the geometry at the beginning of the time range is
		// just a rigid reconstruction from present day to the beginning of the time range.
		GeometrySample::non_null_ptr_type begin_geometry_sample =
				rigid_reconstruct(
						d_time_window_span->get_present_day_sample(),
						d_time_range.get_begin_time());

		// Store in the beginning time slot.
		d_time_window_span->set_sample_in_time_slot(
				begin_geometry_sample,
				0/*time_slot*/);

		// Iterate over the time range going *forward* in time from the beginning of the
		// time range (least recent) to the end (most recent).
		boost::optional<GeometrySample::non_null_ptr_type> end_geometry_sample =
				reconstruct_time_steps(
						begin_geometry_sample,
						0/*start_time_slot*/,
						num_time_slots - 1/*end_time_slot*/);

		// If the end geometry sample is active then use it to set the present day geometry sample.
		if (end_geometry_sample)
		{
			GeometrySample::non_null_ptr_type present_day_geometry_sample =
					// Rigidly reconstruct from the end of the time range to present day if necessary...
					(d_time_range.get_end_time() > GPlatesMaths::Real(0.0))
					? rigid_reconstruct(
							end_geometry_sample.get(),
							d_time_range.get_end_time(),
							true/*reverse_reconstruct*/,
							// We're storing the imported geometry sample in our time span so use our allocator...
							d_pool_allocator)
					: end_geometry_sample.get();

			// Reset the present day geometry points.
			// They will have been affected by the topologies within the time range.
			d_time_window_span->get_present_day_sample() = present_day_geometry_sample;
		}
	}
	else // d_geometry_import_time < d_time_range.get_end_time() ...
	{
		// The geometry import time is younger than the end of the time range.
		// The geometry at the import time is just the rigid rotation of present day geometry
		// to the import time. And since there's rigid rotation from geometry import time to the
		// end of the time range, the geometry at the end of the time range is just a
		// rigid reconstruction from present day to the end of the time range.
		GeometrySample::non_null_ptr_type end_geometry_sample =
				rigid_reconstruct(
						d_time_window_span->get_present_day_sample(),
						d_time_range.get_end_time());

		// Store in the end time slot.
		d_time_window_span->set_sample_in_time_slot(
				end_geometry_sample,
				num_time_slots - 1);

		// Iterate over the time range going *backwards* in time from the end of the
		// time range (most recent) to the beginning (least recent).
		reconstruct_time_steps(
				end_geometry_sample,
				num_time_slots - 1/*start_time_slot*/,
				0/*end_time_slot*/);

		// Note that we don't need to reset the present day geometry points since the geometry
		// import time is after (younger than) the end of the time range and hence the
		// present day geometry is not affected by the topologies in the time range.
	}
}


boost::optional<GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type>
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_time_steps(
		const GeometrySample::non_null_ptr_type &start_geometry_sample,
		unsigned int start_time_slot,
		unsigned int end_time_slot)
{
	if (start_time_slot == end_time_slot)
	{
		return start_geometry_sample;
	}

	const bool reverse_reconstruct = end_time_slot > start_time_slot;
	const int time_slot_direction = reverse_reconstruct ? 1 : -1;

	// The geometry sample from the previous time step.
	// For the first time step (start_time_slot -> start_time_slot +/- 1) this is the start geometry sample.
	GeometrySample::non_null_ptr_type prev_geometry_sample = start_geometry_sample;

	// Reconstruct the start time slot to the next time slot.
	// The start sample is always active (because it would need a previous sample before it can be
	// deactivated and start sample does not have a previous sample).
	GeometrySample::non_null_ptr_type current_geometry_sample =
			reconstruct_first_time_step(
					start_geometry_sample,
					start_time_slot/*current_time_slot*/,
					start_time_slot + time_slot_direction/*next_time_slot*/);

	// Iterate over the remaining time slots either backward or forward in time (depending on 'time_slot_direction').
	for (unsigned int time_slot = start_time_slot + time_slot_direction;
		time_slot != end_time_slot;
		time_slot += time_slot_direction)
	{
		const unsigned int current_time_slot = time_slot;
		const unsigned int next_time_slot = current_time_slot + time_slot_direction;

		// Reconstruct from the current time slot to the next time slot.
		// This also determines whether the *current* time slot is active
		// (it signals this by returning none for the *next* time slot).
		boost::optional<GeometrySample::non_null_ptr_type> next_geometry_sample =
				reconstruct_intermediate_time_step(
						prev_geometry_sample,
						current_geometry_sample,
						current_time_slot,
						next_time_slot);
		if (!next_geometry_sample)
		{
			// Current time slot is not active - so the last active time slot is the previous time slot.
			if (reverse_reconstruct) // forward in time ...
			{
				d_time_slot_of_disappearance = current_time_slot - time_slot_direction;
			}
			else // backward in time ...
			{
				d_time_slot_of_appearance = current_time_slot - time_slot_direction;
			}

			return boost::none;
		}

		// The current time slot is active, so set the geometry sample for it.
		d_time_window_span->set_sample_in_time_slot(current_geometry_sample, current_time_slot);

		// Set the previous geometry sample for the next time step.
		prev_geometry_sample = current_geometry_sample;

		// Set the current geometry sample for the next time step.
		current_geometry_sample = next_geometry_sample.get();
	}

	//
	// In order to be able to calculate velocities at either end of the time range
	// we need the topology point locations for those time slots.
	// We also need to deactivate any points that are subducted/consumed.
	// So we do one final pass.
	//
	// Note that we don't actually advance the current point locations
	// (they're already at one end of the time range).
	//

	if (!reconstruct_last_time_step(
			prev_geometry_sample,    // prior-to-end geometry sample
			current_geometry_sample, // end geometry sample
			end_time_slot,
			d_time_range.get_time_increment()/*time_increment*/,
			reverse_reconstruct))
	{
		// End time slot is not active - so the last active time slot is the time slot prior to it.
		if (reverse_reconstruct) // forward in time ...
		{
			d_time_slot_of_disappearance = end_time_slot - time_slot_direction;
		}
		else // backward in time ...
		{
			d_time_slot_of_appearance = end_time_slot - time_slot_direction;
		}

		return boost::none;
	}

	// The end time slot is active, so set the geometry sample for it.
	d_time_window_span->set_sample_in_time_slot(current_geometry_sample, end_time_slot);

	return current_geometry_sample; // end geometry sample
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_first_time_step(
		const GeometrySample::non_null_ptr_type &current_geometry_sample,
		unsigned int current_time_slot,
		unsigned int next_time_slot)
{
	// Get the resolved boundaries/networks for the current time slot.
	//
	// As an optimisation, remove those boundaries/networks that the current geometry points do not intersect.
	rtb_seq_type resolved_boundaries;
	rtn_seq_type resolved_networks;
	if (!get_resolved_topologies(resolved_boundaries, resolved_networks, current_time_slot, current_geometry_sample))
	{
		return rigid_stage_reconstruct(
				current_geometry_sample,
				d_time_range.get_time(current_time_slot)/*initial_time*/,
				d_time_range.get_time(next_time_slot)/*final_time*/);
	}
	// We've excluded those resolved boundaries/networks that can't possibly intersect the current geometry points.
	// This doesn't mean the remaining boundaries/networks will definitely intersect though - they might not.

	//
	// Attempt to reconstruct using the topologies.
	//

	const double current_time = d_time_range.get_time(current_time_slot);
	const double next_time = d_time_range.get_time(next_time_slot);

	// Reverse reconstruction means forward in time (time slots increase going forward in time).
	const bool reverse_reconstruct = next_time_slot > current_time_slot;
	// The time increment should always be positive.
	const double time_increment = reverse_reconstruct
			? (current_time - next_time)
			: (next_time - current_time);

	std::vector<GeometryPoint *> &current_geometry_points =
			current_geometry_sample->get_geometry_points(d_accessing_strain_rates);
	const unsigned int num_geometry_points = current_geometry_points.size();

	// The geometry points for the next geometry sample.
	std::vector<GeometryPoint *> next_geometry_points;
	next_geometry_points.resize(num_geometry_points, NULL);

	// Keep track of the stage rotations of resolved boundaries as we encounter them.
	// This is an optimisation that saves a few seconds (for a large number of points in geometry)
	// since many points will be inside the same resolved boundary.
	plate_id_to_stage_rotation_map_type resolved_boundary_reconstruct_stage_rotation_map;

	// Keep track of number of topology reconstructed geometry points for the current time.
	unsigned int num_topology_reconstructed_geometry_points = 0;

	// Iterate over the current geometry points and attempt to reconstruct them using resolved boundaries/networks.
	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *current_geometry_point = current_geometry_points[geometry_point_index];

		// Ignore current point if it's not active.
		// Actually all points should be active initially - but we'll check just in case.
		if (current_geometry_point == NULL)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere current_point(current_geometry_point->position);

		//
		// Iterate over the resolved networks for the current time.
		//
		// NOTE: We search resolved networks before resolved boundaries in case some networks
		// overlap (on top of) resolved boundaries - we want networks to have a higher priority.
		//

		boost::optional<GPlatesMaths::PointOnSphere> topology_reconstructed_point;

		// First attempt uses resolved networks.
		topology_reconstructed_point = reconstruct_point_using_resolved_networks(
				current_point,
				current_geometry_point->location,
				resolved_networks,
				time_increment,
				reverse_reconstruct);
		if (!topology_reconstructed_point)
		{
			// Second attempt uses resolved boundaries.
			topology_reconstructed_point = reconstruct_point_using_resolved_boundaries(
					current_point,
					current_geometry_point->location,
					resolved_boundaries,
					resolved_boundary_reconstruct_stage_rotation_map,
					current_time,
					next_time);
		}

		if (topology_reconstructed_point)
		{
			// Record the next point.
			next_geometry_points[geometry_point_index] =
					d_pool_allocator->geometry_point_pool.construct(topology_reconstructed_point.get());

			++num_topology_reconstructed_geometry_points;
		}
	}

	// If none of the current geometry points intersect any topology surfaces then continue to the next time slot.
	if (num_topology_reconstructed_geometry_points == 0)
	{
		return rigid_stage_reconstruct(
				current_geometry_sample,
				d_time_range.get_time(current_time_slot)/*initial_time*/,
				d_time_range.get_time(next_time_slot)/*final_time*/);
	}

	// If we get here then at least one geometry point was reconstructed using resolved boundaries/networks.

	// If not all geometry points were reconstructed using resolved boundaries/networks then
	// rigidly rotate those that were not.
	if (num_topology_reconstructed_geometry_points < num_geometry_points)
	{
		// Get the rigid finite rotation used for those geometry points that did not
		// intersect any resolved boundaries/networks and hence must be rigidly rotated.
		const GPlatesMaths::FiniteRotation rigid_stage_rotation = get_stage_rotation(
				d_reconstruction_plate_id,
				d_topology_reconstruct->get_reconstruction_tree_creator(),
				current_time/*initial_time*/,
				next_time/*final_time*/);

		// Find and rotate those points that were not topology reconstructed.
		for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
		{
			GeometryPoint *&next_geometry_point = next_geometry_points[geometry_point_index];

			// If next point was not topology reconstructed...
			if (next_geometry_point == NULL)
			{
				GeometryPoint *current_geometry_point = current_geometry_points[geometry_point_index];

				// If current point is active...
				if (current_geometry_point != NULL)
				{
					// Add rigidly rotated geometry point.
					next_geometry_point = d_pool_allocator->geometry_point_pool.construct(
							rigid_stage_rotation * current_geometry_point->position);
				}
			}
		}
	}

	// Return the next geometry sample.
	return GeometrySample::create_swap(next_geometry_points, d_pool_allocator);
}


boost::optional<GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type>
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_intermediate_time_step(
		const GeometrySample::non_null_ptr_type &prev_geometry_sample,
		const GeometrySample::non_null_ptr_type &current_geometry_sample,
		unsigned int current_time_slot,
		unsigned int next_time_slot)
{
	// Get the resolved boundaries/networks for the current time slot.
	//
	// As an optimisation, remove those boundaries/networks that the current geometry points do not intersect.
	rtb_seq_type resolved_boundaries;
	rtn_seq_type resolved_networks;
	if (!get_resolved_topologies(resolved_boundaries, resolved_networks, current_time_slot, current_geometry_sample))
	{
		return rigid_stage_reconstruct(
				current_geometry_sample,
				d_time_range.get_time(current_time_slot)/*initial_time*/,
				d_time_range.get_time(next_time_slot)/*final_time*/);
	}
	// We've excluded those resolved boundaries/networks that can't possibly intersect the current geometry points.
	// This doesn't mean the remaining boundaries/networks will definitely intersect though - they might not.

	//
	// Attempt to reconstruct using the topologies.
	//

	const double current_time = d_time_range.get_time(current_time_slot);
	const double next_time = d_time_range.get_time(next_time_slot);

	// Reverse reconstruction means forward in time (time slots increase going forward in time).
	const bool reverse_reconstruct = next_time_slot > current_time_slot;
	// The time increment should always be positive.
	const double time_increment = reverse_reconstruct
			? (current_time - next_time)
			: (next_time - current_time);

	std::vector<GeometryPoint *> &current_geometry_points =
			current_geometry_sample->get_geometry_points(d_accessing_strain_rates);
	const unsigned int num_geometry_points = current_geometry_points.size();

	// Previous geometry points.
	const std::vector<GeometryPoint *> &prev_geometry_points =
			prev_geometry_sample->get_geometry_points(d_accessing_strain_rates);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			prev_geometry_points.size() == num_geometry_points,
			GPLATES_ASSERTION_SOURCE);

	// The geometry points for the next geometry sample.
	std::vector<GeometryPoint *> next_geometry_points;
	next_geometry_points.resize(num_geometry_points, NULL);

	// Keep track of the stage rotations of resolved boundaries as we encounter them.
	// This is an optimisation that saves a few seconds (for a large number of points in geometry)
	// since many points will be inside the same resolved boundary.
	plate_id_to_stage_rotation_map_type resolved_boundary_reconstruct_stage_rotation_map;
	plate_id_to_stage_rotation_map_type resolved_boundary_velocity_stage_rotation_map;

	// The minimum distance threshold used to determine when to deactivate geometry points.
	GPlatesMaths::AngularExtent min_distance_threshold_radians = GPlatesMaths::AngularExtent::ZERO;
	if (d_active_point_parameters)
	{
		min_distance_threshold_radians = GPlatesMaths::AngularExtent::create_from_angle(
					d_active_point_parameters->threshold_distance_to_boundary_in_kms_per_my *
							// Need to convert kms/my to kms using time increment...
							time_increment * INVERSE_EARTH_EQUATORIAL_RADIUS_KMS);
	}

	// Keep track of number of topology reconstructed geometry points for the current time.
	unsigned int num_topology_reconstructed_geometry_points = 0;
	// Keep track of number of active geometry points for the current time.
	unsigned int num_active_geometry_points = 0;

	// Iterate over the current geometry points and attempt to reconstruct them using resolved boundaries/networks.
	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *current_geometry_point = current_geometry_points[geometry_point_index];

		// Ignore current point if it's not active.
		if (current_geometry_point == NULL)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere current_point(current_geometry_point->position);

		//
		// Iterate over the resolved networks for the current time.
		//
		// NOTE: We search resolved networks before resolved boundaries in case some networks
		// overlap (on top of) resolved boundaries - we want networks to have a higher priority.
		//

		boost::optional<GPlatesMaths::PointOnSphere> topology_reconstructed_point;

		// First attempt uses resolved networks.
		topology_reconstructed_point = reconstruct_point_using_resolved_networks(
				current_point,
				current_geometry_point->location,
				resolved_networks,
				time_increment,
				reverse_reconstruct);
		if (!topology_reconstructed_point)
		{
			// Second attempt uses resolved boundaries.
			topology_reconstructed_point = reconstruct_point_using_resolved_boundaries(
					current_point,
					current_geometry_point->location,
					resolved_boundaries,
					resolved_boundary_reconstruct_stage_rotation_map,
					current_time,
					next_time);
		}

		// If can deactivate points...
		if (d_active_point_parameters)
		{
			// Now that we have the current topology point location (was set above) we can determine
			// if the current point should be de-activated (eg, subducted forward in time or consumed
			// by mid-ocean ridge backward in time).
			//
			// But we can only do this if we have a previous active geometry point.
			GeometryPoint *prev_geometry_point = prev_geometry_points[geometry_point_index];
			if (prev_geometry_point &&
				!is_point_active(
						GPlatesMaths::PointOnSphere(prev_geometry_point->position)/*prev_point*/,
						prev_geometry_point->location/*prev_location*/,
						current_point,
						current_geometry_point->location/*current_location*/,
						current_time,
						time_increment,
						reverse_reconstruct,
						min_distance_threshold_radians,
						resolved_boundary_velocity_stage_rotation_map))
			{
				// De-activate the current point.
				current_geometry_points[geometry_point_index] = NULL;

				// Continue without setting the next point.
				// The current point is inactive and so the next point is too.
				continue;
			}
		}

		if (topology_reconstructed_point)
		{
			// Record the next point.
			next_geometry_points[geometry_point_index] =
					d_pool_allocator->geometry_point_pool.construct(topology_reconstructed_point.get());

			++num_topology_reconstructed_geometry_points;
		}

		// Active points include both topology reconstructed points and rigidly rotated points.
		++num_active_geometry_points;
	}

	// If there are no active points then signal this.
	if (num_active_geometry_points == 0)
	{
		return boost::none;
	}

	// If none of the current geometry points intersect any topology surfaces then continue to the next time slot.
	if (num_topology_reconstructed_geometry_points == 0)
	{
		return rigid_stage_reconstruct(
				current_geometry_sample,
				d_time_range.get_time(current_time_slot)/*initial_time*/,
				d_time_range.get_time(next_time_slot)/*final_time*/);
	}

	// If we get here then at least one geometry point was reconstructed using resolved boundaries/networks.

	// If not all geometry points were reconstructed using resolved boundaries/networks then
	// rigidly rotate those that were not.
	if (num_topology_reconstructed_geometry_points < num_geometry_points)
	{
		// Get the rigid finite rotation used for those geometry points that did not
		// intersect any resolved boundaries/networks and hence must be rigidly rotated.
		const GPlatesMaths::FiniteRotation rigid_stage_rotation = get_stage_rotation(
				d_reconstruction_plate_id,
				d_topology_reconstruct->get_reconstruction_tree_creator(),
				current_time/*initial_time*/,
				next_time/*final_time*/);

		// Find and rotate those points that were not topology reconstructed.
		for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
		{
			GeometryPoint *&next_geometry_point = next_geometry_points[geometry_point_index];

			// If next point was not topology reconstructed...
			if (next_geometry_point == NULL)
			{
				GeometryPoint *current_geometry_point = current_geometry_points[geometry_point_index];

				// If current point is active...
				if (current_geometry_point != NULL)
				{
					// Add rigidly rotated geometry point.
					next_geometry_point = d_pool_allocator->geometry_point_pool.construct(
							rigid_stage_rotation * current_geometry_point->position);
				}
			}
		}
	}

	// Return the next geometry sample.
	return GeometrySample::create_swap(next_geometry_points, d_pool_allocator);
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_last_time_step(
		boost::optional<GeometrySample::non_null_ptr_type> prev_geometry_sample,
		const GeometrySample::non_null_ptr_type &current_geometry_sample,
		unsigned int current_time_slot,
		const double &time_increment,
		bool reverse_reconstruct)
{
	// Get the resolved boundaries/networks for the current time slot.
	//
	// As an optimisation, remove those boundaries/networks that the current geometry points do not intersect.
	rtb_seq_type resolved_boundaries;
	rtn_seq_type resolved_networks;
	if (!get_resolved_topologies(resolved_boundaries, resolved_networks, current_time_slot, current_geometry_sample))
	{
		// There are still active geometry points - it's just that none of them intersected resolved topologies.
		return true;
	}
	// We've excluded those resolved boundaries/networks that can't possibly intersect the current geometry points.
	// This doesn't mean the remaining boundaries/networks will definitely intersect though - they might not.

	const double current_time = d_time_range.get_time(current_time_slot);

	std::vector<GeometryPoint *> &current_geometry_points =
			current_geometry_sample->get_geometry_points(d_accessing_strain_rates);
	const unsigned int num_geometry_points = current_geometry_points.size();

	// Previous geometry points (if the current geometry points are not the first time slot).
	boost::optional<const std::vector<GeometryPoint *> &> prev_geometry_points;
	if (prev_geometry_sample)
	{
		prev_geometry_points = prev_geometry_sample.get()->get_geometry_points(d_accessing_strain_rates);

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				prev_geometry_points->size() == num_geometry_points,
				GPLATES_ASSERTION_SOURCE);
	}

	// Keep track of the stage rotations of resolved boundaries as we encounter them.
	// This is an optimisation that saves a few seconds (for a large number of points in geometry)
	// since many points will be inside the same resolved boundary.
	plate_id_to_stage_rotation_map_type resolved_boundary_velocity_stage_rotation_map;

	// The minimum distance threshold used to determine when to deactivate geometry points.
	GPlatesMaths::AngularExtent min_distance_threshold_radians = GPlatesMaths::AngularExtent::ZERO;
	if (d_active_point_parameters)
	{
		min_distance_threshold_radians = GPlatesMaths::AngularExtent::create_from_angle(
					d_active_point_parameters->threshold_distance_to_boundary_in_kms_per_my *
							// Need to convert kms/my to kms using time increment...
							time_increment * INVERSE_EARTH_EQUATORIAL_RADIUS_KMS);
	}

	// Keep track of number of active geometry points for the current time.
	unsigned int num_active_geometry_points = 0;

	// Iterate over the current geometry points and attempt to reconstruct them using resolved boundaries/networks.
	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *current_geometry_point = current_geometry_points[geometry_point_index];

		// Ignore current point if it's not active.
		if (current_geometry_point == NULL)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere current_point(current_geometry_point->position);

		//
		// Iterate over the resolved networks for the current time.
		//
		// NOTE: We search resolved networks before resolved boundaries in case some networks
		// overlap (on top of) resolved boundaries - we want networks to have a higher priority.
		//

		// First search the resolved networks.
		if (!reconstruct_last_point_using_resolved_networks(
				current_point,
				current_geometry_point->location,
				resolved_networks))
		{
			// Second search the resolved boundaries.
			reconstruct_last_point_using_resolved_boundaries(
					current_point,
					current_geometry_point->location,
					resolved_boundaries);
		}

		// If can deactivate points...
		if (d_active_point_parameters)
		{
			// Now that we have the current topology point location (was set above) we can determine
			// if the current point should be de-activated (eg, subducted forward in time or consumed
			// by mid-ocean ridge backward in time).
			//
			// But we can only do this if we have a previous active geometry point.
			GeometryPoint *prev_geometry_point = NULL;
			if (prev_geometry_points)
			{
				prev_geometry_point = prev_geometry_points.get()[geometry_point_index];

				if (prev_geometry_point &&
					!is_point_active(
							GPlatesMaths::PointOnSphere(prev_geometry_point->position)/*prev_point*/,
							prev_geometry_point->location/*prev_location*/,
							current_point,
							current_geometry_point->location/*current_location*/,
							current_time,
							time_increment,
							reverse_reconstruct,
							min_distance_threshold_radians,
							resolved_boundary_velocity_stage_rotation_map))
				{
					// De-activate the current point.
					current_geometry_points[geometry_point_index] = NULL;

					continue;
				}
			}
		}

		++num_active_geometry_points;
	}

	// If there are no active points then signal this.
	if (num_active_geometry_points == 0)
	{
		return false;
	}

	return true;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_point_using_resolved_networks(
		const GPlatesMaths::PointOnSphere &point,
		TopologyPointLocation &location,
		rtn_seq_type &resolved_networks,
		const double &time_increment,
		bool reverse_reconstruct)
{
	// Iterate over the resolved networks.
	rtn_seq_type::iterator resolved_networks_iter = resolved_networks.begin();
	rtn_seq_type::iterator resolved_networks_end = resolved_networks.end();
	for ( ; resolved_networks_iter != resolved_networks_end; ++resolved_networks_iter)
	{
		const ResolvedTopologicalNetwork::non_null_ptr_type resolved_network = *resolved_networks_iter;

		boost::optional<
				std::pair<
						GPlatesMaths::PointOnSphere,
						ResolvedTriangulation::Network::point_location_type> > deformed_point_result =
				resolved_network->get_triangulation_network().calculate_deformed_point(
						point,
						time_increment,
						reverse_reconstruct,
						d_deformation_uses_natural_neighbour_interpolation);
		if (!deformed_point_result)
		{
			// The point is outside the network so continue searching the resolved networks.
			continue;
		}

		// Store the network location of the point.
		location = TopologyPointLocation(resolved_network, deformed_point_result->second);

		// The deformed point.
		const GPlatesMaths::PointOnSphere &deformed_geometry_point = deformed_point_result->first;

		// The next point is probably in the same resolved network so make it the first one to be tested next time.
		if (resolved_networks_iter != resolved_networks.begin())
		{
			std::swap(resolved_networks.front(), *resolved_networks_iter);
		}

		return deformed_geometry_point;
	}

	return boost::none;
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_last_point_using_resolved_networks(
		const GPlatesMaths::PointOnSphere &point,
		TopologyPointLocation &location,
		rtn_seq_type &resolved_networks)
{
	// Iterate over the resolved networks.
	rtn_seq_type::iterator resolved_networks_iter = resolved_networks.begin();
	rtn_seq_type::iterator resolved_networks_end = resolved_networks.end();
	for ( ; resolved_networks_iter != resolved_networks_end; ++resolved_networks_iter)
	{
		const ResolvedTopologicalNetwork::non_null_ptr_type resolved_network = *resolved_networks_iter;

		boost::optional<ResolvedTriangulation::Network::point_location_type> point_location_result =
				resolved_network->get_triangulation_network().get_point_location(point);
		if (!point_location_result)
		{
			// The point is outside the network so continue searching the resolved networks.
			continue;
		}

		// Store the network location of the point.
		location = TopologyPointLocation(resolved_network, point_location_result.get());

		// The next point is probably in the same resolved network so make it the first one to be tested next time.
		if (resolved_networks_iter != resolved_networks.begin())
		{
			std::swap(resolved_networks.front(), *resolved_networks_iter);
		}

		return true;
	}

	return false;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_point_using_resolved_boundaries(
		const GPlatesMaths::PointOnSphere &point,
		TopologyPointLocation &location,
		rtb_seq_type &resolved_boundaries,
		plate_id_to_stage_rotation_map_type &resolved_boundary_stage_rotation_map,
		const double &current_time,
		const double &next_time)
{
	rtb_seq_type::iterator resolved_boundaries_iter = resolved_boundaries.begin();
	rtb_seq_type::iterator resolved_boundaries_end = resolved_boundaries.end();
	for ( ; resolved_boundaries_iter != resolved_boundaries_end; ++resolved_boundaries_iter)
	{
		const ResolvedTopologicalBoundary::non_null_ptr_type resolved_boundary = *resolved_boundaries_iter;

		// Note that the medium and high speed point-in-polygon tests include a quick small circle
		// bounds test so we don't need to perform that test before the point-in-polygon test.
		if (!resolved_boundary->resolved_topology_boundary()->is_point_in_polygon(
				point,
				GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
		{
			// The point is outside the resolved boundary so continue searching resolved boundaries.
			continue;
		}

		const boost::optional<GPlatesModel::integer_plate_id_type> resolved_boundary_plate_id = resolved_boundary->plate_id();
		if (!resolved_boundary_plate_id)
		{
			// Shouldn't happen - resolved boundary should have a plate ID - ignore if doesn't.
			continue;
		}

		// Store the resolved boundary containing the point.
		location = TopologyPointLocation(resolved_boundary);

		const GPlatesMaths::FiniteRotation &resolved_boundary_stage_rotation =
				get_or_create_stage_rotation(
						resolved_boundary_plate_id.get(),
						resolved_boundary->get_reconstruction_tree_creator(),
						current_time/*initial_time*/,
						next_time/*final_time*/,
						resolved_boundary_stage_rotation_map);

		// The next point is probably in the same resolved boundary so make it the first one to be tested next time.
		if (resolved_boundaries_iter != resolved_boundaries.begin())
		{
			std::swap(resolved_boundaries.front(), *resolved_boundaries_iter);
		}

		// Return reconstructed point.
		return resolved_boundary_stage_rotation * point;
	}

	return boost::none;
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::reconstruct_last_point_using_resolved_boundaries(
		const GPlatesMaths::PointOnSphere &point,
		TopologyPointLocation &location,
		rtb_seq_type &resolved_boundaries)
{
	rtb_seq_type::iterator resolved_boundaries_iter = resolved_boundaries.begin();
	rtb_seq_type::iterator resolved_boundaries_end = resolved_boundaries.end();
	for ( ; resolved_boundaries_iter != resolved_boundaries_end; ++resolved_boundaries_iter)
	{
		const ResolvedTopologicalBoundary::non_null_ptr_type resolved_boundary = *resolved_boundaries_iter;

		// Note that the medium and high speed point-in-polygon tests include a quick small circle
		// bounds test so we don't need to perform that test before the point-in-polygon test.
		if (!resolved_boundary->resolved_topology_boundary()->is_point_in_polygon(
				point,
				GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
		{
			// The point is outside the resolved boundary so continue searching resolved boundaries.
			continue;
		}

		const boost::optional<GPlatesModel::integer_plate_id_type> resolved_boundary_plate_id = resolved_boundary->plate_id();
		if (!resolved_boundary_plate_id)
		{
			// Shouldn't happen - resolved boundary should have a plate ID - ignore if doesn't.
			continue;
		}

		// Store the resolved boundary containing the point.
		location = TopologyPointLocation(resolved_boundary);

		// The next point is probably in the same resolved boundary so make it the first one to be tested next time.
		if (resolved_boundaries_iter != resolved_boundaries.begin())
		{
			std::swap(resolved_boundaries.front(), *resolved_boundaries_iter);
		}

		return true;
	}

	return false;
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::is_point_active(
		const GPlatesMaths::PointOnSphere &prev_point,
		const TopologyPointLocation &prev_location,
		const GPlatesMaths::PointOnSphere &current_point,
		const TopologyPointLocation &current_location,
		const double &current_time,
		const double &time_increment,
		bool reverse_reconstruct,
		const GPlatesMaths::AngularExtent &min_distance_threshold_radians,
		plate_id_to_stage_rotation_map_type &resolved_boundary_stage_rotation_map) const
{
	//
	// If transitioning:
	//   (1) from a deforming network to a rigid plate, or
	//   (2) from a rigid plate to a deforming network, or
	//   (3) from a rigid plate to a rigid plate with a different plate ID
	// ...then calculate the difference in velocities and continue testing as follows
	// (otherwise, if there's no transition, then the point is still active)...
	//
	// If the velocity difference is below a threshold then we assume the previous plate was split,
	// or two plates joined or deformation just started or ended. In this case the point has not subducted
	// (forward in time) or been consumed by a mid-ocean (backward in time) and hence is still active.
	//
	// If the velocity difference is large enough then we see if the distance of the *previous* position
	// to the polygon boundary (of the deforming network or rigid plate containing it) exceeds a threshold.
	// If the distance exceeds the threshold then the point is far enough away from the boundary that it
	// cannot be subducted or consumed by it and hence the point is still active.
	// However if the point is close enough then we assume the point was subducted/consumed
	// (remember that the point switched plate IDs or transitioned to/from a network).
	// Also note that the threshold distance increases according to the velocity difference to account for fast
	// moving points (that would otherwise tunnel through the boundary and accrete onto the other plate/network).
	// The reason for testing the distance from the *previous* point, and not from the *current* point, is:
	//
	//   (i)  A topological boundary may *appear* near the current point (such as a plate split at the current time)
	//        and we don't want that split to consume the current point regardless of the velocity difference.
	//        It won't get consumed because the *previous* point was not near a boundary (because before split happened).
	//        If the velocity difference is large enough then it might cause the current point to transition to the
	//        adjacent split plate in the *next* time step (and that's when it should get consumed, not in the current time step).
	//        An example of this is a mid-ocean ridge suddenly appearing (going forward in time).
	//
	//   (ii) A topological boundary may *disappear* near the current point (such as a plate merge at the current time)
	//        and we want that merge to consume the current point if the velocity difference is large enough.
	//        In this case the *previous* point is near a boundary (because before plate merged) and hence can be
	//        consumed (provided velocity difference is large enough). And since the boundary existed in the previous
	//        time step, it will affect position of the current point (and whether it gets consumed or not).
	//        An example of this is a mid-ocean ridge suddenly disappearing (going backward in time).
	// 
	// ...note that items (i) and (ii) above apply both going forward and backward in time.
	//

	const boost::optional<TopologyPointLocation::network_location_type> current_network_location =
			current_location.located_in_resolved_network();
	if (current_network_location)
	{
		const boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type> prev_boundary =
				prev_location.located_in_resolved_boundary();
		if (!prev_boundary)
		{
			return true;
		}

		const ResolvedTopologicalNetwork::non_null_ptr_type &current_resolved_network = current_network_location->first;

		boost::optional<
				std::pair<
						GPlatesMaths::Vector3D,
						boost::optional<const ResolvedTriangulation::Network::RigidBlock &> > > velocity_curr_point_curr_location_prev_time_result =
				current_resolved_network->get_triangulation_network().calculate_velocity(
						current_point,
						time_increment,
						// Note the use of delta-time is the same as if we had calculated velocity normally at the current time...
						reverse_reconstruct ? VelocityDeltaTime::T_PLUS_DELTA_T_TO_T : VelocityDeltaTime::T_TO_T_MINUS_DELTA_T,
						current_network_location->second);
		// Should get a result because we know point is inside the network.
		// If we don't, for some reason, then leave velocity as zero.
		GPlatesMaths::Vector3D velocity_curr_point_curr_location_prev_time;
		if (velocity_curr_point_curr_location_prev_time_result)
		{
			velocity_curr_point_curr_location_prev_time = velocity_curr_point_curr_location_prev_time_result->first;
		}

		// Should have a plate ID.
		// If we don't, for some reason, then leave velocity as zero.
		GPlatesMaths::Vector3D velocity_curr_point_prev_location_prev_time;
		const boost::optional<GPlatesModel::integer_plate_id_type> prev_boundary_plate_id = prev_boundary.get()->plate_id();
		if (prev_boundary_plate_id)
		{
			// Calculate the velocity of the *current* point using the previous resolved boundary plate ID.
			//
			// Note that even though the current point is not inside the previous boundary, we can still calculate
			// a velocity using its plate ID (because we really should use the same point in our velocity comparison).
			const GPlatesMaths::FiniteRotation &resolved_boundary_stage_rotation =
					get_or_create_velocity_stage_rotation(
							prev_boundary_plate_id.get(),
							prev_boundary.get()->get_reconstruction_tree_creator(),
							current_time,
							time_increment,
							// Note the use of delta-time is the same as if we had calculated velocity normally at the current time...
							reverse_reconstruct ? VelocityDeltaTime::T_PLUS_DELTA_T_TO_T : VelocityDeltaTime::T_TO_T_MINUS_DELTA_T,
							resolved_boundary_stage_rotation_map);
			velocity_curr_point_prev_location_prev_time = GPlatesMaths::calculate_velocity_vector(
					current_point,
					resolved_boundary_stage_rotation,
					time_increment);
		}

		const GPlatesMaths::Vector3D delta_velocity =
				velocity_curr_point_prev_location_prev_time - velocity_curr_point_curr_location_prev_time;

		return is_delta_velocity_small_enough_or_point_far_from_boundary(
				delta_velocity,
				// The polygon used for distance query...
				prev_boundary.get()->resolved_topology_boundary(),
				prev_point,
				time_increment,
				min_distance_threshold_radians);
	}

	const boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type> current_boundary =
			current_location.located_in_resolved_boundary();
	if (!current_boundary)
	{
		return true;
	}

	const boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type> prev_boundary =
			prev_location.located_in_resolved_boundary();
	if (prev_boundary)
	{
		const boost::optional<GPlatesModel::integer_plate_id_type> current_boundary_plate_id = current_boundary.get()->plate_id();
		const boost::optional<GPlatesModel::integer_plate_id_type> prev_boundary_plate_id = prev_boundary.get()->plate_id();
		if (current_boundary_plate_id == prev_boundary_plate_id)
		{
			return true;
		}

		// Should have a plate ID.
		// If we don't, for some reason, then leave velocity as zero.
		GPlatesMaths::Vector3D velocity_curr_point_curr_location_prev_time;
		if (current_boundary_plate_id)
		{
			// Calculate the velocity of the *current* point using the current resolved boundary plate ID.
			const GPlatesMaths::FiniteRotation &resolved_boundary_stage_rotation =
					get_or_create_velocity_stage_rotation(
							current_boundary_plate_id.get(),
							current_boundary.get()->get_reconstruction_tree_creator(),
							current_time,
							time_increment,
							// Note the use of delta-time is the same as if we had calculated velocity normally at the current time...
							reverse_reconstruct ? VelocityDeltaTime::T_PLUS_DELTA_T_TO_T : VelocityDeltaTime::T_TO_T_MINUS_DELTA_T,
							resolved_boundary_stage_rotation_map);
			velocity_curr_point_curr_location_prev_time = GPlatesMaths::calculate_velocity_vector(
					current_point,
					resolved_boundary_stage_rotation,
					time_increment);
		}

		// Should have a plate ID.
		// If we don't, for some reason, then leave velocity as zero.
		GPlatesMaths::Vector3D velocity_curr_point_prev_location_prev_time;
		if (prev_boundary_plate_id)
		{
			// Calculate the velocity of the *current* point using the previous resolved boundary plate ID.
			//
			// Note that even though the current point is not inside the previous boundary, we can still calculate
			// a velocity using its plate ID (because we really should use the same point in our velocity comparison).
			const GPlatesMaths::FiniteRotation &resolved_boundary_stage_rotation =
					get_or_create_velocity_stage_rotation(
							prev_boundary_plate_id.get(),
							prev_boundary.get()->get_reconstruction_tree_creator(),
							current_time,
							time_increment,
							// Note the use of delta-time is the same as if we had calculated velocity normally at the current time...
							reverse_reconstruct ? VelocityDeltaTime::T_PLUS_DELTA_T_TO_T : VelocityDeltaTime::T_TO_T_MINUS_DELTA_T,
							resolved_boundary_stage_rotation_map);
			velocity_curr_point_prev_location_prev_time = GPlatesMaths::calculate_velocity_vector(
					current_point,
					resolved_boundary_stage_rotation,
					time_increment);
		}

		const GPlatesMaths::Vector3D delta_velocity =
				velocity_curr_point_prev_location_prev_time - velocity_curr_point_curr_location_prev_time;

		return is_delta_velocity_small_enough_or_point_far_from_boundary(
				delta_velocity,
				// The polygon used for distance query...
				prev_boundary.get()->resolved_topology_boundary(),
				prev_point,
				time_increment,
				min_distance_threshold_radians);
	}

	const boost::optional<TopologyPointLocation::network_location_type> prev_network_location =
			prev_location.located_in_resolved_network();
	if (!prev_network_location)
	{
		return true;
	}

	// Calculate the velocity of the *previous* point using the current resolved boundary plate ID.
	const boost::optional<GPlatesModel::integer_plate_id_type> current_boundary_plate_id = current_boundary.get()->plate_id();
	// Should have a plate ID.
	// If we don't, for some reason, then leave velocity as zero.
	GPlatesMaths::Vector3D velocity_prev_point_curr_location_prev_time;
	if (current_boundary_plate_id)
	{
		const GPlatesMaths::FiniteRotation &resolved_boundary_stage_rotation =
				get_or_create_velocity_stage_rotation(
						current_boundary_plate_id.get(),
						current_boundary.get()->get_reconstruction_tree_creator(),
						current_time,
						time_increment,
						// Note the use of delta-time is the same as if we had calculated velocity normally at the current time...
						reverse_reconstruct ? VelocityDeltaTime::T_PLUS_DELTA_T_TO_T : VelocityDeltaTime::T_TO_T_MINUS_DELTA_T,
						resolved_boundary_stage_rotation_map);
		// Note that we test using the *previous* point (not the current point) because we need to compare
		// against the previous network and it can only calculate velocity at the previous point because
		// the current point is outside the previous network (it's in a resolved boundary).
		velocity_prev_point_curr_location_prev_time = GPlatesMaths::calculate_velocity_vector(
				prev_point,
				resolved_boundary_stage_rotation,
				time_increment);
	}

	const ResolvedTopologicalNetwork::non_null_ptr_type &prev_resolved_network = prev_network_location->first;

	// Calculate the velocity of the *previous* point using the previous resolved network.
	//
	// Note that we have to test using the *previous* point (not the current point) because
	// the current point is outside the network (it's in a resolved boundary).
	boost::optional<
			std::pair<
					GPlatesMaths::Vector3D,
					boost::optional<const ResolvedTriangulation::Network::RigidBlock &> > > velocity_prev_point_prev_location_prev_time_result =
			prev_resolved_network->get_triangulation_network().calculate_velocity(
					prev_point,
					time_increment,
					// Note the normal use of delta-time (since network is already at the previous time)...
					reverse_reconstruct ? VelocityDeltaTime::T_TO_T_MINUS_DELTA_T : VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					prev_network_location->second);
	// Should get a result because we know point is inside the network.
	// If we don't, for some reason, then leave velocity as zero.
	GPlatesMaths::Vector3D velocity_prev_point_prev_location_prev_time;
	if (velocity_prev_point_prev_location_prev_time_result)
	{
		velocity_prev_point_prev_location_prev_time = velocity_prev_point_prev_location_prev_time_result->first;
	}

	const GPlatesMaths::Vector3D delta_velocity_at_prev_time =
			velocity_prev_point_prev_location_prev_time - velocity_prev_point_curr_location_prev_time;

	return is_delta_velocity_small_enough_or_point_far_from_boundary(
			delta_velocity_at_prev_time,
			// The polygon used for distance query...
			prev_resolved_network->boundary_polygon(),
			prev_point,
			time_increment,
			min_distance_threshold_radians);
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::is_delta_velocity_small_enough_or_point_far_from_boundary(
		const GPlatesMaths::Vector3D &delta_velocity,
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &prev_topology_boundary,
		const GPlatesMaths::PointOnSphere &prev_point,
		const double &time_increment,
		const GPlatesMaths::AngularExtent &min_distance_threshold_radians) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_active_point_parameters,
			GPLATES_ASSERTION_SOURCE);
	const ActivePointParameters &active_point_parameters = d_active_point_parameters.get();

	// Optimisation: Avoid 'sqrt' unless needed.
	const double delta_velocity_magnitude_squared = delta_velocity.magSqrd().dval();
	if (delta_velocity_magnitude_squared <
		active_point_parameters.threshold_velocity_delta * active_point_parameters.threshold_velocity_delta)
	{
		// The change in velocity is small enough, so the current point remains active.
		return true;
	}

	// Convert our delta velocity to relative distance traveled.
	const double cms_yr_to_kms_my = 10;/*cms/yr -> kms/my*/
	const double delta_velocity_kms_per_my = cms_yr_to_kms_my * std::sqrt(delta_velocity_magnitude_squared);
	GPlatesMaths::Real delta_velocity_angle = delta_velocity_kms_per_my * time_increment * INVERSE_EARTH_EQUATORIAL_RADIUS_KMS;
	// We shouldn't get anywhere near the maximum possible angle, but check just to be sure an exception is not thrown.
	if (delta_velocity_angle >= GPlatesMaths::PI)
	{
		delta_velocity_angle = GPlatesMaths::PI;
	}
	const GPlatesMaths::AngularExtent delta_velocity_threshold =
			GPlatesMaths::AngularExtent::create_from_angle(delta_velocity_angle);

	// Add the minimum distance threshold to the delta velocity threshold.
	// The delta velocity threshold only allows those points that are close enough to the boundary to reach
	// it given their current relative velocity.
	// The minimum distance threshold accounts for sudden changes in the shape of a plate/network boundary
	// which are not supposed to represent a new or shifted boundary but are just a result of the topology
	// builder/user digitising a new boundary line that differs noticeably from that of the previous time period.
	const GPlatesMaths::AngularExtent distance_threshold_radians = min_distance_threshold_radians + delta_velocity_threshold;

	// If the distance from the previous point to the previous polygon boundary exceeds the threshold
	// then the current point remains active.
	if (GPlatesMaths::AngularExtent::PI == minimum_distance(
			prev_point,
			*prev_topology_boundary,
			false/*polygon_interior_is_solid*/,
			distance_threshold_radians))
	{
		return true;
	}

	// Deactivate the current point.
	return false;
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_resolved_topologies(
		rtb_seq_type &resolved_boundaries,
		rtn_seq_type &resolved_networks,
		unsigned int time_slot,
		const GeometrySample::non_null_ptr_type &geometry_sample) const
{
	// Get the resolved boundaries for the time slot.
	boost::optional<const rtb_seq_type &> resolved_boundaries_opt =
			d_topology_reconstruct->get_resolved_boundary_time_span()->get_sample_in_time_slot(time_slot);

	// Get the resolved networks for the time slot.
	boost::optional<const rtn_seq_type &> resolved_networks_opt =
			d_topology_reconstruct->get_resolved_network_time_span()->get_sample_in_time_slot(time_slot);

	// If there are no boundaries and no networks for the time slot then return early.
	const bool have_topology_surfaces =
			(resolved_boundaries_opt && !resolved_boundaries_opt->empty()) ||
			(resolved_networks_opt && !resolved_networks_opt->empty());
	if (!have_topology_surfaces)
	{
		return false;
	}

	// Make a copy of the list of boundaries/networks.
	// We will then can cull those that can't possibly intersect the geometry sample.
	if (resolved_boundaries_opt)
	{
		resolved_boundaries = resolved_boundaries_opt.get();
	}
	if (resolved_networks_opt)
	{
		resolved_networks = resolved_networks_opt.get();
	}

	const std::vector<GeometryPoint *> &geometry_points = geometry_sample->get_geometry_points(d_accessing_strain_rates);
	const unsigned int num_geometry_points = geometry_points.size();

	// Iterate through the points and calculate the sum of vertex positions.
	GPlatesMaths::Vector3D sum_point_positions(0,0,0);
	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *geometry_point = geometry_points[geometry_point_index];

		// Ignore point if it's not active.
		if (geometry_point)
		{
			sum_point_positions = sum_point_positions + GPlatesMaths::Vector3D(geometry_point->position);
		}
	}

	// If can calculate a centroid of the geometry points then form a bounding circle around them to cull with.
	if (!sum_point_positions.is_zero_magnitude())
	{
		const GPlatesMaths::UnitVector3D centroid_point_positions = sum_point_positions.get_normalisation();
		GPlatesMaths::BoundingSmallCircleBuilder geometry_points_small_circle_bounds_builder(centroid_point_positions);

		// Note that we don't need to worry about adding great circle arcs (if the geometry type is a
		// polyline or polygon) because we only test if the points intersect the resolved boundaries/networks.
		// If interior arc sub-segment of a great circle arc (polyline/polygon edge) intersects
		// resolved boundary/network it doesn't matter (only the arc end points matter).
		for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
		{
			GeometryPoint *geometry_point = geometry_points[geometry_point_index];

			// Ignore point if it's not active.
			if (geometry_point)
			{
				geometry_points_small_circle_bounds_builder.add(geometry_point->position);
			}
		}

		const GPlatesMaths::BoundingSmallCircle geometry_points_small_circle_bounds =
				geometry_points_small_circle_bounds_builder.get_bounding_small_circle();

		if (!resolved_boundaries.empty())
		{
			resolved_boundaries.erase(
					std::remove_if(
							resolved_boundaries.begin(),
							resolved_boundaries.end(),
							std::not1(IntersectGeometryPointsAndResolvedBoundarySmallCircleBounds(
									&geometry_points_small_circle_bounds))),
					resolved_boundaries.end());
		}

		if (!resolved_networks.empty())
		{
			resolved_networks.erase(
					std::remove_if(
							resolved_networks.begin(),
							resolved_networks.end(),
							std::not1(IntersectGeometryPointsAndResolvedNetworkSmallCircleBounds(
									&geometry_points_small_circle_bounds))),
					resolved_networks.end());
		}
	}

	// Return true if there are any remaining topology surfaces.
	return !resolved_boundaries.empty() || !resolved_networks.empty();
}


const GPlatesMaths::FiniteRotation &
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_or_create_stage_rotation(
		GPlatesModel::integer_plate_id_type reconstruction_plate_id,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &initial_time,
		const double &final_time,
		plate_id_to_stage_rotation_map_type &stage_rotation_map) const
{
	// See if already exists.
	plate_id_to_stage_rotation_map_type::const_iterator stage_rotation_iter =
			stage_rotation_map.find(reconstruction_plate_id);
	if (stage_rotation_iter != stage_rotation_map.end())
	{
		return stage_rotation_iter->second;
	}

	// Calculate stage rotation and insert into the map.
	const std::pair<plate_id_to_stage_rotation_map_type::iterator, bool> insert_result =
			stage_rotation_map.insert(
					plate_id_to_stage_rotation_map_type::value_type(
							reconstruction_plate_id,
							get_stage_rotation(
									reconstruction_plate_id,
									reconstruction_tree_creator,
									initial_time,
									final_time)));

	return insert_result.first->second;
}


const GPlatesMaths::FiniteRotation &
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_or_create_velocity_stage_rotation(
		GPlatesModel::integer_plate_id_type reconstruction_plate_id,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		plate_id_to_stage_rotation_map_type &stage_rotation_map) const
{
	// See if already exists.
	plate_id_to_stage_rotation_map_type::const_iterator stage_rotation_iter =
			stage_rotation_map.find(reconstruction_plate_id);
	if (stage_rotation_iter != stage_rotation_map.end())
	{
		return stage_rotation_iter->second;
	}

	// Calculate stage rotation and insert into the map.
	const std::pair<plate_id_to_stage_rotation_map_type::iterator, bool> insert_result =
			stage_rotation_map.insert(
					plate_id_to_stage_rotation_map_type::value_type(
							reconstruction_plate_id,
							PlateVelocityUtils::calculate_stage_rotation(
									reconstruction_plate_id,
									reconstruction_tree_creator,
									reconstruction_time,
									velocity_delta_time,
									velocity_delta_time_type)));

	return insert_result.first->second;
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::rigid_reconstruct(
		const GeometrySample::non_null_ptr_type &geometry_sample,
		const double &reconstruction_time,
		bool reverse_reconstruct,
		boost::optional<PoolAllocator::non_null_ptr_type> pool_allocator) const
{
	GPlatesMaths::FiniteRotation rotation =
			d_topology_reconstruct->get_reconstruction_tree_creator()
					.get_reconstruction_tree(reconstruction_time)
							->get_composed_absolute_rotation(d_reconstruction_plate_id).first;

	if (reverse_reconstruct)
	{
		rotation = get_reverse(rotation);
	}

	// Create a new rotated geometry sample.
	return rotate_geometry_sample(geometry_sample, rotation, pool_allocator);
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::rigid_stage_reconstruct(
		const GeometrySample::non_null_ptr_type &geometry_sample,
		const double &initial_time,
		const double &final_time,
		boost::optional<PoolAllocator::non_null_ptr_type> pool_allocator) const
{
	const GPlatesMaths::FiniteRotation initial_to_final_rotation =
			get_stage_rotation(
					d_reconstruction_plate_id,
					d_topology_reconstruct->get_reconstruction_tree_creator(),
					initial_time,
					final_time);

	// Create a new rotated geometry sample.
	return rotate_geometry_sample(geometry_sample, initial_to_final_rotation, pool_allocator);
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::rotate_geometry_sample(
		const GeometrySample::non_null_ptr_type &geometry_sample,
		const GPlatesMaths::FiniteRotation &rotation,
		boost::optional<PoolAllocator::non_null_ptr_type> pool_allocator) const
{
	if (!pool_allocator)
	{
		// We're not storing this sample in our time span so don't share our pool allocator.
		// Sample gets its own allocator which means it releases its memory when it's no longer needed.
		// This is important since otherwise memory usage would continually increase each time
		// a geometry sample outside the time windows (in the time range) was requested.
		pool_allocator = PoolAllocator::create();
	}

	// If using the same pool allocator then can share allocated objects.
	const bool sharing_pool_allocator = (pool_allocator.get() == d_pool_allocator);

	const std::vector<GeometryPoint *> &geometry_points = geometry_sample->get_geometry_points(d_accessing_strain_rates);

	const unsigned int num_geometry_points = geometry_points.size();

	std::vector<GeometryPoint *> rotated_geometry_points;
	rotated_geometry_points.resize(num_geometry_points, NULL);

	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		const GeometryPoint *geometry_point = geometry_points[geometry_point_index];

		// Ignore current point if it's not active.
		if (geometry_point == NULL)
		{
			continue;
		}

		// Rigidly reconstruct the sample point.
		const GPlatesMaths::PointOnSphere rotated_point(rotation * geometry_point->position);

		GeometryPoint *rotated_geometry_point = pool_allocator.get()->geometry_point_pool.construct(rotated_point);

		if (d_accessing_strains)
		{
			// Also copy the per-point (total) strains.
			// There is no deformation during rigid time spans so the *instantaneous* deformation (strain rate) is zero.
			// But the *accumulated* deformation (strain) is propagated across gaps between time windows.
			if (geometry_point->strain)
			{
				if (sharing_pool_allocator)
				{
					rotated_geometry_point->strain = geometry_point->strain;
				}
				else
				{
					// Copy into a new strain object since we can't share the same object (because using our own allocator).
					rotated_geometry_point->strain = pool_allocator.get()->deformation_strain_pool.construct(*geometry_point->strain);
				}
			}
		}

		rotated_geometry_points[geometry_point_index] = rotated_geometry_point;
	}

	// Create a new geometry sample.
	return GeometrySample::create_swap(rotated_geometry_points, pool_allocator.get());
}


void
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::initialise_deformation_total_strains() const
{
	// We'll be accessing strain rates to accumulate total strains.
	AccessingStrainRates accessing_strain_rates(*this);

	// The time range of the geometry samples.
	const unsigned int num_time_slots = d_time_range.get_num_time_slots();

	// We need to convert time increment from My to seconds.
	static const double SECONDS_IN_A_MILLION_YEARS = 365.25 * 24 * 3600 * 1.0e6;
	const double time_increment_in_seconds = SECONDS_IN_A_MILLION_YEARS * d_time_range.get_time_increment();

	boost::optional<GeometrySample::non_null_ptr_type &> most_recent_geometry_sample =
			d_time_window_span->get_sample_in_time_slot(0);

	// Iterate over the time range going *forward* in time from the beginning of the
	// time range (least recent) to the end (most recent).
	for (unsigned int time_slot = 1; time_slot < num_time_slots; ++time_slot)
	{
		// Get the geometry sample for the current time slot.
		boost::optional<GeometrySample::non_null_ptr_type &> current_geometry_sample =
				d_time_window_span->get_sample_in_time_slot(time_slot);

		if (!current_geometry_sample)
		{
			most_recent_geometry_sample = current_geometry_sample.get();
			// Skip the current geometry sample - all its points are inactive.
			continue;
		}

		const std::vector<GeometryPoint *> &current_geometry_points =
				current_geometry_sample.get()->get_geometry_points(d_accessing_strain_rates);

		const unsigned int num_geometry_points = current_geometry_points.size();

		if (most_recent_geometry_sample)
		{
			const std::vector<GeometryPoint *> &most_recent_geometry_points =
					most_recent_geometry_sample.get()->get_geometry_points(d_accessing_strain_rates);

			// The number of points in each geometry sample should be the same.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						most_recent_geometry_points.size() == num_geometry_points,
						GPLATES_ASSERTION_SOURCE);

			// Iterate over the most recent and current geometry sample points.
			for (unsigned int point_index = 0; point_index < num_geometry_points; ++point_index)
			{
				GeometryPoint *current_geometry_point = current_geometry_points[point_index];

				// Ignore current point if it's not active.
				if (current_geometry_point == NULL)
				{
					continue;
				}

				const GeometryPoint *most_recent_geometry_point = most_recent_geometry_points[point_index];

				if (current_geometry_point->strain_rate ||
					(most_recent_geometry_point && most_recent_geometry_point->strain_rate))
				{
					DeformationStrain most_recent_strain; // Default to identity strain.
					DeformationStrainRate most_recent_strain_rate; // Default to zero strain rate.
					DeformationStrainRate current_strain_rate; // Default to zero strain rate.

					// If most recent point is active and has a non-zero strain or strain rate...
					if (most_recent_geometry_point)
					{
						if (most_recent_geometry_point->strain)
						{
							most_recent_strain = *most_recent_geometry_point->strain;
						}
						if (most_recent_geometry_point->strain_rate)
						{
							most_recent_strain_rate = *most_recent_geometry_point->strain_rate;
						}
					}

					// If current point has a non-zero strain rate...
					if (current_geometry_point->strain_rate)
					{
						current_strain_rate = *current_geometry_point->strain_rate;
					}

					// Compute new strain for the current geometry point using the strain at the
					// most recent point and the strain rate at the current sample.
					const DeformationStrain current_strain = accumulate_strain(
							most_recent_strain,
							most_recent_strain_rate,
							current_strain_rate,
							time_increment_in_seconds);
					current_geometry_point->strain = d_pool_allocator->deformation_strain_pool.construct(current_strain);
				}
				else
				{
					// Both the most recent and current strain rates are zero so the current strain
					// remains the same as the most recent strain.
					//
					// We can share the strain object since all geometry samples in the time span used the same pool allocator.
					if (most_recent_geometry_point)
					{
						current_geometry_point->strain = most_recent_geometry_point->strain;
					}
					// ...else leave current strain as NULL.
				}
			}
		}
		else
		{
			// There is no most recent geometry sample which means the most recent strains and strain rates are zero.

			// Iterate over the current geometry sample points.
			for (unsigned int point_index = 0; point_index < num_geometry_points; ++point_index)
			{
				GeometryPoint *current_geometry_point = current_geometry_points[point_index];

				// Ignore current point if it's not active.
				if (current_geometry_point == NULL)
				{
					continue;
				}

				// If the current strain rate is zero then the current strain is also zero (so leave as NULL).
				// Otherwise update the current strain.
				if (current_geometry_point->strain_rate)
				{
					const DeformationStrainRate &current_strain_rate = *current_geometry_point->strain_rate;

					// Compute new strain for the current geometry sample assuming zero strain and strain rate for most recent sample.
					const DeformationStrain current_strain = accumulate_strain(
							DeformationStrain()/*most_recent_strain*/,
							DeformationStrainRate()/*most_recent_strain_rate*/,
							current_strain_rate,
							time_increment_in_seconds);
					current_geometry_point->strain = d_pool_allocator->deformation_strain_pool.construct(current_strain);
				}
				// ...else leave current strain as NULL.
			}
		}

		most_recent_geometry_sample = current_geometry_sample.get();
	}

	// Transfer the final accumulated values to the present-day sample.
	//
	// This ensures reconstructions between the end of the time range and present-day will
	// have the final accumulated values (because they will get carried over from the present-day
	// sample when it is rigidly rotated to the reconstruction time).
	if (most_recent_geometry_sample)
	{
		// There is no deformation during rigid time spans so the *instantaneous* deformations (strain rates) are zero.
		// But the *accumulated* deformation (strain) is propagated across gaps between time windows.

		const std::vector<GeometryPoint *> &most_recent_geometry_points =
				most_recent_geometry_sample.get()->get_geometry_points(d_accessing_strain_rates);

		const unsigned int num_geometry_points = most_recent_geometry_points.size();

		const std::vector<GeometryPoint *> &present_day_geometry_points =
				d_time_window_span->get_present_day_sample()->get_geometry_points(d_accessing_strain_rates);

		// The number of points in each geometry sample should be the same.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					present_day_geometry_points.size() == num_geometry_points,
					GPLATES_ASSERTION_SOURCE);

		for (unsigned int point_index = 0; point_index < num_geometry_points; ++point_index)
		{
			GeometryPoint *present_day_geometry_point = present_day_geometry_points[point_index];

			// Ignore present day point if it's not active.
			if (present_day_geometry_point == NULL)
			{
				continue;
			}

			GeometryPoint *most_recent_geometry_point = most_recent_geometry_points[point_index];

			// We can share the strain object since all geometry samples in the time span used the same pool allocator.
			if (most_recent_geometry_point)
			{
				present_day_geometry_point->strain = most_recent_geometry_point->strain;
			}
			// ...else leave present day strain as NULL.
		}
	}

	d_have_initialised_strains = true;
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::is_valid(
		const double &reconstruction_time) const
{
	if (d_time_slot_of_appearance ||
		d_time_slot_of_disappearance)
	{
		// Determine the two nearest time slots bounding the reconstruction time (if any).
		double interpolate_time_slots;
		const boost::optional< std::pair<unsigned int/*first_time_slot*/, unsigned int/*second_time_slot*/> >
				reconstruction_time_slots = d_time_range.get_bounding_time_slots(reconstruction_time, interpolate_time_slots);
		if (reconstruction_time_slots)
		{
			// If the geometry has a time of appearance (time slot in the time range) and the reconstruction
			// time slot is prior to it then the geometry has not appeared yet.
			if (d_time_slot_of_appearance &&
				reconstruction_time_slots->first < d_time_slot_of_appearance.get())
			{
				return false;
			}

			// If the geometry has a time of disappearance (time slot in the time range) and the reconstruction
			// time slot is after it then the geometry has already disappeared.
			if (d_time_slot_of_disappearance &&
				reconstruction_time_slots->second > d_time_slot_of_disappearance.get())
			{
				return false;
			}
		}
		else // reconstruction time is outside the time range...
		{
			// If the geometry has a time of appearance (time slot in the time range) and the reconstruction
			// time is prior to the beginning of the time range then the geometry has not appeared yet.
			if (d_time_slot_of_appearance &&
				reconstruction_time >= d_time_range.get_begin_time())
			{
				return false;
			}

			// If the geometry has a time of disappearance (time slot in the time range) and the reconstruction
			// time is after the end of the time range then the geometry has already disappeared.
			if (d_time_slot_of_disappearance &&
				reconstruction_time <= d_time_range.get_end_time())
			{
				return false;
			}
		}
	}

	return true;
}


boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_geometry(
		const double &reconstruction_time) const
{
	boost::optional<GeometrySample::non_null_ptr_type> geometry_sample = get_geometry_sample(reconstruction_time);
	if (!geometry_sample)
	{
		// The geometry is not valid/active at the reconstruction time.
		return boost::none;
	}

	const std::vector<GeometryPoint *> &geometry_points = geometry_sample.get()->get_geometry_points(d_accessing_strain_rates);
	const unsigned int num_geometry_points = geometry_points.size();

	// See if original geometry was a point.
	if (num_geometry_points == 1)
	{
		if (geometry_points.front() == NULL)
		{
			// The point geometry is not valid/active at the reconstruction time.
			// Note that the sole point should not be inactive because otherwise the 'get_geometry_sample()'
			// call above would have failed.
			return boost::none;
		}

		// Return as a PointOnSphere.
		return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
				GPlatesMaths::PointOnSphere::create_on_heap(geometry_points.front()->position));
	}
	// ...else return geometry as a multipoint...

	std::vector<GPlatesMaths::PointOnSphere> points;
	points.reserve(num_geometry_points);

	// Get the active geometry points.
	// Note that they should not all be inactive because otherwise the 'get_geometry_sample()' call above would have failed.
	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *geometry_point = geometry_points[geometry_point_index];

		// Ignore geometry point if it's not active.
		if (geometry_point == NULL)
		{
			continue;
		}

		points.push_back(GPlatesMaths::PointOnSphere(geometry_point->position));
	}

	// Return as a MultiPointOnSphere.
	return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
			GPlatesMaths::MultiPointOnSphere::create_on_heap(points));
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_geometry_data(
		const double &reconstruction_time,
		boost::optional< std::vector<GPlatesMaths::PointOnSphere> &> points,
		boost::optional< std::vector<DeformationStrainRate> &> strain_rates,
		boost::optional< std::vector<DeformationStrain> &> strains) const
{
	// If we'll be accessing strain rates.
	boost::optional<AccessingStrainRates> accessing_strain_rates;
	if (strain_rates)
	{
		accessing_strain_rates = boost::in_place(*this);
	}

	// If we'll be accessing strains.
	boost::optional<AccessingStrains> accessing_strains;
	if (strains)
	{
		accessing_strains = boost::in_place(*this);
	}

	boost::optional<GeometrySample::non_null_ptr_type> geometry_sample = get_geometry_sample(reconstruction_time);
	if (!geometry_sample)
	{
		// The geometry is not valid/active at the reconstruction time.
		return false;
	}

	const std::vector<GeometryPoint *> &geometry_points = geometry_sample.get()->get_geometry_points(d_accessing_strain_rates);
	const unsigned int num_geometry_points = geometry_points.size();

	if (points)
	{
		points->reserve(num_geometry_points);
	}
	if (strain_rates)
	{
		strain_rates->reserve(num_geometry_points);
	}
	if (strains)
	{
		strains->reserve(num_geometry_points);
	}

	// Get the active geometry points.
	// Note that they should not all be inactive because otherwise the 'get_geometry_sample()' call above would have failed.
	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *geometry_point = geometry_points[geometry_point_index];

		// Ignore geometry point if it's not active.
		if (geometry_point == NULL)
		{
			continue;
		}

		if (points)
		{
			points->push_back(GPlatesMaths::PointOnSphere(geometry_point->position));
		}

		if (strain_rates)
		{
			if (geometry_point->strain_rate)
			{
				strain_rates->push_back(*geometry_point->strain_rate);
			}
			else
			{
				// Strain rate is zero.
				strain_rates->push_back(DeformationStrainRate());
			}
		}

		if (strains)
		{
			if (geometry_point->strain)
			{
				strains->push_back(*geometry_point->strain);
			}
			else
			{
				// Strain is zero.
				strains->push_back(DeformationStrain());
			}
		}
	}

	return true;
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_all_geometry_data(
		const double &reconstruction_time,
		boost::optional< std::vector< boost::optional<GPlatesMaths::PointOnSphere> > &> points,
		boost::optional< std::vector< boost::optional<DeformationStrainRate> > &> strain_rates,
		boost::optional< std::vector< boost::optional<DeformationStrain> > &> strains) const
{
	// If we'll be accessing strain rates.
	boost::optional<AccessingStrainRates> accessing_strain_rates;
	if (strain_rates)
	{
		accessing_strain_rates = boost::in_place(*this);
	}

	// If we'll be accessing strains.
	boost::optional<AccessingStrains> accessing_strains;
	if (strains)
	{
		accessing_strains = boost::in_place(*this);
	}

	boost::optional<GeometrySample::non_null_ptr_type> geometry_sample = get_geometry_sample(reconstruction_time);
	if (!geometry_sample)
	{
		// The geometry is not valid/active at the reconstruction time.
		return false;
	}

	const std::vector<GeometryPoint *> &geometry_points = geometry_sample.get()->get_geometry_points(d_accessing_strain_rates);
	const unsigned int num_geometry_points = geometry_points.size();

	if (points)
	{
		points->reserve(num_geometry_points);
	}
	if (strain_rates)
	{
		strain_rates->reserve(num_geometry_points);
	}
	if (strains)
	{
		strains->reserve(num_geometry_points);
	}

	// Get the active geometry points.
	// Note that they should not all be inactive because otherwise the 'get_geometry_sample()' call above would have failed.
	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *geometry_point = geometry_points[geometry_point_index];

		if (geometry_point)
		{
			if (points)
			{
				points->push_back(GPlatesMaths::PointOnSphere(geometry_point->position));
			}

			if (strain_rates)
			{
				if (geometry_point->strain_rate)
				{
					strain_rates->push_back(*geometry_point->strain_rate);
				}
				else
				{
					// Strain rate is zero.
					strain_rates->push_back(DeformationStrainRate());
				}
			}

			if (strains)
			{
				if (geometry_point->strain)
				{
					strains->push_back(*geometry_point->strain);
				}
				else
				{
					// Strain is zero.
					strains->push_back(DeformationStrain());
				}
			}
		}
		else // inactive point...
		{
			if (points)
			{
				// Inactive point.
				points->push_back(boost::none);
			}

			if (strain_rates)
			{
				// Inactive point.
				strain_rates->push_back(boost::none);
			}

			if (strains)
			{
				// Inactive point.
				strains->push_back(boost::none);
			}
		}
	}

	return true;
}


bool
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_velocities(
		std::vector<GPlatesMaths::PointOnSphere> &domain_points,
		std::vector<GPlatesMaths::Vector3D> &velocities,
		std::vector< boost::optional<const ReconstructionGeometry *> > &surfaces,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	// Determine the two nearest time slots bounding the reconstruction time.
	double interpolate_time_slots;
	const boost::optional< std::pair<unsigned int/*first_time_slot*/, unsigned int/*second_time_slot*/> >
			bounding_time_slots = d_time_range.get_bounding_time_slots(reconstruction_time, interpolate_time_slots);
	// If outside the time range then no interpolation between two time slot velocities is necessary.
	if (!bounding_time_slots ||
		// See if the reconstruction time coincides with a time slot.
		// This is another case where no interpolation between two time slot velocities is necessary...
		bounding_time_slots->first == bounding_time_slots->second/*both time slots equal*/)
	{
		// Get the geometry (domain) points.
		boost::optional<GeometrySample::non_null_ptr_type> domain_sample = get_geometry_sample(reconstruction_time);
		if (!domain_sample)
		{
			// The geometry is not valid/active at the reconstruction time.
			return false;
		}

		// Note that if outside time range then the topology point locations will not include
		// any resolved boundaries/networks (and hence all velocities will be rigid stage rotations).
		calc_velocities(
				domain_sample.get(),
				domain_points,
				velocities,
				surfaces,
				reconstruction_time,
				velocity_delta_time,
				velocity_delta_time_type);

		return true;
	}

	// Use the interpolated domain positions at the reconstruction time.
	boost::optional<GeometrySample::non_null_ptr_type> domain_sample = get_geometry_sample(reconstruction_time);
	if (!domain_sample)
	{
		// The geometry is not valid/active at the reconstruction time.
		return false;
	}

	//
	// Use the velocities of the geometry sample at the nearest time slot closer to the geometry import time.
	//
	// This mirrors what 'interpolate_geometry_sample()' does (which is called internally when we called
	// 'get_geometry_sample(reconstruction_time)' above). This is important because we then calculate
	// velocities using the same geometry sample and hence the number of active points will match.
	//

	const double initial_time = d_time_range.get_time(
			(reconstruction_time > d_geometry_import_time)
			? bounding_time_slots->second  // second time slot is closer to geometry import time
			: bounding_time_slots->first); // first time slot is closer to geometry import time

	// Get the geometry (domain) points at the initial time slot.
	boost::optional<GeometrySample::non_null_ptr_type> initial_domain_sample = get_geometry_sample(initial_time);
	if (!initial_domain_sample)
	{
		// The geometry is not valid/active at the initial time.
		// This actually shouldn't happen since we passed this test at the reconstruction time above
		// and hence all geometry samples closer to the geometry import time should also be active.
		// But we'll check just in case.
		return false;
	}

	// Calculate velocities at the initial time slot.
	std::vector<GPlatesMaths::PointOnSphere> initial_points;
	calc_velocities(
			initial_domain_sample.get(),
			initial_points,
			velocities,
			surfaces,
			initial_time,
			velocity_delta_time,
			velocity_delta_time_type);

	const std::vector<GeometryPoint *> &domain_geometry_points =
			domain_sample.get()->get_geometry_points(d_accessing_strain_rates);

	const unsigned int num_domain_geometry_points = domain_geometry_points.size();

	domain_points.reserve(num_domain_geometry_points);

	// Return the interpolated domain positions.
	for (unsigned int domain_geometry_point_index = 0;
		domain_geometry_point_index < num_domain_geometry_points;
		++domain_geometry_point_index)
	{
		GeometryPoint *domain_geometry_point = domain_geometry_points[domain_geometry_point_index];

		// Ignore domain geometry point if it's not active.
		if (domain_geometry_point == NULL)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere domain_point(domain_geometry_point->position);
		domain_points.push_back(domain_point);
	}

	// Both the reconstruction time geometry sample and the initial time sample should have
	// the same number of active points. This is due to 'interpolate_geometry_sample()' using
	// the nearest time slot that is closer to the geometry import time and hence both samples are
	// essentially the same (same active geometry points, just with different positions).
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			domain_points.size() == velocities.size(),
			GPLATES_ASSERTION_SOURCE);

	return true;
}


void
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::calc_velocities(
		const GeometrySample::non_null_ptr_type &domain_geometry_sample,
		std::vector<GPlatesMaths::PointOnSphere> &domain_points,
		std::vector<GPlatesMaths::Vector3D> &velocities,
		std::vector< boost::optional<const ReconstructionGeometry *> > &surfaces,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	//
	// Calculate the velocities at the geometry (domain) points.
	//

	const std::vector<GeometryPoint *> &domain_geometry_points =
			domain_geometry_sample->get_geometry_points(d_accessing_strain_rates);

	const unsigned int num_domain_geometry_points = domain_geometry_points.size();

	domain_points.reserve(num_domain_geometry_points);
	velocities.reserve(num_domain_geometry_points);
	surfaces.reserve(num_domain_geometry_points);

	// Only calculate rigid stage rotation if some points need to be rigidly rotated.
	boost::optional<GPlatesMaths::FiniteRotation> rigid_stage_rotation;

	// Keep track of the stage rotations of resolved boundaries as we encounter them.
	// This is an optimisation since many points can be inside the same resolved boundary.
	plate_id_to_stage_rotation_map_type resolved_boundary_stage_rotation_map;

	// Iterate over the domain points and calculate their velocities (and surfaces).
	for (unsigned int domain_geometry_point_index = 0;
		domain_geometry_point_index < num_domain_geometry_points;
		++domain_geometry_point_index)
	{
		GeometryPoint *domain_geometry_point = domain_geometry_points[domain_geometry_point_index];

		// Ignore domain geometry point if it's not active.
		if (domain_geometry_point == NULL)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere domain_point(domain_geometry_point->position);
		const TopologyPointLocation &topology_point_location = domain_geometry_point->location;

		// Get the resolved network point location that the current point lies within (if any).
		const boost::optional<TopologyPointLocation::network_location_type> network_point_location =
				topology_point_location.located_in_resolved_network();
		if (network_point_location)
		{
			const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_network = network_point_location->first;
			const ResolvedTriangulation::Network::point_location_type &point_location = network_point_location->second;

			boost::optional<
					std::pair<
							GPlatesMaths::Vector3D,
							boost::optional<const ResolvedTriangulation::Network::RigidBlock &> > >
					velocity = resolved_network->get_triangulation_network().calculate_velocity(
							domain_point,
							velocity_delta_time,
							velocity_delta_time_type,
							point_location);
			if (velocity)
			{
				const GPlatesMaths::Vector3D &velocity_vector = velocity->first;

				// If the point was in one of the network's rigid blocks.
				if (velocity->second)
				{
					const ResolvedTriangulation::Network::RigidBlock &rigid_block = velocity->second.get();
					const ReconstructionGeometry *velocity_recon_geom =
							rigid_block.get_reconstructed_feature_geometry().get();

					domain_points.push_back(domain_point);
					velocities.push_back(velocity_vector);
					surfaces.push_back(velocity_recon_geom);

					// Continue to the next domain point.
					continue;
				}

				const ReconstructionGeometry *velocity_recon_geom = resolved_network.get();

				domain_points.push_back(domain_point);
				velocities.push_back(velocity_vector);
				surfaces.push_back(velocity_recon_geom);

				// Continue to the next domain point.
				continue;
			}
		}

		// Get the resolved boundary point location that the current point lies within (if any).
		const boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_boundary =
				topology_point_location.located_in_resolved_boundary();
		if (resolved_boundary)
		{
			// Get the plate ID from resolved boundary.
			const boost::optional<GPlatesModel::integer_plate_id_type> resolved_boundary_plate_id =
					resolved_boundary.get()->plate_id();
			if (resolved_boundary_plate_id)
			{
				const GPlatesMaths::FiniteRotation &resolved_boundary_stage_rotation =
						get_or_create_velocity_stage_rotation(
								resolved_boundary_plate_id.get(),
								resolved_boundary.get()->get_reconstruction_tree_creator(),
								reconstruction_time,
								velocity_delta_time,
								velocity_delta_time_type,
								resolved_boundary_stage_rotation_map);

				// Calculate the velocity of the point inside the resolved boundary.
				const GPlatesMaths::Vector3D velocity_vector =
						GPlatesMaths::calculate_velocity_vector(
								domain_point,
								resolved_boundary_stage_rotation,
								velocity_delta_time);

				const ReconstructionGeometry *velocity_recon_geom = resolved_boundary.get().get();

				domain_points.push_back(domain_point);
				velocities.push_back(velocity_vector);
				surfaces.push_back(velocity_recon_geom);

				// Continue to the next domain point.
				continue;
			}
		}

		//
		// Domain point was not in a resolved boundary or network (or there were no resolved boundaries/networks).
		// So calculate velocity using rigid rotation.
		//

		// Only need to calculate the stage rotation once.
		if (!rigid_stage_rotation)
		{
			rigid_stage_rotation = PlateVelocityUtils::calculate_stage_rotation(
					d_reconstruction_plate_id,
					d_topology_reconstruct->get_reconstruction_tree_creator(),
					reconstruction_time,
					velocity_delta_time,
					velocity_delta_time_type);
		}

		// Calculate the velocity.
		const GPlatesMaths::Vector3D velocity_vector =
				GPlatesMaths::calculate_velocity_vector(
						domain_point,
						rigid_stage_rotation.get(),
						velocity_delta_time);

		// Add the velocity - there was no surface (ie, resolved boundary/network) intersection though.
		domain_points.push_back(domain_point);
		velocities.push_back(velocity_vector);
		surfaces.push_back(boost::none/*surface*/);
	}
}


boost::optional<GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type>
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::get_geometry_sample(
		const double &reconstruction_time) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry is not valid/active at the reconstruction time.
		return boost::none;
	}

	// If total strains have been requested then generate them if they haven't already been generated.
	if (d_accessing_strains &&
		!d_have_initialised_strains)
	{
		initialise_deformation_total_strains();
	}

	// Look up the geometry sample in the time window span.
	// This performs rigid rotation from the closest younger (deformed) geometry sample if needed.
	return d_time_window_span->get_or_create_sample(reconstruction_time);
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::create_rigid_geometry_sample(
		const double &reconstruction_time,
		const double &closest_younger_sample_time,
		const GeometrySample::non_null_ptr_type &closest_younger_sample) const
{
	// Create a new geometry sample that has points rigidly reconstructed from youngest geometry sample.
	//
	// Note that the new geometry sample gets its own pool allocator (rather than sharing our allocator)
	// because we can get called for many reconstruction times and, for each call, the memory allocated
	// would continually increase if we didn't do this...
	return rigid_stage_reconstruct(
			closest_younger_sample,
			closest_younger_sample_time/*initial_time*/,
			reconstruction_time/*final_time*/);
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::interpolate_geometry_sample(
		const double &interpolate_position,
		const double &first_geometry_time,
		const double &second_geometry_time,
		const GeometrySample::non_null_ptr_type &first_geometry_sample,
		const GeometrySample::non_null_ptr_type &second_geometry_sample) const
{
	const double reconstruction_time =
			(1 - interpolate_position) * first_geometry_time +
				interpolate_position * second_geometry_time;

	// Determine whether to reconstruct backward or forward in time when interpolating points.
	double initial_time;
	const std::vector<GeometryPoint *> *initial_geometry_points;
	const std::vector<GeometryPoint *> *final_geometry_points;
	double time_increment;
	bool reverse_reconstruct;
	double interpolate_initial_to_final_position;
	if (reconstruction_time > d_geometry_import_time)
	{
		// Reconstruct backward in time away from the geometry import time.
		initial_time = second_geometry_time;
		initial_geometry_points = &second_geometry_sample->get_geometry_points(d_accessing_strain_rates);
		final_geometry_points = &first_geometry_sample->get_geometry_points(d_accessing_strain_rates);
		reverse_reconstruct = false;
		time_increment = reconstruction_time - initial_time; // Time increment must be positive.
		interpolate_initial_to_final_position = 1.0 - interpolate_position; // Invert interpolate position.
	}
	else
	{
		// Reconstruct forward in time away from the geometry import time.
		initial_time = first_geometry_time;
		initial_geometry_points = &first_geometry_sample->get_geometry_points(d_accessing_strain_rates);
		final_geometry_points = &second_geometry_sample->get_geometry_points(d_accessing_strain_rates);
		reverse_reconstruct = true;
		time_increment = initial_time - reconstruction_time; // Time increment must be positive.
		interpolate_initial_to_final_position = interpolate_position;
	}

	const unsigned int num_geometry_points = initial_geometry_points->size();

	// We're not storing this sample in our time span so don't share our pool allocator.
	// Sample gets its own allocator which means it releases its memory when it's no longer needed.
	// This is important since otherwise memory usage would continually increase each time
	// a geometry sample outside the time windows (in the time range) was requested.
	PoolAllocator::non_null_ptr_type pool_allocator = PoolAllocator::create();

	// The interpolated geometry points.
	std::vector<GeometryPoint *> interpolated_geometry_points;
	interpolated_geometry_points.resize(num_geometry_points, NULL);

	// Only calculate rigid stage rotation if some points need to be rigidly rotated.
	boost::optional<GPlatesMaths::FiniteRotation> interpolate_rigid_stage_rotation;

	// Keep track of the stage rotations of resolved boundaries as we encounter them.
	// This is an optimisation since many points can be inside the same resolved boundary.
	plate_id_to_stage_rotation_map_type resolved_boundary_stage_rotation_map;

	for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
	{
		GeometryPoint *initial_geometry_point = (*initial_geometry_points)[geometry_point_index];

		// Ignore initial point if it's not active.
		if (initial_geometry_point == NULL)
		{
			continue;
		}

		const GPlatesMaths::PointOnSphere initial_point(initial_geometry_point->position);
		const TopologyPointLocation &initial_point_location = initial_geometry_point->location;

		GeometryPoint *interpolated_geometry_point = NULL;

		// Get the resolved network point location that the initial point lies within (if any).
		const boost::optional<TopologyPointLocation::network_location_type> network_point_location =
				initial_point_location.located_in_resolved_network();
		if (network_point_location)
		{
			const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_network = network_point_location->first;
			const ResolvedTriangulation::Network::point_location_type &point_location = network_point_location->second;

			// Deform the initial point by the interpolate time increment.
			boost::optional<
					std::pair<
							GPlatesMaths::PointOnSphere,
							ResolvedTriangulation::Network::point_location_type> > interpolated_point_result =
					resolved_network->get_triangulation_network().calculate_deformed_point(
							initial_point,
							time_increment,
							reverse_reconstruct,
							d_deformation_uses_natural_neighbour_interpolation,
							point_location);
			if (interpolated_point_result)
			{
				const GPlatesMaths::PointOnSphere &interpolated_point = interpolated_point_result->first;

				interpolated_geometry_point =
						pool_allocator->geometry_point_pool.construct(interpolated_point, initial_point_location);
			}
		}

		if (interpolated_geometry_point == NULL)
		{
			//
			// The initial geometry point is outside all networks so test if it's inside a resolved boundary.
			//

			// Get the resolved boundary point location that the initial point lies within (if any).
			const boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_boundary =
					initial_point_location.located_in_resolved_boundary();
			if (resolved_boundary)
			{
				const boost::optional<GPlatesModel::integer_plate_id_type> resolved_boundary_plate_id =
						resolved_boundary.get()->plate_id();
				if (resolved_boundary_plate_id)
				{
					// Rotate the initial point by the interpolate time increment using resolved boundary.
					const GPlatesMaths::FiniteRotation &interpolate_resolved_boundary_stage_rotation =
							get_or_create_stage_rotation(
									resolved_boundary_plate_id.get(),
									resolved_boundary.get()->get_reconstruction_tree_creator(),
									initial_time,        // initial_time
									reconstruction_time, // final_time
									resolved_boundary_stage_rotation_map);

					const GPlatesMaths::PointOnSphere interpolated_point =
							interpolate_resolved_boundary_stage_rotation * initial_point;

					interpolated_geometry_point =
							pool_allocator->geometry_point_pool.construct(interpolated_point, initial_point_location);
				}
			}
		}

		if (interpolated_geometry_point == NULL)
		{
			//
			// The initial geometry point is outside all networks and resolved boundaries so rigidly rotate it instead.
			//

			if (!interpolate_rigid_stage_rotation)
			{
				interpolate_rigid_stage_rotation = get_stage_rotation(
						d_reconstruction_plate_id,
						d_topology_reconstruct->get_reconstruction_tree_creator(),
						initial_time,      // initial_time
						reconstruction_time); // final_time
			}

			const GPlatesMaths::PointOnSphere interpolated_point =
					interpolate_rigid_stage_rotation.get() * initial_point;

			interpolated_geometry_point =
					pool_allocator->geometry_point_pool.construct(interpolated_point, initial_point_location);
		}

		// Interpolate the strain rates and (total) strains if they're being accessed.
		if (d_accessing_strain_rates ||
			d_accessing_strains)
		{
			// If we also have the final geometry point (ie, is active) then interpolate the strain rates and total strains,
			// otherwise use those from the initial geometry point.
			GeometryPoint *final_geometry_point = (*final_geometry_points)[geometry_point_index];

			if (final_geometry_point)
			{
				if (d_accessing_strain_rates)
				{
					if (initial_geometry_point->strain_rate &&
						final_geometry_point->strain_rate)
					{
						const DeformationStrainRate interpolated_strain_rate =
								(1 - interpolate_initial_to_final_position) * *initial_geometry_point->strain_rate +
									interpolate_initial_to_final_position * *final_geometry_point->strain_rate;

						interpolated_geometry_point->strain_rate = pool_allocator->deformation_strain_rate_pool.construct(interpolated_strain_rate);
					}
					else if (initial_geometry_point->strain_rate)
					{
						// Copy into a new strain object since we can't share the same object (because using our own allocator).
						interpolated_geometry_point->strain_rate =
								pool_allocator->deformation_strain_rate_pool.construct(*initial_geometry_point->strain_rate);
					}
					else if (final_geometry_point->strain_rate)
					{
						// Copy into a new strain object since we can't share the same object (because using our own allocator).
						interpolated_geometry_point->strain_rate =
								pool_allocator->deformation_strain_rate_pool.construct(*final_geometry_point->strain_rate);
					}
					// ...else leave as NULL.
				}

				if (d_accessing_strains)
				{
					if (initial_geometry_point->strain &&
						final_geometry_point->strain)
					{
						const DeformationStrain interpolated_strain =
								interpolate_strain(
										*initial_geometry_point->strain,
										*final_geometry_point->strain,
										interpolate_initial_to_final_position);

						interpolated_geometry_point->strain = pool_allocator->deformation_strain_pool.construct(interpolated_strain);
					}
					else if (initial_geometry_point->strain)
					{
						// Copy into a new strain object since we can't share the same object (because using our own allocator).
						interpolated_geometry_point->strain =
								pool_allocator->deformation_strain_pool.construct(*initial_geometry_point->strain);
					}
					else if (final_geometry_point->strain)
					{
						// Copy into a new strain object since we can't share the same object (because using our own allocator).
						interpolated_geometry_point->strain =
								pool_allocator->deformation_strain_pool.construct(*final_geometry_point->strain);
					}
					// ...else leave as NULL.
				}
			}
			else
			{
				if (d_accessing_strain_rates)
				{
					if (initial_geometry_point->strain_rate)
					{
						// Copy into a new strain object since we can't share the same object (because using our own allocator).
						interpolated_geometry_point->strain_rate =
								pool_allocator->deformation_strain_rate_pool.construct(*initial_geometry_point->strain_rate);
					}
				}
				// ...else leave as NULL.

				if (d_accessing_strains)
				{
					if (initial_geometry_point->strain)
					{
						// Copy into a new strain object since we can't share the same object (because using our own allocator).
						interpolated_geometry_point->strain =
								pool_allocator->deformation_strain_pool.construct(*initial_geometry_point->strain);
					}
				}
				// ...else leave as NULL.
			}
		}

		interpolated_geometry_points[geometry_point_index] = interpolated_geometry_point;
	}

	return GeometrySample::create_swap(interpolated_geometry_points, pool_allocator);
}


GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::create_import_sample(
		interpolate_original_points_seq_type &interpolate_original_points,
		const GPlatesMaths::GeometryOnSphere &geometry,
		const PoolAllocator::non_null_ptr_type &pool_allocator,
		boost::optional<double> max_poly_segment_angular_extent_radians)
{
	// Tessellate if geometry is a polyline or polygon (and we've been requested to tessellate).
	if (max_poly_segment_angular_extent_radians)
	{
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline =
				GeometryUtils::get_polyline_on_sphere(geometry);
		if (polyline)
		{
			return create_tessellated_poly_import_sample(
					interpolate_original_points,
					polyline.get()->begin(),
					polyline.get()->end(),
					false/*is_polygon*/,
					pool_allocator,
					max_poly_segment_angular_extent_radians.get());
		}

		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon =
				GeometryUtils::get_polygon_on_sphere(geometry);
		if (polygon)
		{
			// Only tessellating the exterior ring for now.
			return create_tessellated_poly_import_sample(
					interpolate_original_points,
					polygon.get()->exterior_ring_begin(),
					polygon.get()->exterior_ring_end(),
					true/*is_polygon*/,
					pool_allocator,
					max_poly_segment_angular_extent_radians.get());
		}
	}

	//
	// Handle all geometry types (point, multi-point, polyline and polygon) without tessellation.
	//

	// Get the points of the geometry (and only exterior ring of polygons for now).
	std::vector<GPlatesMaths::PointOnSphere> points;
	GeometryUtils::get_geometry_exterior_points(geometry, points);

	// All points are original points (ie, no tessellation).
	const unsigned int num_points = points.size();
	interpolate_original_points.reserve(num_points);
	for (unsigned int n = 0; n < num_points; ++n)
	{
		interpolate_original_points.push_back(
				InterpolateOriginalPoints(0.0, n, n));
	}

	return GeometrySample::create(points, pool_allocator);
}


template <typename GreatCircleArcIter>
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::non_null_ptr_type
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::create_tessellated_poly_import_sample(
		interpolate_original_points_seq_type &interpolate_original_points,
		GreatCircleArcIter great_circle_arcs_begin,
		GreatCircleArcIter great_circle_arcs_end,
		bool is_polygon,
		const PoolAllocator::non_null_ptr_type &pool_allocator,
		const double &max_poly_segment_angular_extent_radians)
{
	std::vector<GPlatesMaths::PointOnSphere> tessellated_points;

	GreatCircleArcIter great_circle_arcs_iter = great_circle_arcs_begin;
	unsigned int gca_index = 0;
	while (true)
	{
		const GPlatesMaths::GreatCircleArc &gca = *great_circle_arcs_iter;

		// Tessellate the current great circle arc.
		const unsigned int initial_tessellated_points_size = tessellated_points.size();
		tessellate(tessellated_points, gca, max_poly_segment_angular_extent_radians);

		const unsigned int num_tessellated_gca_points = tessellated_points.size() - initial_tessellated_points_size;
		const double interpolate_increment = 1.0 / (num_tessellated_gca_points - 1);

		const unsigned int first_original_point_index = gca_index;

		++great_circle_arcs_iter;
		++gca_index;

		// The second original point index wraps around to zero for the last arc of a polygon.
		const unsigned int second_original_point_index =
				(great_circle_arcs_iter == great_circle_arcs_end)
				? (is_polygon ? 0 : gca_index)
				: gca_index;

		for (unsigned int t = 0; t < num_tessellated_gca_points; ++t)
		{
			interpolate_original_points.push_back(
					InterpolateOriginalPoints(
							t * interpolate_increment,
							first_original_point_index,
							second_original_point_index));
		}

		if (great_circle_arcs_iter == great_circle_arcs_end)
		{
			// Note: For polylines we don't remove the arc end point of the *last* arc.
			// But for polygons we remove it otherwise the start point of the *first* arc will duplicate it.
			if (is_polygon)
			{
				tessellated_points.pop_back();
				interpolate_original_points.pop_back();
			}

			break;
		}

		// Remove the tessellated arc's end point.
		// Otherwise the next arc's start point will duplicate it.
		//
		// Tessellating a great circle arc should always add at least two points.
		// So we should always be able to remove one point (the arc end point).
		tessellated_points.pop_back();
		interpolate_original_points.pop_back();
	}

	return GeometrySample::create(tessellated_points, pool_allocator);
}


void
GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::GeometrySample::calc_deformation_strain_rates()
{
	const unsigned int num_points = d_geometry_points.size();

	// Iterate over the network point locations and calculate instantaneous deformation information.
	for (unsigned int point_index = 0; point_index < num_points; ++point_index)
	{
		GeometryPoint *geometry_point = d_geometry_points[point_index];

		// Ignore geometry point if it's not active.
		if (geometry_point == NULL)
		{
			continue;
		}

		// If the current geometry point is inside a deforming region then copy the deformation strain rates
		// from the delaunay face it lies within (if we're not smoothing strain rates), otherwise
		// calculate the smoothed deformation at the current geometry point (this is all handled
		// internally by 'ResolvedTriangulation::Network::calculate_deformation()'.
		const boost::optional<TopologyPointLocation::network_location_type> network_point_location =
				geometry_point->location.located_in_resolved_network();
		if (network_point_location)
		{
			const GPlatesMaths::PointOnSphere point(geometry_point->position);

			const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_network = network_point_location->first;
			const ResolvedTriangulation::Network::point_location_type &point_location = network_point_location->second;

			boost::optional<ResolvedTriangulation::DeformationInfo> face_deformation_info =
					resolved_network->get_triangulation_network().calculate_deformation(point, point_location);
			if (face_deformation_info)
			{
				// Set the instantaneous strain rate.
				// The accumulated strain will subsequently depend on the instantaneous strain rate.
				geometry_point->strain_rate = d_pool_allocator->deformation_strain_rate_pool.construct(
						face_deformation_info->get_strain_rate());
			}
		}
	}

	d_have_initialised_strain_rates = true;
}
