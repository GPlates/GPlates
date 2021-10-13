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
#include <boost/optional.hpp>

#include "TopologyUtils.h"

#include "file-io/FileInfo.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/CalculateVelocity.h"
#include "maths/Vector3D.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

// FIXME: There should be no view operation code here (this is app logic code).
// Fix 'solve_velocities()' below to not create any rendered geometries (move to
// a higher source code tier).
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class Reconstruction;
	class ReconstructionGeometryCollection;
	class ReconstructionTree;

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
		 * Solves velocities for all loaded velocity feature collections.
		 *
		 * See @a PlateVelocityUtils::solve_velocities for details on
		 * how the results are generated and where they are stored.
		 */
		const ReconstructionGeometryCollection::non_null_ptr_type
		solve_velocities(
				ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &mesh_point_feature_collections,
				const ReconstructionGeometryCollection &reconstructed_polygons);

		/**
		 * Solves velocities for all loaded velocity feature collections.
		 *
		 * See @a PlateVelocityUtils::solve_velocities for details on
		 * how the results are generated and where they are stored.
		 */
		const ReconstructionGeometryCollection::non_null_ptr_type
		solve_velocities_on_multiple_recon_geom_collections(
				ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &mesh_point_feature_collections,
				std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type> &reconstructed_polygons);

		/**
		 * Solves velocities for mesh-point features in @a mesh_point_feature_collection
		 * and stores the new MultiPointVectorField instances in
		 * @a velocity_fields_to_populate.
		 *
		 * The velocities are calculated at the domain points specified in each feature
		 * (currently by the GmlMultiPoint in the gpml:VelocityField feature).
		 *
		 * The reconstruction trees @a reconstruction_tree_1 and @a reconstruction_tree_2
		 * are used to calculate velocities and correspond to times
		 * @a reconstruction_time_1 and @a reconstruction_time_2.
		 * The larger the time delta the larger the calculated velocities will be.
		 *
		 * @a reconstruction_time_1 is "the" reconstruction time, and
		 * @a reconstruction_tree_1 is "the" reconstruction tree.
		 *
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		void
		solve_velocities_inner(
				ReconstructionGeometryCollection &velocity_fields_to_populate,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &mesh_point_feature_collection,
				const ReconstructionGeometryCollection &reconstructed_polygons,
				ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_1,
				ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_2,
				const double &reconstruction_time_1,
				const double &reconstruction_time_2,
				GPlatesModel::integer_plate_id_type reconstruction_root);


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
					const TopologyUtils::resolved_network_for_interpolation_query_type &
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
			TopologyUtils::resolved_network_for_interpolation_query_type d_network_velocities_query;
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
