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

#include <algorithm>

#include "ReconstructLayerTask.h"

#include "ApplicationState.h"
#include "AppLogicUtils.h"
#include "LayerProxyUtils.h"
#include "ReconstructMethodRegistry.h"
#include "ReconstructUtils.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"


GPlatesAppLogic::ReconstructLayerTask::ReconstructLayerTask(
		const ReconstructMethodRegistry &reconstruct_method_registry) :
	d_layer_params(ReconstructLayerParams::create()),
	d_default_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
	d_using_default_reconstruction_layer_proxy(true),
	d_reconstruct_layer_proxy(
			ReconstructLayerProxy::create(
					reconstruct_method_registry,
					d_layer_params->get_reconstruct_params()))
{
	// Notify our layer output whenever the layer params are modified.
	QObject::connect(
			d_layer_params.get(), SIGNAL(modified_reconstruct_params(GPlatesAppLogic::ReconstructLayerParams &)),
			this, SLOT(handle_reconstruct_params_modified(GPlatesAppLogic::ReconstructLayerParams &)));
}


bool
GPlatesAppLogic::ReconstructLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		ApplicationState &application_state)
{
	return ReconstructUtils::has_reconstructable_features(
			feature_collection,
			application_state.get_reconstruct_method_registry());
}


boost::shared_ptr<GPlatesAppLogic::ReconstructLayerTask>
GPlatesAppLogic::ReconstructLayerTask::create_layer_task(
		ApplicationState &application_state)
{
	return boost::shared_ptr<ReconstructLayerTask>(
			new ReconstructLayerTask(
					application_state.get_reconstruct_method_registry()));
}

std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::ReconstructLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the reconstruction tree.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::RECONSTRUCTION_TREE,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RECONSTRUCTION)); // rotations

	// Channel definition for the reconstructable features.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::RECONSTRUCTABLE_FEATURES,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL));


	// Channel definition for the topology surfaces on which to reconstruct geometries (if using topologies to reconstruct):
	// - resolved topological closed plate boundaries.
	// - resolved topological networks.
	std::vector<LayerTaskType::Type> topology_surfaces_input_channel_types;
	topology_surfaces_input_channel_types.push_back(LayerTaskType::TOPOLOGY_NETWORK_RESOLVER);
	topology_surfaces_input_channel_types.push_back(LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER);
	input_channel_types.push_back(
		LayerInputChannelType(
				LayerInputChannelName::TOPOLOGY_SURFACES,
				LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
				topology_surfaces_input_channel_types));

	return input_channel_types;
}


GPlatesAppLogic::LayerInputChannelName::Type
GPlatesAppLogic::ReconstructLayerTask::get_main_input_feature_collection_channel() const
{
	return LayerInputChannelName::RECONSTRUCTABLE_FEATURES;
}


