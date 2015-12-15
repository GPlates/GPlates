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

#include <boost/foreach.hpp>

#include "AssignPlateIds.h"

#include "LayerProxyUtils.h"
#include "PartitionFeatureTask.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructParams.h"
#include "ReconstructUtils.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "model/NotificationGuard.h"

#include "utils/Profile.h"

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
		bool verify_information_model,
		bool allow_partitioning_using_topological_plate_polygons,
		bool allow_partitioning_using_topological_networks,
		bool allow_partitioning_using_static_polygons,
		bool respect_feature_time_period) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_feature_property_types_to_assign(feature_property_types_to_assign),
	d_respect_feature_time_period(respect_feature_time_period)
{
	ReconstructionTreeCreator reconstruction_tree_cache =
			create_cached_reconstruction_tree_creator(
					reconstruction_feature_collections,
					anchor_plate_id,
					10/*max_num_reconstruction_trees_in_cache*/,
					// We're not going to modify the reconstruction features so no need to clone...
					false/*clone_reconstruction_features*/);

	ReconstructMethodRegistry reconstruct_method_registry;

	// Contains the reconstructed static polygons used for cookie-cutting.
	// Can also contain the topological section geometries referenced by topologies.
	std::vector<reconstructed_feature_geometry_non_null_ptr_type> reconstructed_feature_geometries;

	const ReconstructHandle::type reconstruct_handle = ReconstructUtils::reconstruct(
			reconstructed_feature_geometries,
			reconstruction_time,
			reconstruct_method_registry,
			partitioning_feature_collections,
			reconstruction_tree_cache);

	std::vector<ReconstructHandle::type> reconstruct_handles(1, reconstruct_handle);

	// Contains the resolved topological line sections referenced by topological polygons and networks.
	std::vector<resolved_topological_line_non_null_ptr_type> resolved_topological_lines;
	if (allow_partitioning_using_topological_plate_polygons ||
		allow_partitioning_using_topological_networks)
	{
		// Resolving topological lines generates its own reconstruct handle that will be used by
		// topological polygons and networks to find this group of resolved lines.
		const ReconstructHandle::type resolved_topological_lines_handle =
				TopologyUtils::resolve_topological_lines(
						resolved_topological_lines,
						partitioning_feature_collections,
						reconstruction_tree_cache, 
						reconstruction_time,
						// Resolved topo lines use the reconstructed non-topo geometries...
						reconstruct_handles);

		reconstruct_handles.push_back(resolved_topological_lines_handle);
	}

	// Contains the resolved topological polygons used for cookie-cutting.
	std::vector<resolved_topological_boundary_non_null_ptr_type> resolved_topological_boundaries;

	// Contains the resolved topological networks used for cookie-cutting.
	// See comment in header for why a deforming region is currently used to assign plate ids.
	std::vector<resolved_topological_network_non_null_ptr_type> resolved_topological_networks;

	if (allow_partitioning_using_topological_plate_polygons)
	{
		TopologyUtils::resolve_topological_boundaries(
				resolved_topological_boundaries,
				partitioning_feature_collections,
				reconstruction_tree_cache, 
				reconstruction_time,
				// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
				reconstruct_handles);
	}

	if (allow_partitioning_using_topological_networks)
	{
		TopologyUtils::resolve_topological_networks(
				resolved_topological_networks,
				reconstruction_time,
				partitioning_feature_collections,
				// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
				reconstruct_handles);
	}

	// Contains the reconstructed static polygons used for cookie-cutting.
	boost::optional<const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &> reconstructed_static_polygons;
	if (allow_partitioning_using_static_polygons)
	{
		reconstructed_static_polygons = reconstructed_feature_geometries;
	}

	d_geometry_cookie_cutter.reset(
			new GeometryCookieCutter(
					reconstruction_time,
					reconstructed_static_polygons,
					resolved_topological_boundaries,
					resolved_topological_networks,
					GeometryCookieCutter::SORT_BY_PLATE_ID,
					// Use high speed point-in-poly testing since we're being used for
					// generalised cookie-cutting and we could be asked to test lots of points.
					GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE));

	d_partition_feature_tasks =
			// Get all tasks that assign properties from polygon features to partitioned features.
			get_partition_feature_tasks(
					*reconstruction_tree_cache.get_reconstruction_tree(reconstruction_time),
					assign_plate_id_method,
					feature_property_types_to_assign,
					verify_information_model);
}


