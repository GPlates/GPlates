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
#include "LayerProxyUtils.h"
#include "PartitionFeatureTask.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructParams.h"
#include "ReconstructUtils.h"
#include "ResolvedTopologicalBoundary.h"
#include "TopologyBoundaryResolverLayerProxy.h"
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
	d_feature_property_types_to_assign(feature_property_types_to_assign)
{
	ReconstructionTreeCreator reconstruction_tree_cache =
			get_cached_reconstruction_tree_creator(
					reconstruction_feature_collections,
					reconstruction_time,
					anchor_plate_id,
					10/*max_num_reconstruction_trees_in_cache*/);

	ReconstructMethodRegistry reconstruct_method_registry;
	register_default_reconstruct_method_types(reconstruct_method_registry);

	// Contains the reconstructed static polygons used for cookie-cutting.
	// Can also contain the topological section geometries referenced by topological polygons.
	std::vector<reconstructed_feature_geometry_non_null_ptr_type> reconstructed_feature_geometries;

	const ReconstructHandle::type reconstruct_handle = ReconstructUtils::reconstruct(
			reconstructed_feature_geometries,
			reconstruction_time,
			anchor_plate_id,
			reconstruct_method_registry,
			partitioning_feature_collections,
			reconstruction_tree_cache);

	if (allow_partitioning_using_topological_plate_polygons)
	{
		std::vector<ReconstructHandle::type> reconstruct_handles(1, reconstruct_handle);

		// Contains the resolved topological polygons used for cookie-cutting.
		std::vector<resolved_topological_boundary_non_null_ptr_type> resolved_topological_boundaries;

		TopologyUtils::resolve_topological_boundaries(
				resolved_topological_boundaries,
				partitioning_feature_collections,
				reconstruction_tree_cache.get_reconstruction_tree(),
				reconstruct_handles);

		d_geometry_cookie_cutter.reset(
				new GeometryCookieCutter(
						reconstruction_time,
						reconstructed_feature_geometries,
						resolved_topological_boundaries,
						allow_partitioning_using_static_polygons));
	}
	else
	{
		d_geometry_cookie_cutter.reset(
				new GeometryCookieCutter(
						reconstruction_time,
						reconstructed_feature_geometries,
						boost::none,
						allow_partitioning_using_static_polygons));
	}

	d_partition_feature_tasks =
			// Get all tasks that assign properties from polygon features to partitioned features.
			get_partition_feature_tasks(
					*reconstruction_tree_cache.get_reconstruction_tree(),
					assign_plate_id_method,
					feature_property_types_to_assign);
}


GPlatesAppLogic::AssignPlateIds::AssignPlateIds(
		AssignPlateIdMethodType assign_plate_id_method,
		const LayerProxy::non_null_ptr_type &partitioning_layer_proxy,
		const double &reconstruction_time,
		const feature_property_flags_type &feature_property_types_to_assign) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_feature_property_types_to_assign(feature_property_types_to_assign)
{
	// Contains the reconstructed static polygons used for cookie-cutting.
	std::vector<reconstructed_feature_geometry_non_null_ptr_type> reconstructed_static_polygons;

	// Contains the resolved topological polygons used for cookie-cutting.
	std::vector<resolved_topological_boundary_non_null_ptr_type> resolved_topological_boundaries;

	// The reconstruction tree of the static/dynamic polygons - used to reverse reconstruct if necessary.
	boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree;


	// See if the input layer proxy is a reconstruct layer proxy.
	boost::optional<ReconstructLayerProxy *> static_polygons_layer_proxy =
			LayerProxyUtils::get_layer_proxy_derived_type<
					ReconstructLayerProxy>(partitioning_layer_proxy);
	if (static_polygons_layer_proxy)
	{
		static_polygons_layer_proxy.get()->get_reconstructed_feature_geometries(
				reconstructed_static_polygons,
				reconstruction_time);

		reconstruction_tree = static_polygons_layer_proxy.get()->get_reconstruction_layer_proxy()
				->get_reconstruction_tree(reconstruction_time);
	}

	// See if the input layer proxy is a topology boundary resolver layer proxy.
	boost::optional<TopologyBoundaryResolverLayerProxy *> dynamic_polygons_layer_proxy =
			LayerProxyUtils::get_layer_proxy_derived_type<
					TopologyBoundaryResolverLayerProxy>(partitioning_layer_proxy);
	if (dynamic_polygons_layer_proxy)
	{
		dynamic_polygons_layer_proxy.get()->get_resolved_topological_boundaries(
				resolved_topological_boundaries,
				reconstruction_time);

		reconstruction_tree = dynamic_polygons_layer_proxy.get()->get_reconstruction_layer_proxy()
				->get_reconstruction_tree(reconstruction_time);
	}

	d_geometry_cookie_cutter.reset(
			new GeometryCookieCutter(
					reconstruction_time,
					reconstructed_static_polygons,
					resolved_topological_boundaries,
					true/*allow_partitioning_using_static_polygons*/));

	// If there's no partitioning polygons then there's no reconstruction tree so
	// just create an empty dummy one.
	if (!reconstruction_tree)
	{
		reconstruction_tree = create_reconstruction_tree(reconstruction_time, 0/*anchor_plate_id*/);
	}

	d_partition_feature_tasks =
			// Get all tasks that assign properties from polygon features to partitioned features.
			get_partition_feature_tasks(
					*reconstruction_tree.get(),
					assign_plate_id_method,
					feature_property_types_to_assign);
}


GPlatesAppLogic::AssignPlateIds::~AssignPlateIds()
{
	// Destructor defined in '.cc' so have access to complete type of data members (for their destructors).
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
