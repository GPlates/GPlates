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
#include "ReconstructedFeatureGeometry.h"
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
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &partitioning_feature_collections,
		const ReconstructionTreeCreator &default_reconstruction_tree_creator,
		const double &reconstruction_time,
		const feature_property_flags_type &feature_property_types_to_assign,
		bool verify_information_model,
		bool respect_feature_time_period) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_feature_property_types_to_assign(feature_property_types_to_assign),
	d_default_reconstruct_method_context(
			ReconstructParams(),
			default_reconstruction_tree_creator),
	d_reconstruction_time(reconstruction_time),
	d_respect_feature_time_period(respect_feature_time_period)
{
	ReconstructMethodRegistry reconstruct_method_registry;

	d_geometry_cookie_cutter.reset(
			new GeometryCookieCutter(
					reconstruction_time,
					reconstruct_method_registry,
					partitioning_feature_collections,
					default_reconstruction_tree_creator,
					true/*group_networks_then_boundaries_then_static_polygons*/,
					GeometryCookieCutter::SORT_BY_PLATE_ID,
					// Use high speed point-in-poly testing since we're being used for
					// generalised cookie-cutting and we could be asked to test lots of points.
					GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE));

	d_partition_feature_tasks =
			// Get all tasks that assign properties from polygon features to partitioned features.
			get_partition_feature_tasks(
					assign_plate_id_method,
					feature_property_types_to_assign,
					verify_information_model);
}


GPlatesAppLogic::AssignPlateIds::AssignPlateIds(
		AssignPlateIdMethodType assign_plate_id_method,
		const std::vector<LayerProxy::non_null_ptr_type> &partitioning_layer_proxies,
		const ReconstructionTreeCreator &default_reconstruction_tree_creator,
		const double &reconstruction_time,
		const feature_property_flags_type &feature_property_types_to_assign,
		bool verify_information_model,
		bool respect_feature_time_period) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_feature_property_types_to_assign(feature_property_types_to_assign),
	d_default_reconstruct_method_context(
			ReconstructParams(),
			default_reconstruction_tree_creator),
	d_reconstruction_time(reconstruction_time),
	d_respect_feature_time_period(respect_feature_time_period)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!partitioning_layer_proxies.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Contains the reconstructed static polygons used for cookie-cutting.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_static_polygons;

	// Contains the resolved topological polygons used for cookie-cutting.
	std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;

	// Contains the resolved topological networks used for cookie-cutting.
	// See comment in header for why a deforming region is currently used to assign plate ids.
	std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;

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
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		boost::optional<const ReconstructMethodInterface::Context &> reconstruct_method_context)
{
	if (!feature_collection_ref.is_valid())
	{
		return;
	}

	GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection_ref->begin();
	GPlatesModel::FeatureCollectionHandle::iterator feature_end = feature_collection_ref->end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		assign_reconstruction_plate_id(
				(*feature_iter)->reference(),
				feature_collection_ref,
				reconstruct_method_context);
	}
}


void
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_ids(
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &feature_refs,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		boost::optional<const ReconstructMethodInterface::Context &> reconstruct_method_context)
{
	typedef std::vector<GPlatesModel::FeatureHandle::weak_ref> feature_ref_seq_type;

	for (feature_ref_seq_type::const_iterator feature_ref_iter = feature_refs.begin();
		feature_ref_iter != feature_refs.end();
		++feature_ref_iter)
	{
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref = *feature_ref_iter;

		assign_reconstruction_plate_id(
				feature_ref,
				feature_collection_ref,
				reconstruct_method_context);
	}
}


void
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_id(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		boost::optional<const ReconstructMethodInterface::Context &> reconstruct_method_context)
{
	assign_reconstruction_plate_id_internal(
			feature_ref,
			feature_collection_ref,
			reconstruct_method_context
				? reconstruct_method_context.get()
				: d_default_reconstruct_method_context);
}


void
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_id_internal(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		const ReconstructMethodInterface::Context &reconstruct_method_context)
{
	if (!feature_ref.is_valid())
	{
		return;
	}

	// Merge model events across this scope to avoid excessive number of model callbacks
	// due to modifying features by partitioning them.
	GPlatesModel::NotificationGuard model_notification_guard(*feature_ref->model_ptr());

	// Iterate through the tasks until we find one that can partition the feature.
	partition_feature_task_ptr_seq_type::const_iterator assign_task_iter = d_partition_feature_tasks.begin();
	partition_feature_task_ptr_seq_type::const_iterator assign_task_end = d_partition_feature_tasks.end();
	for ( ; assign_task_iter != assign_task_end; ++assign_task_iter)
	{
		const partition_feature_task_ptr_type &assign_task = *assign_task_iter;

		if (assign_task->can_partition_feature(feature_ref))
		{
			assign_task->partition_feature(
					feature_ref,
					feature_collection_ref,
					*d_geometry_cookie_cutter,
					reconstruct_method_context,
					d_reconstruction_time,
					d_respect_feature_time_period);

			return;
		}
	}
}
