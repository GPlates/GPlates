/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "ReconstructionTreeCreator.h"

#include "file-io/FileInfo.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/CalculateVelocity.h"
#include "maths/Vector3D.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


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
		 * Solves velocities at velocity domain positions using plate IDs.
		 *
		 * The positions at which velocities are calculated is determined by the velocity domain
		 * geometries inside the features in @a velocity_domain_feature_collections.
		 * NOTE: Originally the geometries were expected to be multi-point geometries but now any
		 * geometry type can be used (the points in the geometry are treated as a collection of points).
		 *
		 * NOTE: The positions at which velocities are calculated moves according to each velocity
		 * domain feature's plate ID. This is unlike @a solve_velocities_on_surfaces where the
		 * positions are static on the globe (do not move/rotate).
		 *
		 * The calculated velocities are stored in @a MultiPointVectorField instances (one per
		 * input velocity domain geometry) and appended to @a multi_point_velocity_fields.
		 *
		 * @a reconstruction_tree_creator is used to create @a ReconstructionTree instances
		 * given reconstruction times. Internally it is used to retrieve a reconstruction tree
		 * at @a reconstruction_time and "@a reconstruction_time + 1" in order to calculate velocities.
		 */
		void
		solve_velocities_by_plate_id(
				std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_velocity_fields,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &velocity_domain_feature_collections);


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
		 * geometries inside the features in @a velocity_domain_feature_collections.
		 * NOTE: Originally the geometries were expected to be multi-point geometries but now any
		 * geometry type can be used (the points in the geometry are treated as a collection of points).
		 *
		 * NOTE: The positions at which velocities are calculated does *not* move according to each
		 * velocity domain feature's plate ID. This is unlike @a solve_velocities_by_plate_id where
		 * the positions move/rotate. Originally this function supported gpml:MeshNode velocity
		 * domain features that had a plate ID of zero (the plate ID is ignored here though).
		 *
		 * The calculated velocities are stored in @a MultiPointVectorField instances (one per
		 * input velocity domain geometry) and appended to @a multi_point_velocity_fields.
		 *
		 * Note that only the @a ReconstructedFeatureGeometry objects, in @a velocity_surface_reconstructed_static_polygons,
		 * that contain polygon geometry are used (other geometry types are ignored).
		 *
		 * @a reconstruction_tree_creator is used to create @a ReconstructionTree instances
		 * given reconstruction times. Internally it is used to retrieve a reconstruction tree
		 * at @a reconstruction_time and "@a reconstruction_time + 1" in order to calculate velocities.
		 */
		void
		solve_velocities_on_surfaces(
				std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_velocity_fields,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &velocity_domain_feature_collections,
				// NOTE: Not specifying default arguments because that forces definition of the recon geom classes
				// (due to destructor of std::vector requiring non_null_ptr_type destructor requiring recon geom definition)
				// and we are avoiding that due to a cyclic header dependency with "ResolvedTopologicalNetwork.h"...
				const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &velocity_surface_reconstructed_static_polygons,
				const std::vector<resolved_topological_boundary_non_null_ptr_type> &velocity_surface_resolved_topological_boundaries,
				const std::vector<resolved_topological_network_non_null_ptr_type> &velocity_surface_resolved_topological_networks);


		//////////////////////////////
		// Basic velocity utilities //
		//////////////////////////////


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
		 *
		 * Also can retrieve velocities at the nodes or points that make up the
		 * topological network.
		 *
		 * This class is basically a utility wrapper around a query made using
		 * a @a TopologyUtils function.
		 */
		class TopologicalNetworkVelocities
		{
		public:
			TopologicalNetworkVelocities() {  }

			/**
			 * The real velocity calculations are done by a @a TopologyUtils query that
			 * we are wrapping.
			 */
			explicit
			TopologicalNetworkVelocities(
					const TopologyUtils::resolved_network_for_interpolation_query_shared_ptr_type &
							network_velocities_query) :
				d_network_velocities_query(network_velocities_query)
			{  }

			/**
			 * Returns true if velocities have been set in the constructor.
			 */
			bool
			contains_velocities() const
			{
				return d_network_velocities_query;
			}

			/**
			 * Returns the interpolated velocity at location @a point if it's inside
			 * the network's triangulation otherwise returns false.
			 */
			boost::optional<GPlatesMaths::VectorColatitudeLongitude>
			interpolate_velocity(
					const GPlatesMaths::PointOnSphere &point);

			/**
			 * Returns the velocities at the network points.
			 *
			 * @post Both @a network_points and @a network_velocities are the same size and
			 *       are in the same order.
			 */
			void
			get_network_velocities(
					std::vector<GPlatesMaths::PointOnSphere> &network_points,
					std::vector<GPlatesMaths::VectorColatitudeLongitude> &network_velocities) const;

		private:
			TopologyUtils::resolved_network_for_interpolation_query_shared_ptr_type d_network_velocities_query;
		};


		/**
		 * The number of velocity components in velocities used for topological networks.
		 * This is two since velocity is stored as colatitude/longitude in topological networks.
		 */
		const unsigned int NUM_TOPOLOGICAL_NETWORK_VELOCITY_COMPONENTS = 2;


		/**
		 * Convert velocity colatitude/longitude to a vector of scalars.
		 *
		 * This converts velocity to a form that @a TopologyUtils can use for
		 * topological networks (because @a TopologyUtils works with arbitrary
		 * scalars and doesn't know what they represent - ie, doesn't know
		 * about velocities).
		 */
		inline
		std::vector<double>
		convert_velocity_colatitude_longitude_to_scalars(
				const GPlatesMaths::VectorColatitudeLongitude &velocity_colatitude_longitude)
		{
			std::vector<double> velocity_scalars;
			velocity_scalars.reserve(2);
			velocity_scalars.push_back(
					velocity_colatitude_longitude.get_vector_colatitude().dval());
			velocity_scalars.push_back(
					velocity_colatitude_longitude.get_vector_longitude().dval());
			return velocity_scalars;
		}


		/**
		 * Convert a vector of scalars to velocity colatitude/longitude.
		 *
		 * This converts velocity from a form that @a TopologyUtils used for
		 * topological networks (because @a TopologyUtils works with arbitrary
		 * scalars and doesn't know what they represent - ie, doesn't know
		 * about velocities).
		 */
		inline
		GPlatesMaths::VectorColatitudeLongitude
		convert_velocity_scalars_to_colatitude_longitude(
				const std::vector<double> &velocity_scalars)
		{
			// We're expecting two components to the velocity colat and lon.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					velocity_scalars.size() == 2,
					GPLATES_ASSERTION_SOURCE);

			return GPlatesMaths::VectorColatitudeLongitude(
					velocity_scalars[0], velocity_scalars[1]);
		}
	}
}

#endif // GPLATES_APP_LOGIC_PLATEVELOCITYUTILS_H
