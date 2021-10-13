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

#include "TopologyNetworkResolverLayerTask.h"

#include "AppLogicUtils.h"
#include "ReconstructUtils.h"
#include "TopologyUtils.h"


const QString GPlatesAppLogic::TopologyNetworkResolverLayerTask::TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME =
		"Topological network features";


bool
GPlatesAppLogic::TopologyNetworkResolverLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return TopologyUtils::has_topological_network_features(feature_collection);
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::TopologyNetworkResolverLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the reconstruction tree.
	input_channel_types.push_back(
			LayerInputChannelType(
					get_reconstruction_tree_channel_name(),
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RECONSTRUCTION));

	// Channel definition for the topological network features.
	input_channel_types.push_back(
			LayerInputChannelType(
					TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL));

	// The referenced reconstructed topological section geometries are obtained by referencing
	// the weak observers of those referenced features (ReconstructedFeatureGeometry
	// is a weak observer of a feature). This is basically a global search through all loaded
	// features. And this requires no special input channel (since we could just get the
	// reconstructed feature geometries directly from the topological section feature themselves
	// provided they've already been reconstructed).
	//
	// So the main requirement of this layer task is to get ReconstructedFeatureGeometry objects
	// from all active "Reconstructed Geometries" layers because we don't know which ones contain
	// the referenced topological section features.
	// 
	// We will also, as done previously, restrict our search of those reconstructed geometries to
	// only those that were reconstructed with the same ReconstructionTree.
	// Except now it is a user option perhaps.

	return input_channel_types;
}


QString
GPlatesAppLogic::TopologyNetworkResolverLayerTask::get_main_input_feature_collection_channel() const
{
	return TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME;
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerTask::activate(
		bool active)
{
	// If deactivated then specify an empty set of topological sections layer proxies.
	if (!active)
	{
		d_topology_network_resolver_layer_proxy->set_current_topological_sections_layer_proxies(
				std::vector<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type>());
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME)
	{
		d_topology_network_resolver_layer_proxy
				->add_topological_network_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerTask::remove_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME)
	{
		d_topology_network_resolver_layer_proxy
				->remove_topological_network_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerTask::modified_input_file(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME)
	{
		// Let the reconstruct layer proxy know that one of the network feature collections has been modified.
		d_topology_network_resolver_layer_proxy
				->modified_topological_network_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerTask::add_input_layer_proxy_connection(
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

			d_topology_network_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruction_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerTask::remove_input_layer_proxy_connection(
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

			d_topology_network_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					d_default_reconstruction_layer_proxy);
		}
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_topology_network_resolver_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());

	// Find those layer outputs that come from a reconstruct layer.
	// These will be our topological sections layer proxies.
	// NOTE: We reference all active reconstruct layers because we don't know which ones contain
	// the topological sections that our topologies are referencing (it's a global lookup).
	std::vector<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> topological_sections_layer_proxies;
	reconstruction->get_active_layer_outputs<GPlatesAppLogic::ReconstructLayerProxy>(topological_sections_layer_proxies);
	// Notify our layer proxy of the topological sections layer proxies.
	d_topology_network_resolver_layer_proxy->set_current_topological_sections_layer_proxies(topological_sections_layer_proxies);

	// If our layer proxy is currently using the default reconstruction layer proxy then
	// tell our layer proxy about the new default reconstruction layer proxy.
	if (d_using_default_reconstruction_layer_proxy)
	{
		// Avoid setting it every update unless it's actually a different layer.
		if (reconstruction->get_default_reconstruction_layer_output() != d_default_reconstruction_layer_proxy)
		{
			d_topology_network_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					reconstruction->get_default_reconstruction_layer_output());
		}
	}

	d_default_reconstruction_layer_proxy = reconstruction->get_default_reconstruction_layer_output();
}
