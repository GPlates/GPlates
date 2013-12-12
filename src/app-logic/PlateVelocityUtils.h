/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2012, 2013 California Institute of Technology
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

#ifndef GPLATES_APP_LOGIC_PLATEVELOCITYUTILS_H
#define GPLATES_APP_LOGIC_PLATEVELOCITYUTILS_H

#include <map>
#include <utility>
#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "ReconstructionTreeCreator.h"

#include "file-io/FileInfo.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PointerTraits.h"

#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/Vector3D.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	namespace PlateVelocityUtils
	{
		/**
		 * Returns true if any features in @a feature_collection can be used
		 * as a domain for velocity calculations (currently this is
		 * feature type "gpml:MeshNode").
		 */
		bool
		detect_velocity_mesh_nodes(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);

		/**
		 * Returns true if the specified feature can be used as a domain for velocity calculations
		 * (currently this is feature type "gpml:MeshNode").
		 */
		bool
		detect_velocity_mesh_node(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);


		/**
		 * Creates a new feature collection containing a feature of type
		 * "gpml:VelocityField" for every feature in @a feature_collection_with_mesh_nodes
		 * that can be used as a domain for velocity calculations (currently this is
		 * feature type "gpml:MeshNode").
		 *
		 * Note: Returned feature collection might be empty if
		 * @a feature_collection_with_mesh_nodes contains no features that have a
		 * domain suitable for velocity calculations (use @a detect_velocity_mesh_nodes
		 * to avoid this).
		 *
		 * @throws PreconditionViolationError if @a feature_collection_with_mesh_nodes is invalid.
		 */
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
		create_velocity_field_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_with_mesh_nodes);


		/**
		 * Options to control how velocities are smoothed across plate boundaries.
		 */
		class VelocitySmoothingOptions
		{
		public:
			VelocitySmoothingOptions(
					const double &angular_half_extent_radians_,
					bool exclude_deforming_regions_) :
				angular_half_extent_radians(angular_half_extent_radians_),
				exclude_deforming_regions(exclude_deforming_regions_)
			{  }

			double angular_half_extent_radians;
			bool exclude_deforming_regions;
		};

		/**
		 * Solves velocities for the specified velocity surfaces:
		 * - reconstructed static polygons,
		 * - resolved topological boundaries,
		 * - resolved topological networks.
		 *
		 * Any, or all, of the above can be specified.
		 *
		 * The precedence is topological networks then topological boundaries then static polygons.
		 * So if a point is in a topological network and a topological boundary then the velocity
		 * in the topological network is calculated.
		 *
		 * The positions at which velocities are calculated is determined by the velocity domain
		 * geometries in @a velocity_domains. In other words their *reconstructed* positions are used.
		 * To avoid using reconstructed positions, assign plate id zero to the velocity domain features.
		 * Note that plate id zero can still give a non-zero rotation if the anchor plate id is non-zero.
		 *
		 * The position at which velocities are calculated is then tested against the reconstructed
		 * static polygons and resolved topological plates/networks and the velocities then depend
		 * on these surfaces.
		 *
		 * If @a velocity_smoothing_options is specified then it provides the angular distance (radians)
		 * over which velocities are smoothed across a plate/network boundary and whether smoothing
		 * should occur for points within a deforming region (including network rigid blocks).
		 * If any points of the reconstructed velocity domain lie within this distance from a
		 * boundary then their velocity is interpolated between the domain point's calculated velocity
		 * and the average velocity (at the nearest boundary point) using the distance-to-boundary
		 * for interpolation. The average velocity at the boundary point is the average of the
		 * velocities a very small (epsilon) distance on either side of the boundary.
		 * The smoothing occurs over boundaries of topological boundaries/networks and static polygons.
		 *
		 * NOTE: Originally the geometries were expected to be multi-point geometries but now any
		 * geometry type can be used (the points in the geometry are treated as a collection of points).
		 *
		 * The calculated velocities are stored in @a MultiPointVectorField instances (one per
		 * input velocity domain geometry) and appended to @a multi_point_velocity_fields.
		 *
		 * Note that only the @a ReconstructedFeatureGeometry objects, in
		 * @a velocity_surface_reconstructed_static_polygons, containing polygon geometry are
		 * used (other geometry types are ignored).
		 *
		 * Note that all @a ReconstructionGeometry derived objects (domains and surfaces) have a
		 * reconstruction time of @a reconstruction_time so it is simply provided to avoid having
		 * to retrieve it from any of those @a ReconstructionGeometry objects.
		 */
		void
		solve_velocities_on_surfaces(
				std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_velocity_fields,
				const double &reconstruction_time,
				const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &velocity_domains,
				// NOTE: Not specifying default arguments because that forces definition of the recon geom classes
				// (due to destructor of std::vector requiring non_null_ptr_type destructor requiring recon geom definition)
				// and we are avoiding that due to a cyclic header dependency with "ResolvedTopologicalNetwork.h"...
				const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &velocity_surface_reconstructed_static_polygons,
				const std::vector<resolved_topological_geometry_non_null_ptr_type> &velocity_surface_resolved_topological_boundaries,
				const std::vector<resolved_topological_network_non_null_ptr_type> &velocity_surface_resolved_topological_networks,
				const boost::optional<VelocitySmoothingOptions> &velocity_smoothing_options = boost::none);


		//////////////////////////////
		// Basic velocity utilities //
		//////////////////////////////


		/**
		 * Calculates velocity at @a point by using the rotation between the two specified rotations.
		 */
		GPlatesMaths::VectorColatitudeLongitude
		calc_velocity_colat_lon(
				const GPlatesMaths::PointOnSphere &point,
				const GPlatesMaths::FiniteRotation &finite_rotation1,
				const GPlatesMaths::FiniteRotation &finite_rotation2);

		/**
		 * Calculates velocity at @a point by using the rotation between two
		 * nearby reconstruction times represented by @a reconstruction_tree1
		 * and @a reconstruction_tree2 and using @a reconstruction_plate_id
		 * to lookup the two rotations.
		 */
		GPlatesMaths::VectorColatitudeLongitude
		calc_velocity_colat_lon(
				const GPlatesMaths::PointOnSphere &point,
				const ReconstructionTree &reconstruction_tree1,
				const ReconstructionTree &reconstruction_tree2,
				const GPlatesModel::integer_plate_id_type &reconstruction_plate_id);


		/**
		 * Calculates velocity at @a point by using the rotation between the two specified rotations.
		 */
		inline
		GPlatesMaths::Vector3D
		calc_velocity_vector(
				const GPlatesMaths::PointOnSphere &point,
				const GPlatesMaths::FiniteRotation &finite_rotation1,
				const GPlatesMaths::FiniteRotation &finite_rotation2)
		{
			return GPlatesMaths::calculate_velocity_vector(point, finite_rotation1, finite_rotation2);
		}

		/**
		 * Calculates velocity at @a point by using the rotation between two
		 * nearby reconstruction times represented by @a reconstruction_tree1
		 * and @a reconstruction_tree2 and using @a reconstruction_plate_id
		 * to lookup the two rotations.
		 */
		GPlatesMaths::Vector3D
		calc_velocity_vector(
				const GPlatesMaths::PointOnSphere &point,
				const ReconstructionTree &reconstruction_tree1,
				const ReconstructionTree &reconstruction_tree2,
				const GPlatesModel::integer_plate_id_type &reconstruction_plate_id);


		////////////////////////////////////////////////
		// Utilities relevant to topological networks //
		////////////////////////////////////////////////


		/**
		 * Calculates of velocities at arbitrary points within a topological network.
		 */
		class TopologicalNetworksVelocities
		{
		public:

			explicit
			TopologicalNetworksVelocities(
					const std::vector<resolved_topological_network_non_null_ptr_type> &networks);

			/**
			 * Returns the velocity at location @a point if it's inside any network's boundary,
			 * otherwise returns false.
			 *
			 * The returned velocity is interpolated if it's in a network's deforming region or
			 * the rigid plate velocity if it's in one of a network's interior rigid blocks.
			 */
			boost::optional<
					std::pair<
							// The resolved topological network (if point is inside it's deforming region)
							// or an interior rigid block of the network...
							const ReconstructionGeometry *,
							GPlatesMaths::Vector3D> >
			calculate_velocity(
					const GPlatesMaths::PointOnSphere &point) const;

		private:

			typedef std::vector<resolved_topological_network_non_null_ptr_type> network_seq_type;

			network_seq_type d_networks;
		};
	}
}

#endif // GPLATES_APP_LOGIC_PLATEVELOCITYUTILS_H
