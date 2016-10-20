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


#include "GeometryDeformation.h"

#include "GeometryUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTriangulationNetwork.h"

#include "global/AbortException.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/CalculateVelocity.h"
#include "maths/Centroid.h"
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
	namespace GeometryDeformation
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
			 * Convert a GeometryOnSphere to a sequence of points.
			 */
			std::vector<GPlatesMaths::PointOnSphere>
			extract_geometry_points(
					const GPlatesMaths::GeometryOnSphere &geometry)
			{
				// Get the points of the present day geometry.
				std::vector<GPlatesMaths::PointOnSphere> geometry_points;
				GeometryUtils::get_geometry_exterior_points(geometry, geometry_points);

				return geometry_points;
			}


			/**
			 * Returns geometry points as a @a GeometryOnSphere of same type as the present day geometry.
			 */
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
			create_geometry_on_sphere(
					const std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					GPlatesMaths::GeometryType::Value geometry_type)
			{
				// Create a GeometryOnSphere from the geometry points.
				GPlatesUtils::GeometryConstruction::GeometryConstructionValidity geometry_validity;
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
						geometry_on_sphere = GPlatesUtils::create_geometry_on_sphere(
								geometry_type,
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


			/**
			 * Rigidly rotates the geometry points from present day to @a reconstruction_time.
			 */
			void
			rigid_reconstruct(
					std::vector<GPlatesMaths::PointOnSphere> &rotated_geometry_points,
					const std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					const double &reconstruction_time,
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					bool reverse_reconstruct = false)
			{
				GPlatesMaths::FiniteRotation rotation =
						reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time)
								->get_composed_absolute_rotation(reconstruction_plate_id).first;

				if (reverse_reconstruct)
				{
					rotation = get_reverse(rotation);
				}

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


			/**
			 * Get the rigid rotation from @a initial_time to @a final_time.
			 */
			GPlatesMaths::FiniteRotation
			get_rigid_stage_rotation(
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					const double &initial_time,
					const double &final_time)
			{
				const ReconstructionTree::non_null_ptr_to_const_type initial_reconstruction_tree =
						reconstruction_tree_creator.get_reconstruction_tree(initial_time);
				const ReconstructionTree::non_null_ptr_to_const_type final_reconstruction_tree =
						reconstruction_tree_creator.get_reconstruction_tree(final_time);

				const GPlatesMaths::FiniteRotation &present_day_to_initial_rotation =
						initial_reconstruction_tree->get_composed_absolute_rotation(
								reconstruction_plate_id).first;
				const GPlatesMaths::FiniteRotation &present_day_to_final_rotation =
						final_reconstruction_tree->get_composed_absolute_rotation(
								reconstruction_plate_id).first;

				return GPlatesMaths::compose(
						present_day_to_final_rotation,
						GPlatesMaths::get_reverse(present_day_to_initial_rotation));
			}


			/**
			 * Rigidly rotates the geometry points from @a initial_time to @a final_time.
			 */
			void
			rigid_reconstruct(
					std::vector<GPlatesMaths::PointOnSphere> &rotated_geometry_points,
					const std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
					const double &initial_time,
					const double &final_time,
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator)
			{
				const GPlatesMaths::FiniteRotation initial_to_final_rotation = get_rigid_stage_rotation(
						reconstruction_plate_id, reconstruction_tree_creator, initial_time, final_time);

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


			/**
			 * Interpolate between two sets of geometry points.
			 *
			 * This actually deforms, and rigidly rotates where necessary, points from the younger
			 * set of geometry points by the interpolate time increment. Hence we don't need the
			 * older set of geometry points.
			 */
			void
			interpolate_geometry_points(
					std::vector<GPlatesMaths::PointOnSphere> &interpolated_geometry_points,
					const std::vector<GPlatesMaths::PointOnSphere> &young_geometry_points,
					const network_point_location_opt_seq_type &young_network_point_locations,
					const double &young_time,
					const double &interpolate_time_increment,
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						young_geometry_points.size() == young_network_point_locations.size(),
						GPLATES_ASSERTION_SOURCE);

				const unsigned int num_points = young_geometry_points.size();

				interpolated_geometry_points.reserve(num_points);

				// Only calculate rigid stage rotation if some points need to be rigidly rotated.
				boost::optional<GPlatesMaths::FiniteRotation> interpolate_rigid_stage_rotation;

				for (unsigned int n = 0; n < num_points; ++n)
				{
					const GPlatesMaths::PointOnSphere &young_geometry_point = young_geometry_points[n];

					// Get the network point location that the current point lies within.
					const network_point_location_opt_type &network_point_opt_location = young_network_point_locations[n];
					if (network_point_opt_location)
					{
						const ResolvedTriangulation::Network::point_location_type &network_point_location = network_point_opt_location->first;
						const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_network = network_point_opt_location->second;

						// Deform the current point by the interpolate time increment.
						boost::optional<
								std::pair<
										GPlatesMaths::PointOnSphere,
										ResolvedTriangulation::Network::point_location_type> > interpolated_point_result =
								resolved_network->get_triangulation_network().calculate_deformed_point(
										young_geometry_point,
										interpolate_time_increment,
										// We're deforming backward in time from
										// 'young_time' to 'young_time + interpolate_time_increment'...
										false/*reverse_deform*/,
										network_point_location);
						if (interpolated_point_result)
						{
							const GPlatesMaths::PointOnSphere &interpolated_point = interpolated_point_result->first;
							interpolated_geometry_points.push_back(interpolated_point);

							continue;
						}
					}

					//
					// The current geometry point is outside the network so rigidly rotate it instead.
					//

					if (!interpolate_rigid_stage_rotation)
					{
						interpolate_rigid_stage_rotation = get_rigid_stage_rotation(
								reconstruction_plate_id,
								reconstruction_tree_creator,
								young_time,                               // initial_time
								young_time + interpolate_time_increment); // final_time
					}

					const GPlatesMaths::PointOnSphere interpolated_point =
							interpolate_rigid_stage_rotation.get() * young_geometry_point;

					interpolated_geometry_points.push_back(interpolated_point);
				}
			}


			/**
			 * Deforms @a current_geometry_points by a single time step to @a next_geometry_points.
			 *
			 * By default deformation is backward in time (from 'time' to 'time + time_increment').
			 *
			 * However if @a reverse_deform is true then deformation is forward in time
			 * (from 'time + time_increment' to 'time'), and so @a current_geometry_points should be
			 * associated with 'time + time_increment' (not 'time', as is the case when deforming backwards in time).
			 * This is because the resolved networks are deformed backwards from 'time' to 'time + time_increment'
			 * so that they can grab @a current_geometry_points and deform them forward in time to 'time'.
			 * This is what makes forward deformation mirror backward deformation so that it's exactly reversible).
			 *
			 * Note that @a time_increment should be positive, regardless of @a reverse_deform.
			 *
			 * Return false if none of the geometry points intersected deforming networks.
			 * In other words if the time step is a rigid rotation for all geometry points.
			 */
			bool
			deformation_time_step(
					const double &time,
					const double &time_increment,
					const std::vector<GPlatesMaths::PointOnSphere> &current_geometry_points,
					std::vector<GPlatesMaths::PointOnSphere> &next_geometry_points,
					// Make a copy of the list of networks so we can cull/remove networks just for this loop iteration...
					rtn_seq_type resolved_networks,
					GPlatesModel::integer_plate_id_type reconstruction_plate_id,
					const ReconstructionTreeCreator &reconstruction_tree_creator,
					bool reverse_deform = false,
					boost::optional<network_point_location_opt_seq_type &> current_network_point_locations = boost::none)
			{
				//
				// As an optimisation, remove those networks that the current geometry points do not intersect.
				//
				// However we don't do this when reverse deforming because, unlike regular forward deformation,
				// the current geometry points are associated with a different time than the resolved networks
				// (because the resolved networks are deformed backwards in time by the time increment so that
				// they can grab the current geometry points and deform them forward to the time of the resolved
				// networks - this is what makes forward deformation mirror backward deformation so that it's
				// exactly reversible). The most common path is backward deformation (ie, not reverse deformation)
				// since features are reconstructed/deformed backward from present day, so at least this
				// optimisation applies to the most common path.
				//
				if (!reverse_deform)
				{
					GPlatesMaths::BoundingSmallCircleBuilder current_geometry_points_small_circle_bounds_builder(
							GPlatesMaths::Centroid::calculate_points_centroid(
									current_geometry_points.begin(),
									current_geometry_points.end()));
					// Note that we don't need to worry about adding great circle arcs (if the geometry type is a
					// polyline or polygon) because we only test if the points intersect the resolved networks.
					// If interior arc sub-segment of a great circle arc (polyline/polygon edge) intersects
					// resolved network it doesn't matter (only the arc end points matter).
					std::vector<GPlatesMaths::PointOnSphere>::const_iterator curr_geometry_points_iter = current_geometry_points.begin();
					std::vector<GPlatesMaths::PointOnSphere>::const_iterator curr_geometry_points_end = current_geometry_points.end();
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

					// If none of the resolved networks intersect the geometry points at the current time then return early.
					if (resolved_networks.empty())
					{
						return false;
					}
				}

				const unsigned int num_geometry_points = current_geometry_points.size();

				// We've excluded those resolved networks that can't possibly intersect the current geometry points.
				// This doesn't mean the remaining networks will definitely intersect though - they might not.

				// An array to store deformed geometry points for the next time slot.
				// Starts out with all points being boost::none - and only deformed points will get filled.
				std::vector< boost::optional<GPlatesMaths::PointOnSphere> > deformed_geometry_points(num_geometry_points);

				if (current_network_point_locations)
				{
					// An array to store the network point locations that the current geometry points are in.
					// Starts out with all points being boost::none - and only points inside networks will get filled.
					current_network_point_locations->resize(num_geometry_points);
				}

				// Keep track of number of deformed geometry points for the current time.
				unsigned int num_deformed_geometry_points = 0;

				// Iterate over the current geometry points and attempt to deform them.
				for (unsigned int current_geometry_point_index = 0;
					current_geometry_point_index < num_geometry_points;
					++current_geometry_point_index)
				{
					const GPlatesMaths::PointOnSphere &current_geometry_point = current_geometry_points[current_geometry_point_index];

					// Iterate over the resolved networks for the current time.
					rtn_seq_type::const_iterator resolved_networks_iter = resolved_networks.begin();
					rtn_seq_type::const_iterator resolved_networks_end = resolved_networks.end();
					for ( ; resolved_networks_iter != resolved_networks_end; ++resolved_networks_iter)
					{
						const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_network = *resolved_networks_iter;

						boost::optional<
								std::pair<
										GPlatesMaths::PointOnSphere,
										ResolvedTriangulation::Network::point_location_type> > deformed_point_result =
								resolved_network->get_triangulation_network().calculate_deformed_point(
										current_geometry_point,
										time_increment,
										reverse_deform);
						if (!deformed_point_result)
						{
							// The current geometry point is outside the network so continue searching
							// the next resolved network.
							continue;
						}

						if (current_network_point_locations)
						{
							// As an optimisation, store the network location of the point so
							// we don't have to locate it a second time if we look up the strain.
							current_network_point_locations.get()[current_geometry_point_index] =
									std::make_pair(deformed_point_result->second, resolved_network);
						}

						// The current deformed geometry point.
						const GPlatesMaths::PointOnSphere &deformed_geometry_point = deformed_point_result->first;

						// Record the deformed point.
						deformed_geometry_points[current_geometry_point_index] = deformed_geometry_point;
						++num_deformed_geometry_points;

						// Finished searching resolved networks for the current geometry point.
						break;
					}
				}

				// If none of the resolved networks intersect the current geometry points then return early.
				if (num_deformed_geometry_points == 0)
				{
					return false;
				}

				// If we get here then at least one geometry point was deformed.

				// The geometry points for the next geometry sample.
				next_geometry_points.reserve(num_geometry_points);

				// If not all geometry points were deformed then rigidly rotate those that were not.
				if (num_deformed_geometry_points < num_geometry_points)
				{
					// Get the rigid finite rotation used for those geometry points that did not
					// intersect any resolved networks and hence must be rigidly rotated.
					const GPlatesMaths::FiniteRotation rigid_stage_rotation = reverse_deform
							? get_rigid_stage_rotation(
									reconstruction_plate_id,
									reconstruction_tree_creator,
									time + time_increment/*initial_time*/,
									time/*final_time*/)
							: get_rigid_stage_rotation(
									reconstruction_plate_id,
									reconstruction_tree_creator,
									time/*initial_time*/,
									time + time_increment/*final_time*/);

					for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
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
									rigid_stage_rotation * current_geometry_points[geometry_point_index]);
						}
					}
				}
				else // all geometry points were deformed...
				{
					// Just copy the deformed points into the next geometry sample.
					for (unsigned int geometry_point_index = 0; geometry_point_index < num_geometry_points; ++geometry_point_index)
					{
						GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
								deformed_geometry_points[geometry_point_index],
								GPLATES_ASSERTION_SOURCE);

						next_geometry_points.push_back(
								deformed_geometry_points[geometry_point_index].get());
					}
				}

				return true;
			}
		}
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GeometryDeformation::deform_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		GPlatesModel::integer_plate_id_type reconstruction_plate_id,
		const double &reconstruction_time,
		const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		bool reverse_deform)
{
	// If already at present day then just return the original geometry.
	if (GPlatesMaths::Real(reconstruction_time) == 0.0)
	{
		return geometry;
	}

	// The time range of both the resolved network topologies and the deformed geometry samples.
	const TimeSpanUtils::TimeRange time_range = resolved_network_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	const GPlatesMaths::GeometryType::Value geometry_type = GeometryUtils::get_geometry_type(*geometry);
	std::vector<GPlatesMaths::PointOnSphere> current_geometry_points = extract_geometry_points(*geometry);

	// If deformation happens prior to the reconstruction time then just rigidly reconstruct to/from present day.
	if (reconstruction_time <= time_range.get_end_time())
	{
		std::vector<GPlatesMaths::PointOnSphere> final_geometry_points;
		rigid_reconstruct(
				final_geometry_points,
				current_geometry_points,
				reconstruction_time,
				reconstruction_plate_id,
				reconstruction_tree_creator,
				reverse_deform);

		// Return as a GeometryOnSphere.
		return create_geometry_on_sphere(final_geometry_points, geometry_type);
	}



	if (reverse_deform)
	{
		// Rigidly reconstruct from reconstruction time to the beginning of the deformation time range if necessary.
		// This happens if the reconstruction time is prior to the beginning of deformation.
		if (GPlatesMaths::Real(reconstruction_time) > time_range.get_begin_time())
		{
			std::vector<GPlatesMaths::PointOnSphere> next_geometry_points;
			rigid_reconstruct(
					next_geometry_points,
					current_geometry_points,
					reconstruction_time/*initial_time*/,
					time_range.get_begin_time()/*final_time*/,
					reconstruction_plate_id,
					reconstruction_tree_creator);
			current_geometry_points.swap(next_geometry_points);
		}
	}
	else
	{
		// Rigidly reconstruct from present day to the end of the deformation time range if necessary.
		// This happens if deformation ends prior to present day.
		if (time_range.get_end_time() > GPlatesMaths::Real(0.0))
		{
			std::vector<GPlatesMaths::PointOnSphere> next_geometry_points;
			rigid_reconstruct(
					next_geometry_points,
					current_geometry_points,
					time_range.get_end_time(),
					reconstruction_plate_id,
					reconstruction_tree_creator);
			current_geometry_points.swap(next_geometry_points);
		}
	}

	// Determine the two nearest resolved network time slots bounding the reconstruction time.
	double interpolate_time_slots;
	const boost::optional< std::pair<unsigned int/*first_time_slot*/, unsigned int/*second_time_slot*/> >
			resolved_network_time_slots = time_range.get_bounding_time_slots(
					reconstruction_time, interpolate_time_slots);

	// First time slot is the begin time of deformation if reconstruction time prior to deformation.
	const int first_time_slot = resolved_network_time_slots ? resolved_network_time_slots->first : 0;

	// If reconstruction time is between time slots (versus exactly on a time slot) then
	// we'll need to interpolate in the initial (forward deformation) or final (backward deformation) time step.
	boost::optional<unsigned int> interpolation_time_slot;
	if (resolved_network_time_slots &&
		resolved_network_time_slots->first != resolved_network_time_slots->second) // not exactly on a time slot
	{
		interpolation_time_slot = resolved_network_time_slots->second;
	}

	// Iteration range over deformation time range.
	unsigned int start_time_slot;
	unsigned int end_time_slot;
	int time_slot_direction;
	if (reverse_deform)
	{
		// Iterate over the time range going *forwards* in time from the beginning of the
		// time range (least recent) towards the end (most recent).
		start_time_slot = first_time_slot + 1;
		end_time_slot = num_time_slots;
		time_slot_direction = 1;
	}
	else
	{
		// Iterate over the time range going *backwards* in time from the end of the
		// time range (most recent) towards the beginning (least recent).
		start_time_slot = num_time_slots - 1;
		end_time_slot = first_time_slot;
		time_slot_direction = -1;
	}

	// Iterate over the time slots either bacward or forward in time (depending on 'reverse_deform').
	for (unsigned int time_slot = start_time_slot; time_slot != end_time_slot; time_slot += time_slot_direction)
	{
		// Deformation/reconstruction is backward in time from 'time' to 'time + time_increment',
		// unless 'reverse_deform' is true (in which case deformation is forward in time from
		// 'time + time_increment' to 'time').
		const double time = time_range.get_time(time_slot);

		double time_increment = time_range.get_time_increment();
		// If interpolating current time slot then adjust time increment.
		if (time_slot == interpolation_time_slot)
		{
			// Regardless of whether we're deforming backward or forward in time,
			// the deformation time step is always relative to 'time'
			// (which is the second time slot being interpolated), so invert the interpolation factor
			// to be relative to the second time slot (instead of first time slot)...
			time_increment *= 1.0 - interpolate_time_slots;
		}

		// Get the resolved networks for the current time slot.
		// These are actually in the next time slot because they will deform forwards in time
		// from the current time to the next time.
		boost::optional<const rtn_seq_type &> resolved_networks =
				resolved_network_time_span->get_sample_in_time_slot(time_slot);

		std::vector<GPlatesMaths::PointOnSphere> next_geometry_points;

		// If there are no networks for the current time slot, or
		// none of the current geometry points intersect any networks,
		// then rigidly rotate to the next time slot.
		if (!resolved_networks ||
			resolved_networks->empty() ||
			!deformation_time_step(
					time,
					time_increment,
					current_geometry_points,
					next_geometry_points,
					resolved_networks.get(),
					reconstruction_plate_id,
					reconstruction_tree_creator,
					reverse_deform))
		{
			if (reverse_deform)
			{
				rigid_reconstruct(
						next_geometry_points,
						current_geometry_points,
						time + time_increment/*initial_time*/,
						time/*final_time*/,
						reconstruction_plate_id,
						reconstruction_tree_creator);
			}
			else
			{
				rigid_reconstruct(
						next_geometry_points,
						current_geometry_points,
						time/*initial_time*/,
						time + time_increment/*final_time*/,
						reconstruction_plate_id,
						reconstruction_tree_creator);
			}
		}

		// Set the current geometry points for the next time step.
		current_geometry_points.swap(next_geometry_points);
	}

	if (reverse_deform)
	{
		// Rigidly reconstruct from the end of the deformation time range to present day if necessary.
		// This happens if deformation ends prior to present day.
		if (time_range.get_end_time() > GPlatesMaths::Real(0.0))
		{
			std::vector<GPlatesMaths::PointOnSphere> next_geometry_points;
			rigid_reconstruct(
					next_geometry_points,
					current_geometry_points,
					time_range.get_end_time(),
					reconstruction_plate_id,
					reconstruction_tree_creator,
					true/*reverse_reconstruct*/);
			current_geometry_points.swap(next_geometry_points);
		}
	}
	else
	{
		// Rigidly reconstruct from the beginning of the deformation time range to reconstruction time if necessary.
		// This happens if the reconstruction time is prior to the beginning of deformation.
		if (GPlatesMaths::Real(reconstruction_time) > time_range.get_begin_time())
		{
			std::vector<GPlatesMaths::PointOnSphere> next_geometry_points;
			rigid_reconstruct(
					next_geometry_points,
					current_geometry_points,
					time_range.get_begin_time()/*initial_time*/,
					reconstruction_time/*final_time*/,
					reconstruction_plate_id,
					reconstruction_tree_creator);
			current_geometry_points.swap(next_geometry_points);
		}
	}

	return create_geometry_on_sphere(current_geometry_points, geometry_type);
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometryTimeSpan(
		const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &feature_present_day_geometry,
		GPlatesModel::integer_plate_id_type feature_reconstruction_plate_id) :
	d_reconstruction_plate_id(feature_reconstruction_plate_id),
	d_geometry_type(GeometryUtils::get_geometry_type(*feature_present_day_geometry)),
	d_time_window_span(
			time_window_span_type::create(
					resolved_network_time_span->get_time_range(),
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
					// The present day geometry points...
					GeometrySample(extract_geometry_points(*feature_present_day_geometry)))),
	d_reconstruction_tree_creator(reconstruction_tree_creator),
	d_resolved_network_time_span(resolved_network_time_span),
	d_have_initialised_deformation_total_strains(false)
{
	initialise_time_windows();
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::initialise_time_windows()
{
	//PROFILE_FUNC();

	// The time range of both the resolved network topologies and the deformed geometry samples.
	const TimeSpanUtils::TimeRange time_range = d_time_window_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Set geometry sample in the end time slot (closest to present day).
	// We don't actually need to do this because if the end time slot is empty then the time span will
	// generate a geometry sample by rigidly rotating the present day geometry sample.
	// Note that the end time slot is normally empty because we deform from the end time slot to the
	// time slot just prior to it and store a geometry sample there (and so on backwards in time).
	// However we will fill in the end time slot anyway so that interpolations between the end time slot
	// and the deformed slot just prior to it will actually be interpolations and not rigid rotations.
	d_time_window_span->set_sample_in_time_slot(
			d_time_window_span->get_or_create_sample(time_range.get_end_time()),
			num_time_slots - 1);

	// Iterate over the time range going *backwards* in time from the end of the
	// time range (most recent) to the beginning (least recent).
	for (/*signed*/ int time_slot = num_time_slots - 1; time_slot > 0; --time_slot)
	{
		// Get the resolved networks for the current time slot.
		boost::optional<const rtn_seq_type &> resolved_networks =
				d_resolved_network_time_span->get_sample_in_time_slot(time_slot);

		// If there are no networks for the current time slot then continue to the next time slot.
		if (!resolved_networks ||
			resolved_networks->empty())
		{
			// Geometry will not be stored for the current time.
			continue;
		}

		const double current_time = time_range.get_time(time_slot);

		// NOTE: If the feature does not exist at the current time slot we still deform it.
		// This is an issue to do with storing feature geometry in present day coordinates.
		// We need to be able to change the feature's end time without having it change the position
		// of the feature's deformed geometry prior to the feature's end (disappearance) time.
#if 0
		if (GPlatesPropertyValues::GeoTimeInstant(current_time).is_strictly_earlier_than(feature_begin_time) ||
			GPlatesPropertyValues::GeoTimeInstant(current_time).is_strictly_later_than(feature_end_time))
		{
			// Finished with the current time sample.
			continue;
		}
#endif

		// Get the geometry points for the current time.
		// This performs rigid rotation from the closest younger (deformed) geometry sample if needed.
		GeometrySample current_geometry_sample = d_time_window_span->get_or_create_sample(current_time);
		const std::vector<GPlatesMaths::PointOnSphere> &current_geometry_points = current_geometry_sample.get_points();

		std::vector<GPlatesMaths::PointOnSphere> next_geometry_points;

		// An array to store the network point locations that the current geometry points are in.
		// Starts out with all points being boost::none - and only points inside networks will get filled.
		network_point_location_opt_seq_type current_network_point_locations;

		// If none of the current geometry points intersect any networks then continue to the next time slot.
		// Deformation is backward in time from 'current_time' to 'current_time + time_increment'.
		if (!deformation_time_step(
					current_time,
					time_range.get_time_increment(),
					current_geometry_points,
					next_geometry_points,
					resolved_networks.get(),
					d_reconstruction_plate_id,
					d_reconstruction_tree_creator,
					false/*reverse_deform*/, // Going *backwards* in time away from present day.
					current_network_point_locations))
		{
			// Geometry will not be stored for the current time.
			continue;
		}

		// Set the recorded network point locations on the current geometry sample.
		current_geometry_sample.set_network_point_locations(current_network_point_locations);
		d_time_window_span->set_sample_in_time_slot(current_geometry_sample, time_slot);

		// Set the geometry sample for the next time slot.
		GeometrySample next_geometry_sample(next_geometry_points);
		d_time_window_span->set_sample_in_time_slot(next_geometry_sample, time_slot - 1);
	}
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::initialise_deformation_total_strains()
{
	d_have_initialised_deformation_total_strains = true;

	// The time range of the deformed geometry samples.
	const TimeSpanUtils::TimeRange &time_range = d_time_window_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// We need to convert time increment from My to seconds.
	static const double SECONDS_IN_A_MILLION_YEARS = 365.25 * 24 * 3600 * 1.0e6;
	const double time_increment_in_seconds = SECONDS_IN_A_MILLION_YEARS * time_range.get_time_increment();

	boost::optional<GeometrySample &> prev_geometry_sample = d_time_window_span->get_sample_in_time_slot(0);

	// Iterate over the time range going *forwards* in time from the beginning of the
	// time range (least recent) to the end (most recent).
	for (unsigned int time_slot = 1; time_slot < num_time_slots; ++time_slot)
	{
		// Get the geometry sample for the current time slot.
		boost::optional<GeometrySample &> curr_geometry_sample = d_time_window_span->get_sample_in_time_slot(time_slot);

		// If the current geometry sample is not in a deformation region at the current time slot
		// then skip it - we're only accumulating strain in deformation regions because
		// it doesn't accumulate in rigid regions - this also saves memory.
		// If we don't have a previous sample yet then we've got nothing to accumulate from.
		if (!curr_geometry_sample ||
			!prev_geometry_sample)
		{
			continue;
		}

		const std::vector<DeformationStrain> &prev_deformation_total_strains = prev_geometry_sample->get_deformation_total_strains();
		std::vector<DeformationStrain> &curr_deformation_total_strains = curr_geometry_sample->get_deformation_total_strains();
		const std::vector<DeformationStrain> &curr_deformation_strain_rates = curr_geometry_sample->get_deformation_strain_rates();

		// The number of points in each geometry sample should be the same.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					prev_deformation_total_strains.size() == curr_deformation_total_strains.size(),
					GPLATES_ASSERTION_SOURCE);

		// Iterate over the previous and current geometry sample points.
		const unsigned int num_points = prev_deformation_total_strains.size();
		for (unsigned int point_index = 0; point_index < num_points; ++point_index)
		{
			// Compute new strain for the current geometry sample using the strains at the
			// previous sample and the strain rates at the current sample.
			curr_deformation_total_strains[point_index] = prev_deformation_total_strains[point_index] +
					time_increment_in_seconds * curr_deformation_strain_rates[point_index];
		}

		prev_geometry_sample = curr_geometry_sample.get();
	}

	// Transfer the final accumulated values to the present-day sample.
	//
	// This ensures reconstructions between the end of the time range and present-day will
	// have the final accumulated values (because they will get carried over from the present-day
	// sample when it is rigidly rotated to the reconstruction time).
	if (prev_geometry_sample)
	{
		// There is no deformation during rigid time spans so the *instantaneous* deformation is zero.
		// But the *accumulated* deformation is propagated across gaps between time windows.

		const std::vector<DeformationStrain> &src_deformation_total_strains = prev_geometry_sample->get_deformation_total_strains();
		std::vector<DeformationStrain> &dst_deformation_total_strains =
				d_time_window_span->get_present_day_sample().get_deformation_total_strains();

		const unsigned int num_points = src_deformation_total_strains.size();
		for (unsigned int n = 0; n < num_points; ++n)
		{
			// Propagate *accumulated* deformations from closest younger geometry sample.
			dst_deformation_total_strains[n] = src_deformation_total_strains[n];
		}
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_geometry(
		const double &reconstruction_time,
		boost::optional<std::vector<DeformationStrain> &> deformation_strain_rates,
		boost::optional<std::vector<DeformationStrain> &> deformation_total_strains)
{
	// Get the deformed (or rigidly rotated) geometry points.
	std::vector<GPlatesMaths::PointOnSphere> geometry_points;
	get_geometry_sample_data(
			geometry_points,
			reconstruction_time,
			deformation_strain_rates,
			deformation_total_strains);

	// Return as a GeometryOnSphere.
	return create_geometry_on_sphere(geometry_points, d_geometry_type);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_geometry_points(
		std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		const double &reconstruction_time,
		boost::optional<std::vector<DeformationStrain> &> deformation_strain_rates,
		boost::optional<std::vector<DeformationStrain> &> deformation_total_strains)
{
	get_geometry_sample_data(
			geometry_points,
			reconstruction_time,
			deformation_strain_rates,
			deformation_total_strains);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_velocities(
		std::vector<GPlatesMaths::PointOnSphere> &domain_points,
		std::vector<GPlatesMaths::Vector3D> &velocities,
		std::vector< boost::optional<const ReconstructionGeometry *> > &surfaces,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	// Determine the two nearest resolved network time slots bounding the reconstruction time.
	double interpolate_time_slots;
	const boost::optional< std::pair<unsigned int/*first_time_slot*/, unsigned int/*second_time_slot*/> >
			resolved_network_time_slots = d_resolved_network_time_span->get_time_range()
					.get_bounding_time_slots(reconstruction_time, interpolate_time_slots);
	// If outside the time range then no interpolation between two time slot velocities is necessary.
	if (!resolved_network_time_slots)
	{
		// Get the geometry (domain) points.
		get_geometry_sample_data(domain_points, reconstruction_time);

		calc_velocities(
				domain_points,
				velocities,
				surfaces,
				boost::none/*resolved_networks_query*/,
				reconstruction_time,
				velocity_delta_time,
				velocity_delta_time_type);

		return;
	}

	// See if the reconstruction time coincides with a resolved networks time slot.
	// This is another case where no interpolation between two time slot velocities is necessary.
	if (resolved_network_time_slots->first == resolved_network_time_slots->second/*both time slots equal*/)
	{
		// Get the geometry (domain) points in the time slot.
		get_geometry_sample_data(domain_points, reconstruction_time);

		// Get the resolved topological networks (if any) in the time slot.
		boost::optional<PlateVelocityUtils::TopologicalNetworksVelocities> resolved_networks_query;
		boost::optional<const rtn_seq_type &> resolved_networks =
				d_resolved_network_time_span->get_sample_in_time_slot(resolved_network_time_slots->first);
		if (resolved_networks)
		{
			resolved_networks_query = boost::in_place(resolved_networks.get());
		}

		calc_velocities(
				domain_points,
				velocities,
				surfaces,
				resolved_networks_query,
				reconstruction_time,
				velocity_delta_time,
				velocity_delta_time_type);

		return;
	}

	//
	// Interpolate velocities between the two time slots.
	//

	const double first_time = d_resolved_network_time_span->get_time_range().get_time(resolved_network_time_slots->first);
	const double second_time = d_resolved_network_time_span->get_time_range().get_time(resolved_network_time_slots->second);

	// Get the geometry (domain) points at each time slot.
	std::vector<GPlatesMaths::PointOnSphere> first_domain_points;
	get_geometry_sample_data(first_domain_points, first_time);
	std::vector<GPlatesMaths::PointOnSphere> second_domain_points;
	network_point_location_opt_seq_type second_network_point_locations;
	get_geometry_sample_data(second_domain_points, second_time, boost::none, boost::none, second_network_point_locations);

	// Interpolate the points.
	interpolate_geometry_points(
			domain_points,
			second_domain_points/*young_geometry_points*/,
			second_network_point_locations/*young_network_point_locations*/,
			second_time/*young_time*/,
			// Deforming backwards in time so invert interpolation factor...
			(1.0 - interpolate_time_slots) * d_resolved_network_time_span->get_time_range().get_time_increment()/*interpolate_time_increment*/,
			d_reconstruction_plate_id,
			d_reconstruction_tree_creator);

	// Get the resolved topological networks (if any) in the each time slot.
	boost::optional<PlateVelocityUtils::TopologicalNetworksVelocities> first_resolved_networks_query;
	boost::optional<const rtn_seq_type &> first_resolved_networks =
			d_resolved_network_time_span->get_sample_in_time_slot(resolved_network_time_slots->first);
	if (first_resolved_networks)
	{
		first_resolved_networks_query = boost::in_place(first_resolved_networks.get());
	}
	boost::optional<PlateVelocityUtils::TopologicalNetworksVelocities> second_resolved_networks_query;
	boost::optional<const rtn_seq_type &> second_resolved_networks =
			d_resolved_network_time_span->get_sample_in_time_slot(resolved_network_time_slots->second);
	if (second_resolved_networks)
	{
		second_resolved_networks_query = boost::in_place(second_resolved_networks.get());
	}

	// Calculate velocities at each time slot.
	std::vector<GPlatesMaths::Vector3D> first_velocities;
	std::vector< boost::optional<const ReconstructionGeometry *> > first_surfaces;
	calc_velocities(
			first_domain_points,
			first_velocities,
			first_surfaces,
			first_resolved_networks_query,
			first_time,
			velocity_delta_time,
			velocity_delta_time_type);
	std::vector<GPlatesMaths::Vector3D> second_velocities;
	std::vector< boost::optional<const ReconstructionGeometry *> > second_surfaces;
	calc_velocities(
			second_domain_points,
			second_velocities,
			second_surfaces,
			second_resolved_networks_query,
			second_time,
			velocity_delta_time,
			velocity_delta_time_type);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			first_velocities.size() == second_velocities.size(),
			GPLATES_ASSERTION_SOURCE);

	const unsigned int num_points = domain_points.size();
	velocities.reserve(num_points);
	surfaces.reserve(num_points);

	// Interpolate the velocities.
	for (unsigned int n = 0; n < num_points; ++n)
	{
		velocities.push_back(
				(1 - interpolate_time_slots) * first_velocities[n] +
					interpolate_time_slots * second_velocities[n]);

		// If either first or second surface is a deforming network or rigid interior block then
		// use that. If both are then arbitrarily choose the first one. If neither then will be none.
		surfaces.push_back(first_surfaces[n] ? first_surfaces[n] : second_surfaces[n]);
	}
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::calc_velocities(
		const std::vector<GPlatesMaths::PointOnSphere> &domain_points,
		std::vector<GPlatesMaths::Vector3D> &velocities,
		std::vector< boost::optional<const ReconstructionGeometry *> > &surfaces,
		const boost::optional<PlateVelocityUtils::TopologicalNetworksVelocities> &resolved_networks_query,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	//
	// Calculate the velocities at the geometry (domain) points.
	//

	velocities.reserve(domain_points.size());
	surfaces.reserve(domain_points.size());

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
					network_velocity = resolved_networks_query->calculate_velocity(
							domain_point, velocity_delta_time, velocity_delta_time_type);
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

		// Calculate the velocity.
		const GPlatesMaths::Vector3D velocity_vector =
				PlateVelocityUtils::calculate_velocity_vector(
						domain_point,
						d_reconstruction_plate_id,
						d_reconstruction_tree_creator,
						reconstruction_time,
						velocity_delta_time,
						velocity_delta_time_type);

		// Add the velocity - there was no surface (ie, resolved network) intersection though.
		velocities.push_back(velocity_vector);
		surfaces.push_back(boost::none/*surface*/);
	}
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::get_geometry_sample_data(
		std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		const double &reconstruction_time,
		boost::optional<std::vector<DeformationStrain> &> deformation_strain_rates,
		boost::optional<std::vector<DeformationStrain> &> deformation_total_strains,
		boost::optional<network_point_location_opt_seq_type &> network_point_locations)
{
	// If deformation accumulated/total strains has been requested then first generate the information
	// if it hasn't already been generated.
	if (deformation_total_strains)
	{
		if (!d_have_initialised_deformation_total_strains)
		{
			initialise_deformation_total_strains();
		}
	}

	// Look up the geometry sample in the time window span.
	// This performs rigid rotation from the closest younger (deformed) geometry sample if needed.
	const GeometrySample geometry_sample = d_time_window_span->get_or_create_sample(reconstruction_time);

	// Copy the geometry sample points to the caller's array.
	geometry_points = geometry_sample.get_points();

	// Also copy the per-point deformation information if requested.
	if (deformation_strain_rates)
	{
		deformation_strain_rates.get() = geometry_sample.get_deformation_strain_rates();
	}
	if (deformation_total_strains)
	{
		deformation_total_strains.get() = geometry_sample.get_deformation_total_strains();
	}

	if (network_point_locations)
	{
		network_point_locations.get() = geometry_sample.get_network_point_locations();
	}
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::create_rigid_geometry_sample(
		const double &reconstruction_time,
		const double &closest_younger_sample_time,
		const GeometrySample &closest_younger_sample)
{
	// Rigidly reconstruct the sample points.
	std::vector<GPlatesMaths::PointOnSphere> geometry_points;
	rigid_reconstruct(
			geometry_points,
			closest_younger_sample.get_points(),
			closest_younger_sample_time/*initial_time*/,
			reconstruction_time/*final_time*/,
			d_reconstruction_plate_id,
			d_reconstruction_tree_creator);

	GeometrySample geometry_sample(geometry_points);

	// Also copy the per-point deformation information.
	// There is no deformation during rigid time spans so the *instantaneous* deformation is zero.
	// But the *accumulated* deformation is propagated across gaps between time windows.

	const std::vector<DeformationStrain> &src_deformation_total_strains = closest_younger_sample.get_deformation_total_strains();
	std::vector<DeformationStrain> &dst_deformation_total_strains = geometry_sample.get_deformation_total_strains();

	const unsigned int num_points = src_deformation_total_strains.size();
	for (unsigned int n = 0; n < num_points; ++n)
	{
		// Propagate *accumulated* deformations from closest younger geometry sample.
		dst_deformation_total_strains[n] = src_deformation_total_strains[n];
	}

	return geometry_sample;
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::interpolate_geometry_sample(
		const double &interpolate_position,
		const double &first_geometry_time,
		const double &second_geometry_time,
		const GeometrySample &first_geometry_sample,
		const GeometrySample &second_geometry_sample)
{
	const std::vector<GPlatesMaths::PointOnSphere> &second_geometry_points = second_geometry_sample.get_points();
	const network_point_location_opt_seq_type &second_network_point_locations = second_geometry_sample.get_network_point_locations();

	// Interpolate the points.
	std::vector<GPlatesMaths::PointOnSphere> interpolated_points;
	interpolate_geometry_points(
			interpolated_points,
			second_geometry_points/*young_geometry_points*/,
			second_network_point_locations/*young_network_point_locations*/,
			second_geometry_time/*young_time*/,
			// Deforming backwards in time so invert interpolation factor...
			(1.0 - interpolate_position) * (first_geometry_time - second_geometry_time)/*interpolate_time_increment*/,
			d_reconstruction_plate_id,
			d_reconstruction_tree_creator);

	const std::vector<DeformationStrain> &first_deformation_strain_rates = first_geometry_sample.get_deformation_strain_rates();
	const std::vector<DeformationStrain> &second_deformation_strain_rates = second_geometry_sample.get_deformation_strain_rates();
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			first_deformation_strain_rates.size() == second_deformation_strain_rates.size(),
			GPLATES_ASSERTION_SOURCE);

	const std::vector<DeformationStrain> &first_deformation_total_strains = first_geometry_sample.get_deformation_total_strains();
	const std::vector<DeformationStrain> &second_deformation_total_strains = second_geometry_sample.get_deformation_total_strains();
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			first_deformation_total_strains.size() == second_deformation_total_strains.size(),
			GPLATES_ASSERTION_SOURCE);

	const unsigned int num_points = first_deformation_strain_rates.size();

	// Interpolate the strain rates.
	std::vector<DeformationStrain> interpolated_deformation_strain_rates;
	std::vector<DeformationStrain> interpolated_deformation_total_strains;
	interpolated_deformation_strain_rates.reserve(num_points);
	interpolated_deformation_total_strains.reserve(num_points);
	for (unsigned int n = 0; n < num_points; ++n)
	{
		// Interpolate the deformation strain rates and total strains.
		interpolated_deformation_strain_rates.push_back(
				(1 - interpolate_position) * first_deformation_strain_rates[n] +
					interpolate_position * second_deformation_strain_rates[n]);
		interpolated_deformation_total_strains.push_back(
				(1 - interpolate_position) * first_deformation_total_strains[n] +
					interpolate_position * second_deformation_total_strains[n]);
	}

	return GeometrySample(
			interpolated_points,
			interpolated_deformation_strain_rates,
			interpolated_deformation_total_strains,
			// Use the network point locations of younger sample
			// (it's what is used to deform from younger to older time)...
			second_network_point_locations);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample::calc_deformation_strain_rates() const
{
	d_have_initialised_deformation_strain_rates = true;

	const unsigned int num_points = d_deformation_strain_rates.size();

	// Iterate over the network point locations and calculate instantaneous deformation information.
	for (unsigned int point_index = 0; point_index < num_points; ++point_index)
	{
		const network_point_location_opt_type &network_point_location_opt = d_network_point_locations[point_index];

		// If the current geometry point is inside a deforming region then copy the deformation strain rates
		// from the delaunay face it lies within (if we're not smoothing strain rates), otherwise
		// calculate the smoothed deformation at the current geometry point (this is all handled
		// internally by 'ResolvedTriangulation::Network::calculate_deformation()'.
		if (network_point_location_opt)
		{
			const GPlatesMaths::PointOnSphere &point = d_points[point_index];
			const ResolvedTriangulation::Network::point_location_type &network_point_location = network_point_location_opt->first;
			const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_network = network_point_location_opt->second;

			boost::optional<ResolvedTriangulation::DeformationInfo> face_deformation_info =
					resolved_network->get_triangulation_network().calculate_deformation(point, network_point_location);
			if (face_deformation_info)
			{
				// Set the instantaneous strain rate.
				// The accumulated strain will subsequently depend on the instantaneous strain rate.
				d_deformation_strain_rates[point_index] = face_deformation_info->get_strain_rate();
			}
		}
	}
}