void
GPlatesAppLogic::ReconstructLayerTask::activate(
		bool active)
{
	// If deactivated then flush any RFGs contained in the layer proxy.
	if (!active)
	{
		// This is done so that topologies will no longer find the RFGs when they lookup observers
		// of topological section features.
		// This issue exists because the topology layers do not necessarily restrict topological sections
		// to their input channels (unless topological sections input channels are connected), instead
		// using an unrestricted global feature id lookup even though this 'reconstruct' layer has
		// been disconnected from the topology layer in question.
		d_reconstruct_layer_proxy->reset_reconstruction_cache();
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::add_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTABLE_FEATURES)
	{
		d_reconstruct_layer_proxy->add_reconstructable_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::remove_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTABLE_FEATURES)
	{
		d_reconstruct_layer_proxy->remove_reconstructable_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::modified_input_file(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTABLE_FEATURES)
	{
		// Let the reconstruct layer proxy know that one of the reconstructable feature collections has been modified.
		d_reconstruct_layer_proxy->modified_reconstructable_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::add_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTION_TREE)
	{
		// Make sure the input layer proxy is a reconstruction layer proxy.
		boost::optional<ReconstructionLayerProxy *> reconstruction_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructionLayerProxy>(layer_proxy);
		if (reconstruction_layer_proxy)
		{
			// Stop using the default reconstruction layer proxy.
			d_using_default_reconstruction_layer_proxy = false;

			d_reconstruct_layer_proxy->set_current_reconstruction_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruction_layer_proxy.get()));
		}
	} 
	else if ( input_channel_name == LayerInputChannelName::TOPOLOGY_SURFACES)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - topological geometry resolver (for plate boundaries).
		// - topological network resolver.

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_current_resolved_boundary_topology_surface_layer_proxies.push_back(
					topological_boundary_resolver_layer_proxy.get());
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
					TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (topological_network_resolver_layer_proxy)
		{
			d_current_resolved_network_topology_surface_layer_proxies.push_back(
					topological_network_resolver_layer_proxy.get());
		}
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::remove_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTION_TREE)
	{
		// Make sure the input layer proxy is a reconstruction layer proxy.
		boost::optional<ReconstructionLayerProxy *> reconstruction_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructionLayerProxy>(layer_proxy);
		if (reconstruction_layer_proxy)
		{
			// Start using the default reconstruction layer proxy.
			d_using_default_reconstruction_layer_proxy = true;

			d_reconstruct_layer_proxy->set_current_reconstruction_layer_proxy(
					d_default_reconstruction_layer_proxy);
		}
	}
	else if ( input_channel_name == LayerInputChannelName::TOPOLOGY_SURFACES)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - topological geometry resolver (for plate boundaries).
		// - topological network resolver.

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_current_resolved_boundary_topology_surface_layer_proxies.erase(
					std::remove(
							d_current_resolved_boundary_topology_surface_layer_proxies.begin(),
							d_current_resolved_boundary_topology_surface_layer_proxies.end(),
							topological_boundary_resolver_layer_proxy.get()),
					d_current_resolved_boundary_topology_surface_layer_proxies.end());
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (topological_network_resolver_layer_proxy)
		{
			d_current_resolved_network_topology_surface_layer_proxies.erase(
					std::remove(
							d_current_resolved_network_topology_surface_layer_proxies.begin(),
							d_current_resolved_network_topology_surface_layer_proxies.end(),
							topological_network_resolver_layer_proxy.get()),
					d_current_resolved_network_topology_surface_layer_proxies.end());
		}
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_reconstruct_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());

	// If our layer proxy is currently using the default reconstruction layer proxy then
	// tell our layer proxy about the new default reconstruction layer proxy.
	if (d_using_default_reconstruction_layer_proxy)
	{
		// Avoid setting it every update unless it's actually a different layer.
		if (reconstruction->get_default_reconstruction_layer_output() != d_default_reconstruction_layer_proxy)
		{
			d_reconstruct_layer_proxy->set_current_reconstruction_layer_proxy(
					reconstruction->get_default_reconstruction_layer_output());
		}
	}

	d_default_reconstruction_layer_proxy = reconstruction->get_default_reconstruction_layer_output();


	// Get the 'resolved boundary' topology surface layers.
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> resolved_boundary_topology_surface_layer_proxies;
	get_resolved_boundary_topology_surface_layer_proxies(
			resolved_boundary_topology_surface_layer_proxies,
			reconstruction);

	// Get the 'resolved network' topology surface layers.
	std::vector<TopologyNetworkResolverLayerProxy::non_null_ptr_type> resolved_network_topology_surface_layer_proxies;
	get_resolved_network_topology_surface_layer_proxies(
			resolved_network_topology_surface_layer_proxies,
			reconstruction);

	// Notify our layer proxy of the topology surface layer proxies.
	d_reconstruct_layer_proxy->set_current_topology_surface_layer_proxies(
			resolved_boundary_topology_surface_layer_proxies,
			resolved_network_topology_surface_layer_proxies);
}


bool
GPlatesAppLogic::ReconstructLayerTask::connected_to_topology_surface_layers() const
{
	// If any topology surface layers are connected...
	return !d_current_resolved_boundary_topology_surface_layer_proxies.empty() ||
		!d_current_resolved_network_topology_surface_layer_proxies.empty();
}


void
GPlatesAppLogic::ReconstructLayerTask::get_resolved_boundary_topology_surface_layer_proxies(
		std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &resolved_boundary_topology_surface_layer_proxies,
		const Reconstruction::non_null_ptr_type &reconstruction) const
{
	if (connected_to_topology_surface_layers())
	{
		// Restrict the topology surface layers to only those that are currently connected.
		resolved_boundary_topology_surface_layer_proxies = d_current_resolved_boundary_topology_surface_layer_proxies;
	}
	else
	{
		// Find those layer outputs that come from a topological geometry layer.
		// These will be our topology surface layer proxies that generate resolved topological *boundaries*.
		// NOTE: We reference all active topological geometry layers because we don't know which ones affect
		// the geometries we are reconstructing.
		reconstruction->get_active_layer_outputs<TopologyGeometryResolverLayerProxy>(
				resolved_boundary_topology_surface_layer_proxies);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::get_resolved_network_topology_surface_layer_proxies(
		std::vector<TopologyNetworkResolverLayerProxy::non_null_ptr_type> &resolved_network_topology_surface_layer_proxies,
		const Reconstruction::non_null_ptr_type &reconstruction) const
{
	if (connected_to_topology_surface_layers())
	{
		// Restrict the topology surface layers to only those that are currently connected.
		resolved_network_topology_surface_layer_proxies = d_current_resolved_network_topology_surface_layer_proxies;
	}
	else
	{
		// Find those layer outputs that come from a topological network layer.
		// These will be our topology surface layer proxies that generate resolved topological *networks*.
		// NOTE: We reference all active topological network layers because we don't know which ones affect
		// the geometries we are reconstructing.
		reconstruction->get_active_layer_outputs<TopologyNetworkResolverLayerProxy>(
				resolved_network_topology_surface_layer_proxies);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::handle_reconstruct_params_modified(
		ReconstructLayerParams &layer_params)
{
	// Update our reconstruct layer proxy.
	d_reconstruct_layer_proxy->set_current_reconstruct_params(layer_params.get_reconstruct_params());
}
