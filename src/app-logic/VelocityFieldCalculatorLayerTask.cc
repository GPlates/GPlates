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


const QString GPlatesAppLogic::VelocityFieldCalculatorLayerTask::VELOCITY_DOMAIN_FEATURES_CHANNEL_NAME =
		"Multi-point features";

const QString GPlatesAppLogic::VelocityFieldCalculatorLayerTask::RECONSTRUCTED_STATIC_DYNAMIC_POLYGONS_NETWORKS_CHANNEL_NAME =
		"Reconstructed static/dynamic polygons/networks";


bool
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return PlateVelocityUtils::detect_velocity_mesh_nodes(feature_collection);
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the reconstruction tree.
	input_channel_types.push_back(
			LayerInputChannelType(
					get_reconstruction_tree_channel_name(),
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RECONSTRUCTION));

	// Channel definition for the surfaces on which to calculate velocities:
	// - reconstructed static polygons, or
	// - resolved topological dynamic polygons, or
	// - resolved topological networks.
	std::vector<LayerTaskType::Type> surfaces_input_channel_types;
	surfaces_input_channel_types.push_back(LayerTaskType::RECONSTRUCT);
	surfaces_input_channel_types.push_back(LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER);
	surfaces_input_channel_types.push_back(LayerTaskType::TOPOLOGY_NETWORK_RESOLVER);
	input_channel_types.push_back(
			LayerInputChannelType(
					RECONSTRUCTED_STATIC_DYNAMIC_POLYGONS_NETWORKS_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					surfaces_input_channel_types));

	// Channel definition for the multi-point features.
	// Any features really, just expecting features containing multi-point geometries.
	// NOTE: Previously only accepted "MeshNode" features but now accept anything that
	// 
	input_channel_types.push_back(
			LayerInputChannelType(
					VELOCITY_DOMAIN_FEATURES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL));
	
	return input_channel_types;
}


QString
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_main_input_feature_collection_channel() const
{
	return VELOCITY_DOMAIN_FEATURES_CHANNEL_NAME;
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == VELOCITY_DOMAIN_FEATURES_CHANNEL_NAME)
	{
		d_velocity_field_calculator_layer_proxy
				->add_multi_point_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::remove_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == VELOCITY_DOMAIN_FEATURES_CHANNEL_NAME)
	{
		d_velocity_field_calculator_layer_proxy
				->remove_multi_point_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::modified_input_file(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == VELOCITY_DOMAIN_FEATURES_CHANNEL_NAME)
	{
		// Let the velocity layer proxy know that one of the feature collections
		// containing multi-points has been modified.
		d_velocity_field_calculator_layer_proxy
				->modified_multi_point_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::add_input_layer_proxy_connection(
		const QString &input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == get_reconstruction_tree_channel_name())
	{
		// Make sure the input layer proxy is a reconstruction layer proxy.
		boost::optional<ReconstructionLayerProxy *> reconstruction_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructionLayerProxy>(layer_proxy);
		if (reconstruction_layer_proxy)
		{
			// Stop using the default reconstruction layer proxy.
			d_using_default_reconstruction_layer_proxy = false;

			d_velocity_field_calculator_layer_proxy->set_current_reconstruction_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruction_layer_proxy.get()));
		}
	}
	else if (input_channel_name == RECONSTRUCTED_STATIC_DYNAMIC_POLYGONS_NETWORKS_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological boundary resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->add_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyBoundaryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyBoundaryResolverLayerProxy>(layer_proxy);
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

			d_velocity_field_calculator_layer_proxy->set_current_reconstruction_layer_proxy(
					d_default_reconstruction_layer_proxy);
		}
	}
	else if (input_channel_name == RECONSTRUCTED_STATIC_DYNAMIC_POLYGONS_NETWORKS_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological boundary resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->remove_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyBoundaryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyBoundaryResolverLayerProxy>(layer_proxy);
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
	d_velocity_field_calculator_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());

	// If our layer proxy is currently using the default reconstruction layer proxy then
	// tell our layer proxy about the new default reconstruction layer proxy.
	if (d_using_default_reconstruction_layer_proxy)
	{
		// Avoid setting it every update unless it's actually a different layer.
		if (reconstruction->get_default_reconstruction_layer_output() != d_default_reconstruction_layer_proxy)
		{
			d_velocity_field_calculator_layer_proxy->set_current_reconstruction_layer_proxy(
					reconstruction->get_default_reconstruction_layer_output());
		}
	}

	d_default_reconstruction_layer_proxy = reconstruction->get_default_reconstruction_layer_output();
}
