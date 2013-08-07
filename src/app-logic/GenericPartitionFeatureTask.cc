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

#include "GenericPartitionFeatureTask.h"

#include "GeometryCookieCutter.h"
#include "GeometryUtils.h"
#include "ReconstructionGeometryUtils.h"

#include "global/GPlatesAssert.h"

#include "model/NotificationGuard.h"

#include "utils/Profile.h"


GPlatesAppLogic::GenericPartitionFeatureTask::GenericPartitionFeatureTask(
		const ReconstructionTree &reconstruction_tree,
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method,
		const GPlatesAppLogic::AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign) :
	d_reconstruction_tree(reconstruction_tree),
	d_assign_plate_id_method(assign_plate_id_method),
	d_feature_property_types_to_assign(feature_property_types_to_assign)
{
}


void
GPlatesAppLogic::GenericPartitionFeatureTask::partition_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		const GeometryCookieCutter &geometry_cookie_cutter,
		bool respect_feature_time_period)
{
	//PROFILE_FUNC();

	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(*feature_ref->model_ptr());

	// Partition the feature and get the partitioned results in return.
	// NOTE: This does not modify the feature referenced by 'feature_ref'.
	// NOTE: We call this here before any modifications (such as removing geometry properties)
	// are made to the feature - later on we can modify the feature knowing that we
	// have all the partitioning results.
	const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> partitioned_feature =
			PartitionFeatureUtils::partition_feature(
					feature_ref,
					geometry_cookie_cutter,
					// Determines if partition feature wen not defined at the reconstruction time...
					respect_feature_time_period);

	// If the feature being partitioned does not exist at the reconstruction time of
	// the cookie cutter then return early and do nothing.
	if (!partitioned_feature)
	{
		return;
	}

	// Assigns plate id and time period properties from partitioning polygon to
	// the partitioned feature(s).
	//
	// The original feature will get used to store some of the partitioned geometry while
	// clones will get used to store remaining partitioned geometry (because
	// property values such as plate id might be different and hence need to be
	// stored in a separate feature).
	//
	// So for this reason the property value assigner must always overwrite a
	// property value if it exists (ie, remove it first and then add a new one).
	//
	// NOTE: The default property values (to use for partitioned features outside all
	// partitioning polygons) are obtained from the feature here.
	boost::shared_ptr<PartitionFeatureUtils::PropertyValueAssigner> property_value_assigner(
			new PartitionFeatureUtils::GenericFeaturePropertyAssigner(
					feature_ref,
					d_feature_property_types_to_assign));

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
	GeometryUtils::remove_geometry_properties_from_feature(feature_ref);

	partition_feature(
			feature_ref,
			partitioned_feature,
			partitioned_feature_manager);
}


void
GPlatesAppLogic::GenericPartitionFeatureTask::partition_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
		PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager)
{
	// Assigned plate ids based on the method selected by the caller.
	switch (d_assign_plate_id_method)
	{
	case GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE:
		assign_feature_to_plate_it_overlaps_the_most(
				feature_ref,
				partitioned_feature,
				partitioned_feature_manager);
		break;

	case GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE:
		assign_feature_sub_geometry_to_plate_it_overlaps_the_most(
				feature_ref,
				partitioned_feature,
				partitioned_feature_manager);
		break;

	case GPlatesAppLogic::AssignPlateIds::PARTITION_FEATURE:
		partition_feature_into_plates(
				feature_ref,
				partitioned_feature,
				partitioned_feature_manager);
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


void
GPlatesAppLogic::GenericPartitionFeatureTask::assign_feature_to_plate_it_overlaps_the_most(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
		PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager)
{
	// Find the partitioning polygon that contains the most partitioned geometry
	// over all geometry properties of the feature.
	const boost::optional<const ReconstructionGeometry *> partition =
			PartitionFeatureUtils::find_partition_containing_most_geometry(*partitioned_feature);

	//
	// Iterate over the results of the partitioned feature.
	//
	PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
			properties_iter = partitioned_feature->partitioned_geometry_properties.begin();
	PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
			properties_end = partitioned_feature->partitioned_geometry_properties.end();
	for ( ; properties_iter != properties_end; ++properties_iter)
	{
		// The results of partitioning the current geometry property.
		const PartitionFeatureUtils::PartitionedFeature::GeometryProperty &geometry_property =
				*properties_iter;

		// Transfer current geometry property to the feature associated with the
		// partitioning polygon 'partition' that contained the most geometry.
		// This will create a new feature if necessary (but only the first time it's called
		// since the same 'partition' is passed in each time).
		PartitionFeatureUtils::add_partitioned_geometry_to_feature(
				geometry_property.geometry_property_clone,
				partitioned_feature_manager,
				d_reconstruction_tree,
				partition);
	}
}


void
GPlatesAppLogic::GenericPartitionFeatureTask::assign_feature_sub_geometry_to_plate_it_overlaps_the_most(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
		PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager)
{
	//
	// Iterate over the results of the partitioned feature.
	//
	PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
			properties_iter = partitioned_feature->partitioned_geometry_properties.begin();
	PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
			properties_end = partitioned_feature->partitioned_geometry_properties.end();
	for ( ; properties_iter != properties_end; ++properties_iter)
	{
		// The results of partitioning the current geometry property.
		const PartitionFeatureUtils::PartitionedFeature::GeometryProperty &geometry_property =
				*properties_iter;

		// Find the partitioning polygon that contains the most partitioned geometry
		// of the current geometry property.
		const boost::optional<const ReconstructionGeometry *> partition =
				PartitionFeatureUtils::find_partition_containing_most_geometry(geometry_property);

		// Transfer current geometry property to the feature associated with the
		// partitioning polygon 'partition' that contained the most geometry.
		// This will create a new feature if necessary (since 'partition' can
		// be different each time this is called).
		PartitionFeatureUtils::add_partitioned_geometry_to_feature(
				geometry_property.geometry_property_clone,
				partitioned_feature_manager,
				d_reconstruction_tree,
				partition);
	}
}


void
GPlatesAppLogic::GenericPartitionFeatureTask::partition_feature_into_plates(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> &partitioned_feature,
		PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager)
{
	//PROFILE_FUNC();

	//
	// Iterate over the results of the partitioned feature.
	//
	PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
			properties_iter = partitioned_feature->partitioned_geometry_properties.begin();
	PartitionFeatureUtils::PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator
			properties_end = partitioned_feature->partitioned_geometry_properties.end();
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
					d_reconstruction_tree);
		}

		// Add any partitioned inside geometries to the partitioned features corresponding
		// to the partitioning polygons.
		if (!geometry_property.partitioned_inside_geometries.empty())
		{
			PartitionFeatureUtils::add_partitioned_geometry_to_feature(
					geometry_property.partitioned_inside_geometries,
					geometry_property.geometry_property_name,
					partitioned_feature_manager,
					d_reconstruction_tree);
		}
	}
}
