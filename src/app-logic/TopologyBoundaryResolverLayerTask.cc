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

#include "TopologyBoundaryResolverLayerTask.h"

#include "AppLogicUtils.h"
#include "ReconstructUtils.h"
#include "TopologyUtils.h"


const QString GPlatesAppLogic::TopologyBoundaryResolverLayerTask::TOPOLOGICAL_CLOSED_PLATES_POLYGON_FEATURES_CHANNEL_NAME =
		"Topological closed plate polygon features";
const QString GPlatesAppLogic::TopologyBoundaryResolverLayerTask::TOPOLOGICAL_BOUNDARY_SECTION_FEATURES_CHANNEL_NAME =
		"Topological boundary section features";


bool
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return TopologyUtils::has_topological_closed_plate_boundary_features(feature_collection);
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the reconstruction tree.
	input_channel_types.push_back(
			LayerInputChannelType(
					get_reconstruction_tree_channel_name(),
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RECONSTRUCTION));

	// Channel definition for the reconstructed topological section geometries referenced by
	// the topological closed plate polygons.
	input_channel_types.push_back(
			LayerInputChannelType(
					TOPOLOGICAL_BOUNDARY_SECTION_FEATURES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					LayerTaskType::RECONSTRUCT));

	// Channel definition for the topological closed plate polygon features.
	input_channel_types.push_back(
			LayerInputChannelType(
					TOPOLOGICAL_CLOSED_PLATES_POLYGON_FEATURES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL));

	// The referenced reconstructed topological section geometries have previously been obtained
	// by referencing the weak observers of those referenced features (ReconstructedFeatureGeometry
	// is a weak observer of a feature). This is basically a global search through all loaded
	// features. And this required no special input channel (since we could just get the
	// reconstructed feature geometries directly from the topological section feature themselves
	// provided they've already been reconstructed).
	//
	// However we have now added an input channel to restrict that global search to those
	// topological section features associated with the new input channel.
	// 
	// We will also, as done previously, restrict our search of those reconstructed geometries to
	// only those that were reconstructed with the same ReconstructionTree.
	// Except now it is a user option perhaps.

	return input_channel_types;
}


QString
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::get_main_input_feature_collection_channel() const
{
	return TOPOLOGICAL_CLOSED_PLATES_POLYGON_FEATURES_CHANNEL_NAME;
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == TOPOLOGICAL_CLOSED_PLATES_POLYGON_FEATURES_CHANNEL_NAME)
	{
		d_topology_boundary_resolver_layer_proxy
				->add_topological_closed_plate_polygon_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::remove_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == TOPOLOGICAL_CLOSED_PLATES_POLYGON_FEATURES_CHANNEL_NAME)
	{
		d_topology_boundary_resolver_layer_proxy
				->remove_topological_closed_plate_polygon_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::modified_input_file(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == TOPOLOGICAL_CLOSED_PLATES_POLYGON_FEATURES_CHANNEL_NAME)
	{
		// Let the reconstruct layer proxy know that one of the closed plate polygon feature collections has been modified.
		d_topology_boundary_resolver_layer_proxy
				->modified_topological_closed_plate_polygon_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::add_input_layer_proxy_connection(
		const QString &input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == get_reconstruction_tree_channel_name())
	{
		// Make sure the input layer proxy is a reconstruction layer proxy.
		boost::optional<ReconstructionLayerProxy *> reconstruction_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructionLayerProxy>(layer_proxy);
		if (reconstruction_layer_proxy)
		{
			// Stop using the default reconstruction layer proxy.
			d_using_default_reconstruction_layer_proxy = false;

			d_topology_boundary_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruction_layer_proxy.get()));
		}
	}
	else if (input_channel_name == TOPOLOGICAL_BOUNDARY_SECTION_FEATURES_CHANNEL_NAME)
	{
		// Make sure the input layer proxy is a reconstruct layer proxy.
		boost::optional<ReconstructLayerProxy *> topological_sections_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructLayerProxy>(layer_proxy);
		if (topological_sections_layer_proxy)
		{
			d_topology_boundary_resolver_layer_proxy->add_topological_sections_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_sections_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::remove_input_layer_proxy_connection(
		const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == get_reconstruction_tree_channel_name())
	{
		// Make sure the input layer proxy is a reconstruction layer proxy.
		boost::optional<ReconstructionLayerProxy *> reconstruction_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructionLayerProxy>(layer_proxy);
		if (reconstruction_layer_proxy)
		{
			// Start using the default reconstruction layer proxy.
			d_using_default_reconstruction_layer_proxy = true;

			d_topology_boundary_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					d_default_reconstruction_layer_proxy);
		}
	}
	else if (input_channel_name == TOPOLOGICAL_BOUNDARY_SECTION_FEATURES_CHANNEL_NAME)
	{
		// Make sure the input layer proxy is a reconstruct layer proxy.
		boost::optional<ReconstructLayerProxy *> topological_sections_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructLayerProxy>(layer_proxy);
		if (topological_sections_layer_proxy)
		{
			// Unset the reconstruct layer proxy.
			d_topology_boundary_resolver_layer_proxy->remove_topological_sections_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_sections_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerTask::update(
		const Layer &layer_handle /* the layer invoking this */,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		const ReconstructionLayerProxy::non_null_ptr_type &default_reconstruction_layer_proxy)
{
	d_topology_boundary_resolver_layer_proxy->set_current_reconstruction_time(reconstruction_time);

	// If our layer proxy is currently using the default reconstruction layer proxy then
	// tell our layer proxy about the new default reconstruction layer proxy.
	if (d_using_default_reconstruction_layer_proxy)
	{
		// Avoid setting it every update unless it's actually a different layer.
		if (default_reconstruction_layer_proxy != d_default_reconstruction_layer_proxy)
		{
			d_topology_boundary_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					default_reconstruction_layer_proxy);
		}
	}

	d_default_reconstruction_layer_proxy = default_reconstruction_layer_proxy;
}
