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
#include <boost/bind.hpp>

#include "TopologyGeometryResolverLayerTask.h"

#include "AppLogicUtils.h"
#include "LayerProxyUtils.h"
#include "ReconstructUtils.h"
#include "TopologyUtils.h"


bool
GPlatesAppLogic::TopologyGeometryResolverLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return TopologyUtils::has_topological_boundary_features(feature_collection) ||
		TopologyUtils::has_topological_line_features(feature_collection);
}


GPlatesAppLogic::TopologyGeometryResolverLayerTask::~TopologyGeometryResolverLayerTask()
{
	// One of the topological section input layers is actually this layer (since topological
	// boundaries, in this layer, can depend on topological lines, also in this layer).
	// This can lead to a cyclic non_null_ptr dependency (can't destroy layer proxy because
	// internally it has a non_null_ptr to itself).
	// To avoid this we'll first remove all topological section input layers.
	//
	// Note that this is actually already taken care of by our 'activate()' function since
	// 'activate(false)' gets called when a layer is removed.
	// But we'll also remove the input layers here in case that changes.
	d_topology_geometry_resolver_layer_proxy->set_current_topological_sections_layer_proxies(
			std::vector<ReconstructLayerProxy::non_null_ptr_type>(),
			std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type>());
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::TopologyGeometryResolverLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the reconstruction tree.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::RECONSTRUCTION_TREE,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RECONSTRUCTION));

	// Channel definition for the topological geometry features.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL));

	// Channel definition for the topological section layers.
	// - reconstructed geometries, or
	// - resolved topological lines.
	//
	// The referenced reconstructed topological section geometries are obtained by referencing
	// the weak observers of referenced features (ReconstructedFeatureGeometry is a weak observer
	// of a feature). By default, if there are no connections on this channel, this is a global
	// search through all loaded features. However if there are any connections then the global
	// search is restricted to reconstructed geometries and resolved topological lines that
	// generated by the connected layers..
	std::vector<LayerTaskType::Type> topological_section_channel_types;
	topological_section_channel_types.push_back(LayerTaskType::RECONSTRUCT);
	topological_section_channel_types.push_back(LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER);
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::TOPOLOGICAL_SECTION_LAYERS,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					topological_section_channel_types));

	return input_channel_types;
}


GPlatesAppLogic::LayerInputChannelName::Type
GPlatesAppLogic::TopologyGeometryResolverLayerTask::get_main_input_feature_collection_channel() const
{
	return LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES;
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::activate(
		bool active)
{
	// If deactivated then specify an empty set of topological sections layer proxies.
	if (!active)
	{
		// Topological sections that are reconstructed static geometries.
		d_topology_geometry_resolver_layer_proxy->set_current_topological_sections_layer_proxies(
				std::vector<ReconstructLayerProxy::non_null_ptr_type>(),
				std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type>());
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::add_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES)
	{
		d_topology_geometry_resolver_layer_proxy
				->add_topological_geometry_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::remove_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES)
	{
		d_topology_geometry_resolver_layer_proxy
				->remove_topological_geometry_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::modified_input_file(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES)
	{
		// Let the reconstruct layer proxy know that one of the topological geometry feature collections has been modified.
		d_topology_geometry_resolver_layer_proxy
				->modified_topological_geometry_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::add_input_layer_proxy_connection(
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

			d_topology_geometry_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruction_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::TOPOLOGICAL_SECTION_LAYERS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological geometry resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_current_reconstructed_geometry_topological_sections_layer_proxies.push_back(
					reconstruct_layer_proxy.get());
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_current_resolved_line_topological_sections_layer_proxies.push_back(
					topological_boundary_resolver_layer_proxy.get());
		}
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::remove_input_layer_proxy_connection(
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

			d_topology_geometry_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					d_default_reconstruction_layer_proxy);
		}
	}
	else if (input_channel_name == LayerInputChannelName::TOPOLOGICAL_SECTION_LAYERS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological geometry resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_current_reconstructed_geometry_topological_sections_layer_proxies.erase(
					std::remove(
							d_current_reconstructed_geometry_topological_sections_layer_proxies.begin(),
							d_current_reconstructed_geometry_topological_sections_layer_proxies.end(),
							reconstruct_layer_proxy.get()),
					d_current_reconstructed_geometry_topological_sections_layer_proxies.end());
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_current_resolved_line_topological_sections_layer_proxies.erase(
					std::remove(
							d_current_resolved_line_topological_sections_layer_proxies.begin(),
							d_current_resolved_line_topological_sections_layer_proxies.end(),
							topological_boundary_resolver_layer_proxy.get()),
					d_current_resolved_line_topological_sections_layer_proxies.end());
		}
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_topology_geometry_resolver_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());

	// Get the 'reconstructed geometry' topological section layers.
	std::vector<ReconstructLayerProxy::non_null_ptr_type>
			reconstructed_geometry_topological_sections_layer_proxies;
	get_reconstructed_geometry_topological_sections_layer_proxies(
			reconstructed_geometry_topological_sections_layer_proxies,
			reconstruction);

	// Get the 'resolved line' topological section layers.
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type>
			resolved_line_topological_sections_layer_proxies;
	get_resolved_line_topological_sections_layer_proxies(
			resolved_line_topological_sections_layer_proxies,
			reconstruction);

	// Notify our layer proxy of the topological sections layer proxies.
	d_topology_geometry_resolver_layer_proxy->set_current_topological_sections_layer_proxies(
			reconstructed_geometry_topological_sections_layer_proxies,
			// NOTE: This actually also includes the layer proxy associated with 'this' layer since
			// topological boundaries can reference topological lines from the same layer...
			resolved_line_topological_sections_layer_proxies);

	// If our layer proxy is currently using the default reconstruction layer proxy then
	// tell our layer proxy about the new default reconstruction layer proxy.
	if (d_using_default_reconstruction_layer_proxy)
	{
		// Avoid setting it every update unless it's actually a different layer.
		if (reconstruction->get_default_reconstruction_layer_output() != d_default_reconstruction_layer_proxy)
		{
			d_topology_geometry_resolver_layer_proxy->set_current_reconstruction_layer_proxy(
					reconstruction->get_default_reconstruction_layer_output());
		}
	}

	d_default_reconstruction_layer_proxy = reconstruction->get_default_reconstruction_layer_output();
}


