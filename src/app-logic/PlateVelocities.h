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
#include <QString>

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/FileInfo.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureCollectionHandleUnloader.h"
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
				const GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
				GPlatesModel::ReconstructionTree &reconstruction_tree_1,
				GPlatesModel::ReconstructionTree &reconstruction_tree_2,
				const double &reconstruction_time_1,
				const double &reconstruction_time_2,
				GPlatesModel::integer_plate_id_type reconstruction_root,
				GPlatesFeatureVisitors::TopologyResolver &topology_resolver,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_point_layer,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_arrow_layer);
	}


	/**
	 * Class to handle velocity feature collection loading/unloading and calculations.
	 */
	class PlateVelocities : public
			FeatureCollectionFileState::Workflow
	{
	public:
		/**
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		PlateVelocities(
				ApplicationState &application_state,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_point_layer,
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_arrow_layer) :
			d_model(application_state.get_model_interface()),
			d_comp_mesh_point_layer(comp_mesh_point_layer),
			d_comp_mesh_arrow_layer(comp_mesh_arrow_layer)
		{
			register_workflow(application_state.get_feature_collection_file_state());
		}


		~PlateVelocities()
		{
			unregister_workflow();
		}


		virtual
		tag_type
		get_tag() const
		{
			return "PlateVelocities";
		}


		/**
		 * Priority of this @a FeatureCollectionFileState workflow.
		 */
		virtual
		priority_type
		get_priority() const
		{
			return PRIORITY_NORMAL;
		}


		/**
		 * Callback method notifying of new file (called from @a FeatureCollectionFileState).
		 *
		 * If the feature collection contains features that can be used for
		 * velocity calculations then this method returns true and a new
		 * feature collection is created internally that is used directly
		 * by the velocity solver.
		 */
		virtual
		bool
		add_file(
				FeatureCollectionFileState::file_iterator file_iter,
				const ClassifyFeatureCollection::classifications_type &classification,
				bool used_by_higher_priority_workflow);


		/**
		 * Callback method notifying about to remove file (called from @a FeatureCollectionFileState).
		 */
		virtual
		void
		remove_file(
				FeatureCollectionFileState::file_iterator file_iter);


		/**
		 * Callback method notifying file has changed (called from @a FeatureCollectionFileState).
		 *
		 * If the feature collection contains features that can be used for
		 * velocity calculations then this method returns true and a new
		 * feature collection is created internally that is used directly
		 * by the velocity solver.
		 */
		virtual
		bool
		changed_file(
				FeatureCollectionFileState::file_iterator file_iter,
				GPlatesFileIO::File &old_file,
				const ClassifyFeatureCollection::classifications_type &new_classification);


		virtual
		void
		set_file_active(
				FeatureCollectionFileState::file_iterator file_iter,
				bool active);


		/**
		 * Returns the number of velocity feature collections currently being calculated.
		 */
		unsigned int
		get_num_velocity_feature_collections() const
		{
			return d_velocity_field_feature_collection_infos.size();
		}


		/**
		 * Returns the feature collection at index @a index.
		 */
		const GPlatesModel::FeatureCollectionHandle::weak_ref
		get_velocity_feature_collection(
				unsigned int index) const
		{
			return d_velocity_field_feature_collection_infos[index]
					.d_velocity_field_feature_collection->get_feature_collection();
		}


		/**
		 * Returns the file info of feature collection at index @a index.
		 */
		const GPlatesFileIO::FileInfo &
		get_velocity_file_info(
				unsigned int index) const
		{
			return d_velocity_field_feature_collection_infos[index]
					.d_file_iterator->get_file_info();
		}


		/**
		 * Solves velocities for all loaded velocity feature collections.
		 *
		 * See @a PlateVelocityUtils::solve_velocities for details on
		 * how the results are generated and where they are stored.
		 */
		void
		solve_velocities(
				GPlatesModel::Reconstruction &reconstruction,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				GPlatesFeatureVisitors::TopologyResolver &topology_resolver);

	private:
		/**
		 * Used to associate a mesh node feature collection with a
		 * velocity field feature collection so that when the former is deleted
		 * we can stop calculating velocities for the latter.
		 */
		struct VelocityFieldFeatureCollectionInfo
		{
			VelocityFieldFeatureCollectionInfo(
					FeatureCollectionFileState::file_iterator file_iter,
					const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref &velocity_field_feature_collection) :
				d_file_iterator(file_iter),
				d_velocity_field_feature_collection(velocity_field_feature_collection),
				d_active(false)
			{  }

			FeatureCollectionFileState::file_iterator d_file_iterator;
			GPlatesModel::FeatureCollectionHandleUnloader::shared_ref d_velocity_field_feature_collection;
			bool d_active;
		};

		/**
		 * Typedef for a sequence of associations between mesh node velocity feature collections
		 * and corresponding velocity field feature collections.
		 */
		typedef std::vector<VelocityFieldFeatureCollectionInfo>
				velocity_field_feature_collection_info_seq_type;

		GPlatesModel::ModelInterface d_model;
		velocity_field_feature_collection_info_seq_type d_velocity_field_feature_collection_infos;

		/*
		 * FIXME: Presentation code should not be in here (this is app logic code).
		 * Remove any rendered geometry code to the presentation tier.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_comp_mesh_point_layer;
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type d_comp_mesh_arrow_layer;
	};
}

#endif // GPLATES_APP_LOGIC_PLATEVELOCITIES_H
