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

// DEF TEST
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

#include "maths/CalculateVelocity.h"
#include "maths/Centroid.h"
#include "maths/MathsUtils.h"
#include "maths/SmallCircleBounds.h"
#include "maths/types.h"

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
		get_geometry_points(
				const GPlatesMaths::GeometryOnSphere &geometry)
		{
			// Get the points of the present day geometry.
			std::vector<GPlatesMaths::PointOnSphere> geometry_points;
			GeometryUtils::get_geometry_exterior_points(geometry, geometry_points);

			return geometry_points;
		}


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
							_3,
							// Boost bind stores a copy...
							reconstruction_tree_creator),
					// The present day geometry points...
					GeometrySample(get_geometry_points(*feature_present_day_geometry)))),
	d_have_initialised_forward_time_accumulated_deformation_information(false)
{
	initialise_time_windows(
			resolved_network_time_span,
			reconstruction_tree_creator);

	// We should have at least one geometry sample (containing the present day geometry).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_time_window_span->empty(),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::initialise_time_windows(
		const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span,
		const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	//PROFILE_FUNC();

	// The time range of both the resolved network topologies and the deformed geometry samples.
	const TimeSpanUtils::TimeRange time_range = d_time_window_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Iterate over the time range going *backwards* in time from the end of the
	// time range (most recent) to the beginning (least recent).
	for (/*signed*/ int time_slot = num_time_slots - 1; time_slot > 0; --time_slot)
	{
		const double current_time = time_range.get_time(time_slot);
		const double next_time = current_time + time_range.get_time_increment();

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

		// Get the resolved networks for the current time slot.
		boost::optional<const rtn_seq_type &> resolved_networks_opt =
				resolved_network_time_span->get_sample_in_time_slot(time_slot);

		// If there are no networks for the current time slot then continue to the next time slot.
		// Geometry will not be stored for the current time.
		if (!resolved_networks_opt ||
			resolved_networks_opt->empty())
		{
			// Finished with the current time sample.
			continue;
		}

		// Make a copy of the list of networks so we can cull/remove networks just for this loop iteration.
		rtn_seq_type resolved_networks = resolved_networks_opt.get();

		// Get the geometry points for the current time.
		// This performs rigid rotation from the closest younger (deformed) geometry sample if needed.
		const GeometrySample current_geometry_sample =
				d_time_window_span->get_or_create_sample(current_time);
		const std::vector<GPlatesMaths::PointOnSphere> &current_geometry_points =
				current_geometry_sample.get_points();

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
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator curr_geometry_points_iter =
				current_geometry_points.begin();
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator curr_geometry_points_end =
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
			// Finished with the current time sample.
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
			rtn_seq_type::const_iterator resolved_networks_iter = resolved_networks.begin();
			rtn_seq_type::const_iterator resolved_networks_end = resolved_networks.end();
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
			// Finished with the current time sample.
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

		GeometrySample next_geometry_sample(next_geometry_points);

		// Set the recorded delaunay faces on the current geometry sample.
		next_geometry_sample.set_delaunay_faces(current_delaunay_faces);

		// Set the geometry sample for the next time slot.
		d_time_window_span->set_sample_in_time_slot(
				next_geometry_sample,
				time_slot - 1);
	}
}


