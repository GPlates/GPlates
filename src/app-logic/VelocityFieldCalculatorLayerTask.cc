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

#include "VelocityFieldCalculatorLayerTask.h"

#include "AppLogicUtils.h"
#include "LayerProxyUtils.h"
#include "PlateVelocityUtils.h"
#include "VelocityFieldCalculatorLayerProxy.h"


GPlatesAppLogic::VelocityFieldCalculatorLayerTask::VelocityFieldCalculatorLayerTask() :
	d_layer_params(VelocityFieldCalculatorLayerParams::create()),
	d_velocity_field_calculator_layer_proxy(
			VelocityFieldCalculatorLayerProxy::create())
{
	// Notify our layer output whenever the layer params are modified.
	QObject::connect(
			d_layer_params.get(), SIGNAL(modified_velocity_params(GPlatesAppLogic::VelocityFieldCalculatorLayerParams &)),
			this, SLOT(handle_velocity_params_modified(GPlatesAppLogic::VelocityFieldCalculatorLayerParams &)));
}


bool
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return PlateVelocityUtils::detect_velocity_mesh_nodes(feature_collection);
}


boost::shared_ptr<GPlatesAppLogic::VelocityFieldCalculatorLayerTask>
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::create_layer_task()
{
	return boost::shared_ptr<VelocityFieldCalculatorLayerTask>(
			new VelocityFieldCalculatorLayerTask());
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for velocity domain geometries.
	// NOTE: Previously only accepted "MeshNode" features but now accept anything containing
	// non-topological geometries (points, multi-points, polylines and polygons), and now
	// even topological geometries and network boundaries.
	std::vector<LayerInputChannelType::InputLayerType> domain_input_channel_types;
	domain_input_channel_types.push_back(
			LayerInputChannelType::InputLayerType(
					LayerTaskType::RECONSTRUCT,
					// Auto-connect to the domain (local means associated with same input file)...
					LayerInputChannelType::LOCAL_AUTO_CONNECT));
	domain_input_channel_types.push_back(
			LayerInputChannelType::InputLayerType(
					LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER));
	domain_input_channel_types.push_back(
			LayerInputChannelType::InputLayerType(
					LayerTaskType::TOPOLOGY_NETWORK_RESOLVER));
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::VELOCITY_DOMAIN_LAYERS,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					domain_input_channel_types));

	// Channel definition for the surfaces on which to calculate velocities:
	// - reconstructed static polygons, or
	// - resolved topological dynamic polygons, or
	// - resolved topological networks.
	std::vector<LayerInputChannelType::InputLayerType> surfaces_input_channel_types;
	surfaces_input_channel_types.push_back(LayerTaskType::RECONSTRUCT);
	surfaces_input_channel_types.push_back(
			LayerInputChannelType::InputLayerType(
					LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER,
					// Auto connect to all TOPOLOGY_GEOMETRY_RESOLVER layers...
					LayerInputChannelType::GLOBAL_AUTO_CONNECT));
	surfaces_input_channel_types.push_back(
			LayerInputChannelType::InputLayerType(
					LayerTaskType::TOPOLOGY_NETWORK_RESOLVER,
					// Auto connect to all TOPOLOGY_NETWORK_RESOLVER layers...
					LayerInputChannelType::GLOBAL_AUTO_CONNECT));
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::VELOCITY_SURFACE_LAYERS,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					surfaces_input_channel_types));
	
	return input_channel_types;
}


GPlatesAppLogic::LayerInputChannelName::Type
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_main_input_feature_collection_channel() const
{
	// The main input feature collection channel is not used because we only accept input from other layers.
	return LayerInputChannelName::UNUSED;
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::add_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::remove_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::modified_input_file(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::add_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::VELOCITY_DOMAIN_LAYERS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct.

		boost::optional<ReconstructLayerProxy *> domain_reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (domain_reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_domain_reconstruct_layer_proxy(
					GPlatesUtils::get_non_null_pointer(domain_reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> domain_topological_geometry_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (domain_topological_geometry_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_domain_topological_geometry_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(domain_topological_geometry_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> domain_topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (domain_topological_network_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_domain_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(domain_topological_network_resolver_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::VELOCITY_SURFACE_LAYERS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological geometry resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> surface_reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (surface_reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_surface_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(surface_reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> surface_topological_geometry_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (surface_topological_geometry_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_surface_topological_geometry_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(surface_topological_geometry_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> surface_topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (surface_topological_network_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_surface_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(surface_topological_network_resolver_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::remove_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::VELOCITY_DOMAIN_LAYERS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct.

		boost::optional<ReconstructLayerProxy *> domain_reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (domain_reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_domain_reconstruct_layer_proxy(
					GPlatesUtils::get_non_null_pointer(domain_reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> domain_topological_geometry_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (domain_topological_geometry_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_domain_topological_geometry_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(domain_topological_geometry_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> domain_topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (domain_topological_network_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_domain_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(domain_topological_network_resolver_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::VELOCITY_SURFACE_LAYERS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological geometry resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> surface_reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (surface_reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_surface_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(surface_reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> surface_topological_geometry_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (surface_topological_geometry_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_surface_topological_geometry_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(surface_topological_geometry_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> surface_topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (surface_topological_network_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_surface_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(surface_topological_network_resolver_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_velocity_field_calculator_layer_proxy->set_current_reconstruction_time(
			reconstruction->get_reconstruction_time());
}


GPlatesAppLogic::LayerProxy::non_null_ptr_type
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_layer_proxy()
{
	return d_velocity_field_calculator_layer_proxy;
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::handle_velocity_params_modified(
		VelocityFieldCalculatorLayerParams &layer_params)
{
	// Update our velocity layer proxy.
	d_velocity_field_calculator_layer_proxy->set_current_velocity_params(layer_params.get_velocity_params());
}
