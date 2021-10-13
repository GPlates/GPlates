/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_APP_LOGIC_PROPERTYVALUEPROPOGATOR_H
#define GPLATES_APP_LOGIC_PROPERTYVALUEPROPOGATOR_H

#include <bitset>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "AssignPlateIds.h"
#include "GeometryCookieCutter.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/types.h"

#include "app-logic/Reconstruction.h"
#include "app-logic/PartitionFeatureUtils.h"
#include "app-logic/GeometryUtils.h"


namespace GPlatesAppLogic
{
	class PartitionFeatureTask;

	/**
	 * Assigns reconstruction plate ids to feature(s) using resolved topological
	 * boundaries (reconstructions of TopologicalClosedPlateBoundary features).
	 */
	class PropertyValuePropogator
	{
	public:
		//! Typedef for shared pointer to @a AssignPlateIds.
		typedef boost::shared_ptr<PropertyValuePropogator> non_null_ptr_type;


		/**
		 * How plate ids are assigned to features.
		 */
// 		enum AssignPlateIdMethodType
// 		{
// 			/**
// 			 * Assign, to each feature, a single plate id corresponding to the
// 			 * plate that the feature's geometry overlaps the most.
// 			 *
// 			 * If a feature contains multiple sub-geometries then they are treated
// 			 * as one composite geometry for our purposes here.
// 			 *
// 			 * If a feature does not overlap any plates - even a little bit -
// 			 * (unlikely if the plate polygon model covers the entire globe)
// 			 * then its reconstruction plate id property is removed.
// 			 */
// 			ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE,
// 
// 			/**
// 			 * Assign, to each sub-geometry of each feature, a single plate id
// 			 * corresponding to the plate that the sub-geometry overlaps the most.
// 			 *
// 			 * This can create extra features, for example if a feature has two
// 			 * sub-geometries and one overlaps plate A the most and the other
// 			 * overlaps plate B the most then the original feature (with the two
// 			 * geometries) will get split into two features - one feature will contain
// 			 * the first geometry (and have plate id A) and the other feature will
// 			 * contain the second geometry (and have plate id B).
// 			 * The original feature will be used for the first feature and
// 			 * a cloned version of the original feature will be used for the second -
// 			 * the original feature will have its second geometry removed since it's
// 			 * now being moved over to the second (cloned) feature.
// 			 * When cloning, all non-geometry properties of the original feature are
// 			 * copied over.
// 			 *
// 			 * If a feature sub-geometry does not overlap any plates (unlikely if the
// 			 * plate polygon model covers the entire globe) then the
// 			 * reconstruction plate id property is removed from its containing feature.
// 			 * Using the above example, if one of the two geometries overlaps no plates
// 			 * and the other overlaps plate A then the first feature has no plate id
// 			 * while the second feature has plate id A.
// 			 */
// 			ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE,
// 
// 
// 			/**
// 			 * Partition all geometries of each feature into the plates
// 			 * containing them (intersecting them as needed and assigning the
// 			 * resulting partitioned geometry to the appropriate plates).
// 			 *
// 			 * This can create extra features, for example if a feature has only one
// 			 * sub-geometry but it overlaps plate A and plate B then it is partitioned
// 			 * into geometry that is fully contained by plate A and likewise for plate B.
// 			 * These two partitioned geometries will now be two features since they
// 			 * have different plate ids.
// 			 * The original feature will contain the first partitioned geometry (and
// 			 * have plate id A) while a new cloned feature will contain the second
// 			 * partitioned geometry (and have plate id B).
// 			 * When cloning, all non-geometry properties of the original feature are
// 			 * copied over.
// 			 *
// 			 * If there are any partitioned geometries left over that do not belong to
// 			 * any plate (unlikely if the plate polygon model covers the entire globe)
// 			 * then they are stored in the original feature and it will have no plate id.
// 			 * In this case all the partitioned geometry that is inside plates will be
// 			 * placed in new cloned features (one feature for each plate).
// 			 */
// 			PARTITION_FEATURE
// 		};


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
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method,
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
			return non_null_ptr_type(new PropertyValuePropogator(
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

		void
		propogate_property_value(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
				PartitionFeatureUtils::SimplePropertyValueAssigner::PropertyNameFeatureCollectionMap& property_feature_collection_map)
		{
			const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> partitioned_feature =
				PartitionFeatureUtils::partition_feature(
				feature_ref,
				d_geometry_cookie_cutter);

			boost::shared_ptr<PartitionFeatureUtils::PropertyValueAssigner> property_value_assigner(
				new PartitionFeatureUtils::SimplePropertyValueAssigner(property_feature_collection_map));

			// Used to create/clone features for extra partitioned geometries that require
			// different plate ids.
			PartitionFeatureUtils::PartitionedFeatureManager partitioned_feature_manager(
				feature_ref,
				feature_collection_ref,
				property_value_assigner);

			// Now that we've partitioned the feature's geometry properties we can
			// strip off all geometry properties from the feature.
			// This is so we can add new geometry properties later using the above
			// partitioned information.
			GPlatesAppLogic::GeometryUtils::remove_geometry_properties_from_feature(feature_ref);

			//
			// Iterate over the results of the partitioned feature.
			//
			PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
				properties_iter = partitioned_feature->partitioned_geometry_properties.begin();
			PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
				properties_end = partitioned_feature->partitioned_geometry_properties.end();
#if 0	
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				// The results of partitioning the current geometry property.
				const PartitionFeatureUtils::PartitionedFeature::GeometryProperty &geometry_property =
					*properties_iter;

				// Add any partitioned outside geometries to the outside feature.
				if (!geometry_property.partitioned_outside_geometries.empty())
				{
					PartitionFeatureUtils::add_partitioned_geometry_to_feature(
						geometry_property.partitioned_outside_geometries,
						geometry_property.geometry_property_name,
						partitioned_feature_manager,
						d_reconstruction->reconstruction_tree());
				}

				// Add any partitioned inside geometries to the partitioned features corresponding
				// to the partitioning polygons.
				if (!geometry_property.partitioned_inside_geometries.empty())
				{
					PartitionFeatureUtils::add_partitioned_geometry_to_feature(
						geometry_property.partitioned_inside_geometries,
						geometry_property.geometry_property_name,
						partitioned_feature_manager,
						d_reconstruction->reconstruction_tree());
				}
			}
#endif
			
		}

	private:
		/**
		 * The method used to assign plate ids to features.
		 */
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType d_assign_plate_id_method;

		/**
		 * The types of feature properties to assign.
		 */
		feature_property_flags_type d_feature_property_types_to_assign;

		/**
		 * Contains the reconstructed polygons used for cookie-cutting.
		 */
		GPlatesAppLogic::Reconstruction::non_null_ptr_type d_reconstruction;

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
		PropertyValuePropogator(
			GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method,
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

#endif // GPLATES_APP_LOGIC_PROPERTYVALUEPROPOGATOR_H
