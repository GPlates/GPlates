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
#include "model/FeatureCollectionHandleUnloader.h"
#include "model/ModelInterface.h"
#include "model/types.h"

// FIXME: There should be no view operation code here (this is app logic code).
// Fix 'solve_velocities()' below to not create any rendered geometries (move to
// a higher source code tier).
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesModel
{
	class Reconstruction;
	class ReconstructionTree;
}

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
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		/**
		 * Creates a new feature collection containing a feature of type
		 * "gpml:VelocityField" for every feature in @a feature_collection_with_mesh_nodes
		 * that can be used as a domain for velocity calculations (currently this is
		 * feature type "gpml:MeshNode").
		 *
		 * Note: Returned feature collection may not be valid (eg, if
		 * @a feature_collection_with_mesh_nodes is not valid).
		 * Note: Returned feature collection might be empty if
		 * @a feature_collection_with_mesh_nodes contains no features that have a
		 * domain suitable for velocity calculations (use @a detect_velocity_mesh_nodes
		 * to avoid this).
		 */
		GPlatesModel::FeatureCollectionHandleUnloader::shared_ref
		create_velocity_field_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_with_mesh_nodes,
				GPlatesModel::ModelInterface &model);


		/**
		 * Solves velocities for features in @a velocity_field_feature_collection.
		 *
		 * The velocities are calculated at the domain points specified in each feature
		 * (currently by the GmlMultiPoint in the gpml:VelocityField feature).
		 *
		 * The generated velocities are stored in a new property named "velocities" of
		 * type GmlDataBlock.
		 *
		 * The reconstruction trees @a reconstruction_tree_1 and @a reconstruction_tree_2
		 * are used to calculate velocities and correspond to times
		 * @a reconstruction_time_1 and @a reconstruction_time_2.
		 * The larger the time delta the larger the calculated velocities will be.
		 *
		 * The feature collection @a velocity_field_feature_collection ideally should be
		 * created with @a create_velocity_field_feature_collection so that it contains
		 * features and property(s) of the correct type for the velocity solver.
		 *
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		void
		solve_velocities(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
				GPlatesModel::Reconstruction &reconstruction,
				GPlatesModel::ReconstructionTree &reconstruction_tree_1,
				GPlatesModel::ReconstructionTree &reconstruction_tree_2,
				const double &reconstruction_time_1,
				const double &reconstruction_time_2,
				GPlatesModel::integer_plate_id_type reconstruction_root,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_point_layer,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_arrow_layer);


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
				const GPlatesModel::ReconstructionTree &reconstruction_tree1,
				const GPlatesModel::ReconstructionTree &reconstruction_tree2,
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
				const GPlatesModel::ReconstructionTree &reconstruction_tree1,
				const GPlatesModel::ReconstructionTree &reconstruction_tree2,
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