GPlatesAppLogic::AssignPlateIds::AssignPlateIds(
		AssignPlateIdMethodType assign_plate_id_method,
		const std::vector<LayerProxy::non_null_ptr_type> &partitioning_layer_proxies,
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
		const feature_property_flags_type &feature_property_types_to_assign,
		bool verify_information_model,
		bool respect_feature_time_period) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_feature_property_types_to_assign(feature_property_types_to_assign),
	d_respect_feature_time_period(respect_feature_time_period)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!partitioning_layer_proxies.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Contains the reconstructed static polygons used for cookie-cutting.
	std::vector<reconstructed_feature_geometry_non_null_ptr_type> reconstructed_static_polygons;

	// Contains the resolved topological polygons used for cookie-cutting.
	std::vector<resolved_topological_boundary_non_null_ptr_type> resolved_topological_boundaries;

	// Contains the resolved topological networks used for cookie-cutting.
	// See comment in header for why a deforming region is currently used to assign plate ids.
	std::vector<resolved_topological_network_non_null_ptr_type> resolved_topological_networks;

	const double reconstruction_time = reconstruction_tree->get_reconstruction_time();

	// Iterate over the partitioning layer  proxies.
	BOOST_FOREACH(const LayerProxy::non_null_ptr_type &partitioning_layer_proxy, partitioning_layer_proxies)
	{
		// See if the input layer proxy is a reconstruct layer proxy.
		boost::optional<ReconstructLayerProxy *> static_polygons_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructLayerProxy>(partitioning_layer_proxy);
		if (static_polygons_layer_proxy)
		{
			static_polygons_layer_proxy.get()->get_reconstructed_feature_geometries(
					reconstructed_static_polygons,
					reconstruction_time);
		}


		// See if the input layer proxy is a topology boundary resolver layer proxy.
		boost::optional<TopologyGeometryResolverLayerProxy *> dynamic_polygons_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						TopologyGeometryResolverLayerProxy>(partitioning_layer_proxy);
		if (dynamic_polygons_layer_proxy)
		{
			dynamic_polygons_layer_proxy.get()->get_resolved_topological_boundaries(
				resolved_topological_boundaries,
				reconstruction_time);
		}

		// See if the input layer proxy is a topology network resolver layer proxy.
		// See comment in header for why a deforming region is currently used to assign plate ids.
		boost::optional<TopologyNetworkResolverLayerProxy *> dynamic_networks_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						TopologyNetworkResolverLayerProxy>(partitioning_layer_proxy);
		if (dynamic_networks_layer_proxy)
		{
			dynamic_networks_layer_proxy.get()->get_resolved_topological_networks(
					resolved_topological_networks,
					reconstruction_time);
		}
	}

	d_geometry_cookie_cutter.reset(
			new GeometryCookieCutter(
					reconstruction_time,
					reconstructed_static_polygons,
					resolved_topological_boundaries,
					resolved_topological_networks,
					GeometryCookieCutter::SORT_BY_PLATE_ID,
					// Use high speed point-in-poly testing since we're being used for
					// generalised cookie-cutting and we could be asked to test lots of points.
					// For example, very dense velocity meshes go through this path.
					GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE));

	d_partition_feature_tasks =
			// Get all tasks that assign properties from polygon features to partitioned features.
			get_partition_feature_tasks(
					*reconstruction_tree,
					assign_plate_id_method,
					feature_property_types_to_assign,
					verify_information_model);
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
	//PROFILE_FUNC();

	if (!feature_ref.is_valid())
	{
		return;
	}

	// Merge model events across this scope to avoid excessive number of model callbacks
	// due to modifying features by partitioning them.
	GPlatesModel::NotificationGuard model_notification_guard(*feature_ref->model_ptr());

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
					*d_geometry_cookie_cutter,
					d_respect_feature_time_period);

			return;
		}
	}
}
