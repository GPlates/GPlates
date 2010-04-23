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

#include <bitset>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "GeometryCookieCutter.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/Reconstruction.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	class PartitionFeatureTask;

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
		 * The feature property types we can assign.
		 */
		enum FeaturePropertyType
		{
			//! Reconstruction plate id
			RECONSTRUCTION_PLATE_ID,
			//! Time of appearance and disappearance
			VALID_TIME,

			NUM_FEATURE_PROPERTY_TYPES // Must be the last enum.
		};

		/**
		 * A std::bitset for specifying which feature properties to assign.
		 */
		typedef std::bitset<NUM_FEATURE_PROPERTY_TYPES> feature_property_flags_type;

		//! Specifies only the reconstruction plate id property is assigned.
		static const feature_property_flags_type RECONSTRUCTION_PLATE_ID_PROPERTY_FLAG;


		/**
		 * Create an internal @a Reconstruction using @a partitioning_feature_collections,
		 * @a reconstruction_feature_collections, @a reconstruction_time and @a anchor_plate_id
		 * to create a new set of partitioning polygons to be used for cookie-cutting.
		 *
		 * @a partitioning_feature_collections can be a source of dynamic polygons or
		 * static polygons.
		 * That is they can contain TopologicalClosedPlateBoundary features or
		 * regular static polygon features.
		 *
		 * @a reconstruction_feature_collections contains rotations required to reconstruct
		 * the partitioning polygon features and to reverse reconstruct any features
		 * partitioned by them.
		 *
		 * @a allow_partitioning_using_topological_plate_polygons determines if
		 * topological closed plate boundary features can be used as partitioning polygons.
		 * @a allow_partitioning_using_static_polygons determines if
		 * regular features (with static polygon geometry) can be used as partitioning polygons.
		 * By default they both are allowed but the features in @a partitioning_feature_collections
		 * should ideally only contain one type.
		 *
		 * The default value of @a feature_properties_to_assign only assigns
		 * the reconstruction plate id.
		 */
		static
		non_null_ptr_type
		create(
				AssignPlateIdMethodType assign_plate_id_method,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						partitioning_feature_collections,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_feature_collections,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const feature_property_flags_type &feature_property_types_to_assign =
						RECONSTRUCTION_PLATE_ID_PROPERTY_FLAG,
				bool allow_partitioning_using_topological_plate_polygons = true,
				bool allow_partitioning_using_static_polygons = true)
		{
			return non_null_ptr_type(new AssignPlateIds(
					assign_plate_id_method,
					partitioning_feature_collections,
					reconstruction_feature_collections,
					reconstruction_time,
					anchor_plate_id,
					feature_property_types_to_assign,
					allow_partitioning_using_topological_plate_polygons,
					allow_partitioning_using_static_polygons));
		}


		/**
		 * Returns true if we have partitioning polygons.
		 */
		bool
		has_partitioning_polygons() const;


		/**
		 * Assign reconstruction plate ids to all features in the feature collection.
		 *
		 * This will do nothing if @a has_partitioning_polygons returns false.
		 */
		void
		assign_reconstruction_plate_ids(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref);

		/**
		 * Assign reconstruction plate ids to all features in a list of features.
		 *
		 * All features in @a feature_refs should be contained by @a feature_collection_ref.
		 *
		 * This will do nothing if @a has_partitioning_polygons returns false.
		 */
		void
		assign_reconstruction_plate_ids(
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &feature_refs,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref);

		/**
		 * Assign a reconstruction plate id to a feature.
		 *
		 * @a feature_ref should be contained by @a feature_collection_ref.
		 *
		 * This will do nothing if @a has_partitioning_polygons returns false.
		 */
		void
		assign_reconstruction_plate_id(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref);

	private:
		/**
		 * The method used to assign plate ids to features.
		 */
		AssignPlateIdMethodType d_assign_plate_id_method;

		/**
		 * The types of feature properties to assign.
		 */
		feature_property_flags_type d_feature_property_types_to_assign;

		/**
		 * Contains the reconstructed polygons used for cookie-cutting.
		 */
		GPlatesModel::Reconstruction::non_null_ptr_type d_reconstruction;

		/**
		 * Used to cookie cut geometries to find partitioning polygons.
		 */
		GeometryCookieCutter d_geometry_cookie_cutter;

		//! Tasks that do the actual assigning of properties like plate id.
		std::vector< boost::shared_ptr<PartitionFeatureTask> > d_partition_feature_tasks;


		/**
		 * Create an internal @a Reconstruction using @a partitioning_feature_collections,
		 * @a reconstruction_feature_collections, @a reconstruction_time and @a anchor_plate_id
		 * to create a new set of partitioning polygons to be used for cookie-cutting.
		 */
		AssignPlateIds(
				AssignPlateIdMethodType assign_plate_id_method,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						partitioning_feature_collections,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_feature_collections,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const feature_property_flags_type &feature_property_types_to_assign,
				bool allow_partitioning_using_topological_plate_polygons,
				bool allow_partitioning_using_static_polygons);
	};
}

#endif // GPLATES_APP_LOGIC_ASSIGNPLATEIDS_H
