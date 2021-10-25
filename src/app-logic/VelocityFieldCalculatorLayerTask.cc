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
#include "PlateVelocityUtils.h"
#include "VelocityFieldCalculatorLayerProxy.h"


const QString GPlatesAppLogic::VelocityFieldCalculatorLayerTask::VELOCITY_DOMAIN_LAYERS_CHANNEL_NAME =
		"Velocity domains (points/multi-points/polylines/polygons)";

const QString GPlatesAppLogic::VelocityFieldCalculatorLayerTask::VELOCITY_SURFACE_LAYERS_CHANNEL_NAME =
		"Velocity surfaces (static/dynamic polygons/networks)";


GPlatesAppLogic::VelocityFieldCalculatorLayerTask::VelocityFieldCalculatorLayerTask() :
	d_velocity_field_calculator_layer_proxy(
			VelocityFieldCalculatorLayerProxy::create())
{
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
	// non-topological geometries (points, multi-points, polylines and polygons).
	std::vector<LayerTaskType::Type> domain_input_channel_types;
	domain_input_channel_types.push_back(LayerTaskType::RECONSTRUCT);
	input_channel_types.push_back(
			LayerInputChannelType(
					VELOCITY_DOMAIN_LAYERS_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					domain_input_channel_types));

	// Channel definition for the surfaces on which to calculate velocities:
	// - reconstructed static polygons, or
	// - resolved topological dynamic polygons, or
	// - resolved topological networks.
	std::vector<LayerTaskType::Type> surfaces_input_channel_types;
	surfaces_input_channel_types.push_back(LayerTaskType::RECONSTRUCT);
	surfaces_input_channel_types.push_back(LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER);
	surfaces_input_channel_types.push_back(LayerTaskType::TOPOLOGY_NETWORK_RESOLVER);
	input_channel_types.push_back(
			LayerInputChannelType(
					VELOCITY_SURFACE_LAYERS_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					surfaces_input_channel_types));
	
	return input_channel_types;
}


QString
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_main_input_feature_collection_channel() const
{
	// The main input feature collection channel is not used because we only accept
	// input from other layers - so this string should never be seen by users.
	return QString("Unused Input File Channel");
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::remove_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::modified_input_file(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::add_input_layer_proxy_connection(
		const QString &input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == VELOCITY_DOMAIN_LAYERS_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_velocity_domain_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}
	}
	else if (input_channel_name == VELOCITY_SURFACE_LAYERS_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological geometry resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_topological_boundary_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_boundary_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (topological_network_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_network_resolver_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::remove_input_layer_proxy_connection(
		const QString &input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == VELOCITY_DOMAIN_LAYERS_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_velocity_domain_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}
	}
	else if (input_channel_name == VELOCITY_SURFACE_LAYERS_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological geometry resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_topological_boundary_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_boundary_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (topological_network_resolver_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_network_resolver_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_velocity_field_calculator_layer_proxy->set_current_reconstruction_time(
			reconstruction->get_reconstruction_time());

	// If the layer task params have been modified then update our velocity layer proxy.
	if (d_layer_task_params.d_set_velocity_params_called)
	{
		d_velocity_field_calculator_layer_proxy->set_current_velocity_params(
				d_layer_task_params.d_velocity_params);

		d_layer_task_params.d_set_velocity_params_called = false;
	}
}


GPlatesAppLogic::LayerProxy::non_null_ptr_type
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_layer_proxy()
{
	return d_velocity_field_calculator_layer_proxy;
}


GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params::Params() :
	d_set_velocity_params_called(false)
{
}


const GPlatesAppLogic::VelocityParams &
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params::get_velocity_params() const
{
	return d_velocity_params;
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params::set_velocity_params(
		const VelocityParams &velocity_params)
{
	d_velocity_params = velocity_params;

	d_set_velocity_params_called = true;
	emit_modified();
}
