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
 
#ifndef GPLATES_APP_LOGIC_ASSIGNPLATEIDS_H
#define GPLATES_APP_LOGIC_ASSIGNPLATEIDS_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "app-logic/TopologyUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/ModelInterface.h"
#include "model/Reconstruction.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileState;
	class Reconstruct;

	/**
	 * Assigns reconstruction plate ids to feature(s) using resolved topological
	 * boundaries (reconstructions of TopologicalClosedPlateBoundary features).
	 */
	class AssignPlateIds
	{
	public:
		//! Typedef for shared pointer to @a AssignPlateIds.
		typedef boost::shared_ptr<AssignPlateIds> non_null_ptr_type;


		/**
		 * How plate ids are assigned to features.
		 */
		enum AssignPlateIdMethodType
		{
			/**
			 * Assign, to each feature, a single plate id corresponding to the
			 * plate that the feature's geometry overlaps the most.
			 *
			 * If a feature contains multiple sub-geometries then they are treated
			 * as one composite geometry for our purposes here.
			 *
			 * If a feature does not overlap any plates - even a little bit -
			 * (unlikely if the plate polygon model covers the entire globe)
			 * then its reconstruction plate id property is removed.
			 */
			ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE,

			/**
			 * Assign, to each sub-geometry of each feature, a single plate id
			 * corresponding to the plate that the sub-geometry overlaps the most.
			 *
			 * This can create extra features, for example if a feature has two
			 * sub-geometries and one overlaps plate A the most and the other
			 * overlaps plate B the most then the original feature (with the two
			 * geometries) will get split into two features - one feature will contain
			 * the first geometry (and have plate id A) and the other feature will
			 * contain the second geometry (and have plate id B).
			 * The original feature will be used for the first feature and
			 * a cloned version of the original feature will be used for the second -
			 * the original feature will have its second geometry removed since it's
			 * now being moved over to the second (cloned) feature.
			 * When cloning, all non-geometry properties of the original feature are
			 * copied over.
			 *
			 * If a feature sub-geometry does not overlap any plates (unlikely if the
			 * plate polygon model covers the entire globe) then the
			 * reconstruction plate id property is removed from its containing feature.
			 * Using the above example, if one of the two geometries overlaps no plates
			 * and the other overlaps plate A then the first feature has no plate id
			 * while the second feature has plate id A.
			 */
			ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE,


			/**
			 * Partition all geometries of each feature into the plates
			 * containing them (intersecting them as needed and assigning the
			 * resulting partitioned geometry to the appropriate plates).
			 *
			 * This can create extra features, for example if a feature has only one
			 * sub-geometry but it overlaps plate A and plate B then it is partitioned
			 * into geometry that is fully contained by plate A and likewise for plate B.
			 * These two partitioned geometries will now be two features since they
			 * have different plate ids.
			 * The original feature will contain the first partitioned geometry (and
			 * have plate id A) while a new cloned feature will contain the second
			 * partitioned geometry (and have plate id B).
			 * When cloning, all non-geometry properties of the original feature are
			 * copied over.
			 *
			 * If there are any partitioned geometries left over that do not belong to
			 * any plate (unlikely if the plate polygon model covers the entire globe)
			 * then they are stored in the original feature and it will have no plate id.
			 * In this case all the partitioned geometry that is inside plates will be
			 * placed in new cloned features (one feature for each plate).
			 */
			PARTITION_FEATURE
		};


		/**
		 * Use the reconstruction, reconstruction time and anchor plate id from
		 * @a reconstruct to find our resolved topological boundaries to be used
		 * for cookie-cutting.
		 */
		static
		non_null_ptr_type
		create_at_current_reconstruction_time(
				AssignPlateIdMethodType assign_plate_id_method,
				const GPlatesModel::ModelInterface &model,
				GPlatesAppLogic::Reconstruct &reconstruct)
		{
			return non_null_ptr_type(new AssignPlateIds(
					assign_plate_id_method, model, reconstruct));
		}


		/**
		 * Create an internal @a Reconstruction using the active files from @a file_state
		 * and @a reconstruction_time and @a anchor_plate_id to create a new set of
		 * resolved topological boundaries to be used for cookie-cutting.
		 */
		static
		non_null_ptr_type
		create_at_new_reconstruction_time(
				AssignPlateIdMethodType assign_plate_id_method,
				const GPlatesModel::ModelInterface &model,
				FeatureCollectionFileState &file_state,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id)
		{
			return non_null_ptr_type(new AssignPlateIds(
					assign_plate_id_method, model, file_state,
					reconstruction_time, anchor_plate_id));
		}


		/**
		 * Finds features in @a feature_collection that have no 'gpml:reconstructionPlateId
		 * and are not reconstruction features (used for rotations).
		 *
		 * Returns results in @a features_needing_plate_id.
		 * Returns false if none are found.
		 */
		static
		bool
		find_reconstructable_features_without_reconstruction_plate_id(
				std::vector<GPlatesModel::FeatureHandle::weak_ref> &features_needing_plate_id,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Assign reconstruction plate ids to all features in the feature collection.
		 *
		 * Returns true if any plate ids were assigned.
		 *
		 * NOTE: This will do nothing if no plate boundaries exist - that is, if no
		 * TopologicalClosedPlateBoundary features have been previously loaded and activated.
		 * Actually it will remove any 'gpml:reconstructionPlateId' properties.
		 */
		bool
		assign_reconstruction_plate_ids(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref);

		/**
		 * Assign reconstruction plate ids to all features in a list of features.
		 *
		 * Returns true if any plate ids were assigned.
		 *
		 * NOTE: All features in @a feature_refs should be contained by
		 * @a feature_collection_ref.
		 *
		 * NOTE: This will do nothing if no plate boundaries exist - that is, if no
		 * TopologicalClosedPlateBoundary features have been previously loaded and activated.
		 * Actually it will remove any 'gpml:reconstructionPlateId' properties.
		 */
		bool
		assign_reconstruction_plate_ids(
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &feature_refs,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref);

		/**
		 * Assign a reconstruction plate id to a feature.
		 * Returns true if reconstruction plate id was assigned to the feature.
		 *
		 * Returns true if any plate ids were assigned.
		 *
		 * NOTE: @a feature_ref should be contained by @a feature_collection_ref.
		 *
		 * NOTE: This will do nothing if no plate boundaries exist - that is, if no
		 * TopologicalClosedPlateBoundary features have been previously loaded and activated.
		 * Actually it will remove any 'gpml:reconstructionPlateId' properties.
		 */
		bool
		assign_reconstruction_plate_id(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref);

	private:
		/**
		 * The method used to assign plate ids to features.
		 */
		AssignPlateIdMethodType d_assign_plate_id_method;

		/**
		 * Used to create new features.
		 */
		GPlatesModel::ModelInterface d_model;

		/**
		 * The reconstruction time.
		 */
		double d_reconstruction_time;

		/**
		 * Keep a @a Reconstruction around in case even if we're just linking to
		 * one from @a Reconstruct - if we're creating our own however then we need
		 * to keep the @a ResolvedTopologicalGeometry objects, contained within,
		 * alive for the cookie-cutting queries.
		 */
		GPlatesModel::Reconstruction::non_null_ptr_type d_reconstruction;

		/**
		 * Used to partition geometry using all resolved boundaries in a @a Reconstruction.
		 */
		GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type
				d_resolved_boundaries_geometry_partitioning_query;


		/**
		 * Use the reconstruction, reconstruction time and anchor plate id from
		 * @a reconstruct to find our resolved topological boundaries to be used
		 * for cookie-cutting.
		 */
		AssignPlateIds(
				AssignPlateIdMethodType assign_plate_id_method,
				const GPlatesModel::ModelInterface &model,
				GPlatesAppLogic::Reconstruct &reconstruct);

		/**
		 * Create an internal @a Reconstruction using the active files from @a file_state
		 * and @a reconstruction_time and @a anchor_plate_id to create a new set of
		 * resolved topological boundaries to be used for cookie-cutting.
		 */
		AssignPlateIds(
				AssignPlateIdMethodType assign_plate_id_method,
				const GPlatesModel::ModelInterface &model,
				FeatureCollectionFileState &file_state,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id);
	};
}

#endif // GPLATES_APP_LOGIC_ASSIGNPLATEIDS_H