void
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::initialise_forward_time_accumulated_deformation_information()
{
	d_have_initialised_forward_time_accumulated_deformation_information = true;

	// The time range of the deformed geometry samples.
	const TimeSpanUtils::TimeRange &time_range = d_time_window_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	boost::optional<GeometrySample &> prev_geometry_sample = d_time_window_span->get_sample_in_time_slot(0);

	// Iterate over the time range going *forwards* in time from the beginning of the
	// time range (least recent) to the end (most recent).
	for (unsigned int time_slot = 1; time_slot < num_time_slots; ++time_slot)
	{
		// Get the geometry sample for the current time slot.
		boost::optional<GeometrySample &> curr_geometry_sample =
				d_time_window_span->get_sample_in_time_slot(time_slot);

		// If the current geometry sample is not in a deformation region at the current time slot
		// then skip it - we're only accumulating strain during in deformation regions because
		// it doesn't accumulate in rigid regions - this also saves memory.
		// If we don't have a previous sample yet then we've got nothing to accumulate from.
		if (!curr_geometry_sample ||
			!prev_geometry_sample)
		{
			continue;
		}

		// DEF_TEST

#if 0 // Avoid writing out intermediate file on trunk for now until sorted out on a source code branch...
		// string to hold filename
		std::string def_test_file_name = "DEF_TEST_t";
        // Compute a recon time value from begin and geom sample index 
        double t = time_range.get_time(time_slot);
		std::ostringstream t_oss;
		t_oss << t;
		def_test_file_name += t_oss.str();

        // Complete the file name
 		def_test_file_name += "Ma.xy";

		// Open the file 
		std::ofstream def_test_file;
		def_test_file.open( def_test_file_name.c_str() );

        // Write header data 
		def_test_file << ">HEADER LINES:\n";
		def_test_file << ">point index number \n";
		def_test_file << ">lon lat dilitation sph_dilitation\n";
		def_test_file << ">\n";
		// DEF_TEST
#endif

		const std::vector<DeformationInfo> &prev_deformation_infos = prev_geometry_sample->get_deformation_infos();
		std::vector<DeformationInfo> &curr_deformation_infos = curr_geometry_sample->get_deformation_infos();

		// The number of points in each geometry sample should be the same.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					prev_deformation_infos.size() == curr_deformation_infos.size(),
					GPLATES_ASSERTION_SOURCE);

		// Iterate over the previous and current geometry sample points.
		const unsigned int num_points = prev_deformation_infos.size();
		for (unsigned int point_index = 0; point_index < num_points; ++point_index)
		{
			const DeformationInfo &prev_deformation_info = prev_deformation_infos[point_index];
			DeformationInfo &curr_deformation_info = curr_deformation_infos[point_index];

			// Compute new strain for the current geometry sample using the strains at the
			// previous sample and the strain rates at the current sample.
			// NOTE: 3.15569e13 seconds in 1 My
			// The assumption being that the velocities 
			const double S22 = prev_deformation_info.S22 + curr_deformation_info.SR22 * 3.15569e13;
			const double S33 = prev_deformation_info.S33 + curr_deformation_info.SR33 * 3.15569e13;
			const double S23 = prev_deformation_info.S23 + curr_deformation_info.SR23 * 3.15569e13;

#if 0 // Avoid writing out intermediate file on trunk for now until sorted out on a source code branch...
			// DEF_TEST
			const double dilitation = curr_deformation_info.dilitation; 
			const double sph_dilitation = curr_deformation_info.sph_dilitation; 
#endif

			// Set the new strain at the current geometry sample.
			curr_deformation_info.S22 = S22;
			curr_deformation_info.S33 = S33;
			curr_deformation_info.S23 = S23;

  			// Compute the Dilitational and Second invariant strain for the current geometry sample.
			const double dilitation_strain = S22 + S33;

			// incompressable
			//const double second_invariant_strain = std::sqrt(S22 * S22 + S33 * S33 + 2.0 * S23 * S23);

			// compressable
			// const double second_invariant = std::sqrt( (SR22 * SR33) - (SR23 * SR23) );
			const double tmp1=( S22 * S33) - (S23 * S23);
			const double second_invariant_strain = copysign( std::sqrt( std::abs(tmp1) ),tmp1) ;

			// Principle strains.
			const double S_DIR = 0.5 * std::atan2(2.0 * S23, S33 - S22);
			const double S_variation = std::sqrt(S23*S23 + 0.25*(S33-S22)*(S33-S22));
			const double S1 = 0.5*(S22+S33) + S_variation;
			const double S2 = 0.5*(S22+S33) - S_variation;

			// Set the new values.
			curr_deformation_info.dilitation_strain = dilitation_strain;
			curr_deformation_info.second_invariant_strain = second_invariant_strain;
			curr_deformation_info.S1 = S1;
			curr_deformation_info.S2 = S2;
			curr_deformation_info.S_DIR = S_DIR;

		
#if 0 // Avoid writing out intermediate file on trunk for now until sorted out on a source code branch...
			// DEF_TEST
			// Get all the points into a vector ; will use the point index to get coordinates
			GPlatesMaths::PointOnSphere p = curr_geometry_sample.get_points()[point_index];
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point( p );
			double lat = llp.latitude();
			double lon = llp.longitude();

			def_test_file << ">point index: ";
			def_test_file << point_index;
			def_test_file << "\n";
			def_test_file << lon;
			def_test_file << " ";
			def_test_file << lat;
			def_test_file << " ";
			def_test_file << dilitation;
			def_test_file << " ";
			def_test_file << sph_dilitation;
			def_test_file << " ";
			def_test_file << "\n";
#endif
		}

#if 0 // Avoid writing out intermediate file on trunk for now until sorted out on a source code branch...
		// DEF_TEST
		// close file for this recon time 
		def_test_file.close();
#endif

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

		const std::vector<DeformationInfo> &src_deformation_infos = prev_geometry_sample->get_deformation_infos();
		std::vector<DeformationInfo> &dst_deformation_infos =
				d_time_window_span->get_present_day_sample().get_deformation_infos();

		const unsigned int num_points = src_deformation_infos.size();
		for (unsigned int n = 0; n < num_points; ++n)
		{
			// Propagate *accumulated* deformations from closest younger geometry sample.
			dst_deformation_infos[n].set_accumulated_values(src_deformation_infos[n]);
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
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const resolved_network_time_span_type::non_null_ptr_to_const_type &resolved_network_time_span)
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
	boost::optional<const rtn_seq_type &> resolved_networks =
			resolved_network_time_span->get_nearest_sample_at_time(reconstruction_time);
	if (resolved_networks)
	{
		resolved_networks_query = boost::in_place(resolved_networks.get());
	}

	const std::pair<double, double> time_range = VelocityDeltaTime::get_time_range(
			velocity_delta_time_type, reconstruction_time, velocity_delta_time);

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
						reconstruction_tree_creator,
						time_range.second/*young*/,
						time_range.first/*old*/,
						d_reconstruction_plate_id);

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

	// Look up the geometry sample in the time window span.
	// This performs rigid rotation from the closest younger (deformed) geometry sample if needed.
	const GeometrySample geometry_sample = d_time_window_span->get_or_create_sample(reconstruction_time);

	// Copy the geometry sample points to the caller's array.
	geometry_points = geometry_sample.get_points();

	// Also copy the per-point deformation information if it was requested.
	if (deformation_info_points)
	{
		deformation_info_points.get() = geometry_sample.get_deformation_infos();
	}
}


GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::GeometrySample
GPlatesAppLogic::GeometryDeformation::GeometryTimeSpan::create_rigid_geometry_sample(
		const double &reconstruction_time,
		const double &closest_younger_sample_time,
		const GeometrySample &closest_younger_sample,
		const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	// Rigidly reconstruct the sample points.
	std::vector<GPlatesMaths::PointOnSphere> geometry_points;
	rigid_reconstruct(
			geometry_points,
			closest_younger_sample.get_points(),
			reconstruction_tree_creator,
			closest_younger_sample_time/*initial_time*/,
			reconstruction_time/*final_time*/);

	GeometrySample geometry_sample(geometry_points);

	// Also copy the per-point deformation information.
	// There is no deformation during rigid time spans so the *instantaneous* deformation is zero.
	// But the *accumulated* deformation is propagated across gaps between time windows.

	const std::vector<DeformationInfo> &src_deformation_infos = closest_younger_sample.get_deformation_infos();
	std::vector<DeformationInfo> &dst_deformation_infos = geometry_sample.get_deformation_infos();

	const unsigned int num_points = src_deformation_infos.size();
	for (unsigned int n = 0; n < num_points; ++n)
	{
		// Propagate *accumulated* deformations from closest younger geometry sample.
		dst_deformation_infos[n].set_accumulated_values(src_deformation_infos[n]);
	}

	return geometry_sample;
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
					
			point_deformation_info.dilitation = face_deformation_info.dilitation;
			point_deformation_info.sph_dilitation = face_deformation_info.sph_dilitation;
			point_deformation_info.SR22 = face_deformation_info.SR22;
			point_deformation_info.SR33 = face_deformation_info.SR33;
			point_deformation_info.SR23 = face_deformation_info.SR23;
		}
	}
}
