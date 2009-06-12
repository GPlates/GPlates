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

#ifndef GPLATES_APP_LOGIC_PLATEVELOCITIES_H
#define GPLATES_APP_LOGIC_PLATEVELOCITIES_H

#include <vector>

#include "ReconstructHook.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/types.h"

// FIXME: There should be no view operation code here (this is app logic code).
// Fix 'solve_velocities()' below to not create any rendered geometries (move to
// a higher source code tier).
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesFeatureVisitors
{
	class TopologyResolver;
}

namespace GPlatesModel
{
	class ModelInterface;
	class ReconstructionTree;
}

namespace GPlatesAppLogic
{
	namespace PlateVelocities
	{
		/**
		 * Returns true if any features in @a feature_collection can be used
		 * as a domain for velocity calculations (currently this is
		 * feature type "gpml:MeshNode").
		 */
		bool
		detect_velocity_mesh_nodes(
				GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


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
		GPlatesModel::FeatureCollectionHandle::weak_ref
		create_velocity_field_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_with_mesh_nodes,
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
		 * FIXME: When TopologyResolver is divided into two parts (see comment inside
		 * GPlatesAppLogic::Reconstruct::create_reconstruction) remove it from argument list
		 * replacing it with a @a GPlatesModel::Reconstruction so that the plate boundaries
		 * can be queried from the reconstruction geometries associated with features of type
		 * TopologicalClosedPlateBoundary instead of using the TopologyResolver.
		 *
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		void
		solve_velocities(
				GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
				GPlatesModel::ReconstructionTree &reconstruction_tree_1,
				GPlatesModel::ReconstructionTree &reconstruction_tree_2,
				const double &reconstruction_time_1,
				const double &reconstruction_time_2,
				GPlatesModel::integer_plate_id_type reconstruction_root,
				GPlatesFeatureVisitors::TopologyResolver &topology_resolver,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_layer);
	}


	/**
	 * Hook to render reconstruction geometries after a reconstruction.
	 */
	class PlateVelocitiesHook :
			public GPlatesAppLogic::ReconstructHook
	{
	public:
		/**
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		PlateVelocitiesHook(
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_layer) :
			d_comp_mesh_layer(comp_mesh_layer)
		{  }


		/**
		 * Call this when a new feature collection of reconstructable features
		 * has been loaded by the user.
		 *
		 * If the feature collection contains features that can be used for
		 * velocity calculations then a new feature collection is created internally
		 * that is used directly by the velocity solver.
		 */
		void
		load_reconstructable_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				GPlatesModel::ModelInterface &model);


		/**
		 * Call this when a new feature collection of reconstruction features
		 * has been loaded by the user.
		 *
		 * This is used to determine finite rotations for the velocity calculations.
		 */
		void
		load_reconstruction_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		/**
		 * Call this when any type of feature collection has been unloaded by the user.
		 *
		 * If the feature collection is stored internally then it will be removed.
		 * Applies to calls to either @a load_reconstructable_feature_collection
		 * or @a load_reconstruction_feature_collection.
		 */
		void
		unload_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		/**
		 * Callback hook after a reconstruction is created.
		 */
		virtual
		void
		post_reconstruction_hook(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::Reconstruction &reconstruction,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				GPlatesFeatureVisitors::TopologyResolver &topology_resolver);

	private:
		typedef std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
				feature_collection_weak_ref_seq_type;

		feature_collection_weak_ref_seq_type d_velocity_field_feature_collections;
		feature_collection_weak_ref_seq_type d_reconstruction_feature_collections;

		/**
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_comp_mesh_layer;
	};
}

#endif // GPLATES_APP_LOGIC_PLATEVELOCITIES_H