bool
GPlatesAppLogic::TopologyGeometryResolverLayerTask::connected_to_topological_section_layers() const
{
	// If any topological section layers are connected...
	return !d_current_reconstructed_geometry_topological_sections_layer_proxies.empty() ||
		!d_current_resolved_line_topological_sections_layer_proxies.empty();
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::get_reconstructed_geometry_topological_sections_layer_proxies(
		std::vector<ReconstructLayerProxy::non_null_ptr_type> &reconstructed_geometry_topological_sections_layer_proxies,
		const Reconstruction::non_null_ptr_type &reconstruction) const
{
	if (connected_to_topological_section_layers())
	{
		// Restrict the topological section layers to only those that are currently connected.
		reconstructed_geometry_topological_sections_layer_proxies =
				d_current_reconstructed_geometry_topological_sections_layer_proxies;
	}
	else
	{
		// Find those layer outputs that come from a reconstruct layer.
		// These will be our topological sections layer proxies that generate reconstructed static geometries.
		// NOTE: We reference all active reconstruct layers because we don't know which ones contain
		// the topological sections that our topologies are referencing (it's a global lookup).
		reconstruction->get_active_layer_outputs<ReconstructLayerProxy>(
				reconstructed_geometry_topological_sections_layer_proxies);
	}

	// Filter out reconstructed geometry layers that are connected (and hence deformed) by
	// topological network layers. These reconstructed geometry layers cannot supply
	// topological sections (to topological network layers) because these reconstructed geometries
	// are deformed by the topological networks which in turn would use the reconstructed geometries
	// to build the topological networks - thus creating a cyclic dependency.
	// Note that these reconstructed geometries also cannot supply topological sections to
	// topological 'geometry' layers, eg containing topological lines, because those resolved
	// topological lines can, in turn, be used as topological sections by topological networks -
	// so there's still a cyclic dependency (it's just a more round-about or indirect dependency).
	reconstructed_geometry_topological_sections_layer_proxies.erase(
			std::remove_if(
					reconstructed_geometry_topological_sections_layer_proxies.begin(),
					reconstructed_geometry_topological_sections_layer_proxies.end(),
					boost::bind(&ReconstructLayerProxy::connected_to_topological_layer_proxies, _1)),
			reconstructed_geometry_topological_sections_layer_proxies.end());
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerTask::get_resolved_line_topological_sections_layer_proxies(
		std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &resolved_line_topological_sections_layer_proxies,
		const Reconstruction::non_null_ptr_type &reconstruction) const
{
	if (connected_to_topological_section_layers())
	{
		// Restrict the topological section layers to only those that are currently connected.
		resolved_line_topological_sections_layer_proxies =
				d_current_resolved_line_topological_sections_layer_proxies;
	}
	else
	{
		// Find those layer outputs that come from a topological geometry layer.
		// These will be our topological sections layer proxies that generate resolved topological *lines*.
		// NOTE: We reference all active topological geometry layers because we don't know which ones contain
		// the topological sections that our topologies are referencing (it's a global lookup).
		reconstruction->get_active_layer_outputs<TopologyGeometryResolverLayerProxy>(
				resolved_line_topological_sections_layer_proxies);
	}
}
