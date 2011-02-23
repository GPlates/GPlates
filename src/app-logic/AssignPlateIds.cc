/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "AssignPlateIds.h"

#include "ClassifyFeatureCollection.h"
#include "PartitionFeatureTask.h"
#include "ReconstructUtils.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


const GPlatesAppLogic::AssignPlateIds::feature_property_flags_type
		GPlatesAppLogic::AssignPlateIds::RECONSTRUCTION_PLATE_ID_PROPERTY_FLAG =
				GPlatesAppLogic::AssignPlateIds::feature_property_flags_type().set(
						GPlatesAppLogic::AssignPlateIds::RECONSTRUCTION_PLATE_ID);


GPlatesAppLogic::AssignPlateIds::AssignPlateIds(
		AssignPlateIdMethodType assign_plate_id_method,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				partitioning_feature_collections,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_feature_collections,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const feature_property_flags_type &feature_property_types_to_assign,
		bool allow_partitioning_using_topological_plate_polygons,
		bool allow_partitioning_using_static_polygons) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_feature_property_types_to_assign(feature_property_types_to_assign),
	d_reconstruction_tree(
			GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
					reconstruction_time,
					anchor_plate_id,
					reconstruction_feature_collections)),
	d_reconstructed_geometries_collection(
			GPlatesAppLogic::ReconstructUtils::reconstruct(
					d_reconstruction_tree,
					partitioning_feature_collections))
{
	if (allow_partitioning_using_topological_plate_polygons)
	{
		d_resolved_topological_boundaries_collection =
				TopologyUtils::resolve_topological_boundaries(
						d_reconstruction_tree,
						partitioning_feature_collections);

		d_geometry_cookie_cutter.reset(
				new GeometryCookieCutter(
						*d_reconstructed_geometries_collection,
						**d_resolved_topological_boundaries_collection,
						allow_partitioning_using_static_polygons));
	}
	else
	{
		d_geometry_cookie_cutter.reset(
				new GeometryCookieCutter(
						*d_reconstructed_geometries_collection,
						boost::none,
						allow_partitioning_using_static_polygons));
	}

	d_partition_feature_tasks =
			// Get all tasks that assign properties from polygon features to partitioned features.
			get_partition_feature_tasks(
					*d_reconstructed_geometries_collection->reconstruction_tree(),
					assign_plate_id_method,
					feature_property_types_to_assign);
}


bool
GPlatesAppLogic::AssignPlateIds::has_partitioning_polygons() const
{
	return d_geometry_cookie_cutter->has_partitioning_polygons();
}


void
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_ids(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	if (!feature_collection_ref.is_valid())
	{
		return;
	}

	GPlatesModel::FeatureCollectionHandle::iterator feature_iter =
			feature_collection_ref->begin();
	GPlatesModel::FeatureCollectionHandle::iterator feature_end =
			feature_collection_ref->end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		assign_reconstruction_plate_id(
				(*feature_iter)->reference(),
				feature_collection_ref);
	}
}


void
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_ids(
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &feature_refs,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	typedef std::vector<GPlatesModel::FeatureHandle::weak_ref> feature_ref_seq_type;

	for (feature_ref_seq_type::const_iterator feature_ref_iter = feature_refs.begin();
		feature_ref_iter != feature_refs.end();
		++feature_ref_iter)
	{
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref = *feature_ref_iter;

		if (feature_ref.is_valid())
		{
			assign_reconstruction_plate_id(feature_ref, feature_collection_ref);
		}
	}
}


void
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_id(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	if (!feature_ref.is_valid())
	{
		return;
	}

	// Iterate through the tasks until we find one that can partition the feature.
	partition_feature_task_ptr_seq_type::const_iterator assign_task_iter =
			d_partition_feature_tasks.begin();
	partition_feature_task_ptr_seq_type::const_iterator assign_task_end =
			d_partition_feature_tasks.end();
	for ( ; assign_task_iter != assign_task_end; ++assign_task_iter)
	{
		const partition_feature_task_ptr_type &assign_task = *assign_task_iter;

		if (assign_task->can_partition_feature(feature_ref))
		{
			assign_task->partition_feature(
					feature_ref,
					feature_collection_ref,
					*d_geometry_cookie_cutter);

			return;
		}
	}
}
