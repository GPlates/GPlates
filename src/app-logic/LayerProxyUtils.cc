/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#include <vector>
#include <boost/foreach.hpp>

#include "Layer.h"
#include "LayerProxyUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedFeatureGeometryFinder.h"
#include "ReconstructGraph.h"
#include "Reconstruction.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalLine.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"
#include "TopologyUtils.h"


void
GPlatesAppLogic::LayerProxyUtils::get_reconstructed_feature_geometries(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		std::vector<ReconstructHandle::type> &reconstruct_handles,
		const Reconstruction &reconstruction,
		bool include_topology_reconstructed_feature_geometries)
{
	// Get the reconstruct layer outputs.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_proxies;
	reconstruction.get_active_layer_outputs<ReconstructLayerProxy>(reconstruct_layer_proxies);

	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstruct_layer_proxy,
			reconstruct_layer_proxies)
	{
		// Skip topology reconstructed feature geometries if requested.
		if (!include_topology_reconstructed_feature_geometries &&
			reconstruct_layer_proxy->using_topologies_to_reconstruct())
		{
			continue;
		}

		// Get the reconstructed feature geometries from the current layer for the
		// current reconstruction time and anchor plate id.
		const ReconstructHandle::type reconstruct_handle =
				reconstruct_layer_proxy->get_reconstructed_feature_geometries(reconstructed_feature_geometries);

		// Add the reconstruct handle to the list.
		reconstruct_handles.push_back(reconstruct_handle);
	}
}


void
GPlatesAppLogic::LayerProxyUtils::get_resolved_topological_lines(
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
		std::vector<ReconstructHandle::type> &reconstruct_handles,
		const Reconstruction &reconstruction)
{
	// Get the resolved geometry layer outputs.
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> topology_geometry_resolver_layer_proxies;
	reconstruction.get_active_layer_outputs<TopologyGeometryResolverLayerProxy>(topology_geometry_resolver_layer_proxies);

	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topology_geometry_resolver_layer_proxy,
			topology_geometry_resolver_layer_proxies)
	{
		// Get the resolved topological lines from the current layer.
		const ReconstructHandle::type reconstruct_handle =
				topology_geometry_resolver_layer_proxy->get_resolved_topological_lines(resolved_topological_lines);

		// Add the reconstruct handle to the list.
		reconstruct_handles.push_back(reconstruct_handle);
	}
}


void
GPlatesAppLogic::LayerProxyUtils::find_dependent_topological_sections(
		std::set<GPlatesModel::FeatureId> &dependent_topological_sections,
		const Reconstruction &reconstruction)
{
	dependent_topological_sections.clear();

	// Get the resolved geometry layer outputs.
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> topology_geometry_resolver_layer_proxies;
	reconstruction.get_active_layer_outputs<TopologyGeometryResolverLayerProxy>(topology_geometry_resolver_layer_proxies);

	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topology_geometry_resolver_layer_proxy,
			topology_geometry_resolver_layer_proxies)
	{
		// Get the dependent topological sections from the current layer.
		topology_geometry_resolver_layer_proxy->get_current_dependent_topological_sections(
				dependent_topological_sections/*resolved_boundary_dependent_topological_sections*/,
				dependent_topological_sections/*resolved_line_dependent_topological_sections*/);
	}


	// Get the resolved network layer outputs.
	std::vector<TopologyNetworkResolverLayerProxy::non_null_ptr_type> topology_network_resolver_layer_proxies;
	reconstruction.get_active_layer_outputs<TopologyNetworkResolverLayerProxy>(topology_network_resolver_layer_proxies);

	BOOST_FOREACH(
			const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topology_network_resolver_layer_proxy,
			topology_network_resolver_layer_proxies)
	{
		// Get the dependent topological sections from the current layer.
		topology_network_resolver_layer_proxy->get_current_dependent_topological_sections(dependent_topological_sections);
	}
}


void
GPlatesAppLogic::LayerProxyUtils::find_resolved_topological_sections(
		std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
		const Reconstruction &reconstruction)
{
	resolved_topological_sections.clear();

	// Get the resolved geometry layer outputs.
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> topology_geometry_resolver_layer_proxies;
	reconstruction.get_active_layer_outputs<TopologyGeometryResolverLayerProxy>(topology_geometry_resolver_layer_proxies);

	// Get the resolved topological boundaries.
	std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;
	for (auto topology_geometry_resolver_layer_proxy : topology_geometry_resolver_layer_proxies)
	{
		topology_geometry_resolver_layer_proxy->get_resolved_topological_boundaries(
				resolved_topological_boundaries,
				reconstruction.get_reconstruction_time());
	}


	// Get the resolved network layer outputs.
	std::vector<TopologyNetworkResolverLayerProxy::non_null_ptr_type> topology_network_resolver_layer_proxies;
	reconstruction.get_active_layer_outputs<TopologyNetworkResolverLayerProxy>(topology_network_resolver_layer_proxies);

	// Get the resolved topological networks.
	std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
	for (auto topology_network_resolver_layer_proxy : topology_network_resolver_layer_proxies)
	{
		topology_network_resolver_layer_proxy->get_resolved_topological_networks(
				resolved_topological_networks,
				reconstruction.get_reconstruction_time());
	}

	// Find the resolved topological sections from the resolved topological boundaries and networks.
	TopologyUtils::find_resolved_topological_sections(
			resolved_topological_sections,
			std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type>(resolved_topological_boundaries.begin(), resolved_topological_boundaries.end()),
			std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type>(resolved_topological_networks.begin(), resolved_topological_networks.end()));
}


void
GPlatesAppLogic::LayerProxyUtils::find_reconstruct_layer_outputs_of_feature_collection(
		std::vector<ReconstructLayerProxy::non_null_ptr_type> &reconstruct_layer_outputs,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		const ReconstructGraph &reconstruct_graph)
{
	if (!feature_collection_ref.is_valid())
	{
		return;
	}

	// Check the input files of all active reconstruct layers.
	ReconstructGraph::const_iterator layers_iter = reconstruct_graph.begin();
	ReconstructGraph::const_iterator layers_end = reconstruct_graph.end();
	for ( ; layers_iter != layers_end ; ++layers_iter)
	{
		const Layer layer = *layers_iter;

		if (layer.is_active() &&
			layer.get_type() == LayerTaskType::RECONSTRUCT)
		{
			// The 'reconstruct geometries' layer has input feature collections on its main input channel.
			const LayerInputChannelName::Type main_input_channel = layer.get_main_input_feature_collection_channel();
			const std::vector<Layer::InputConnection> main_inputs = layer.get_channel_inputs(main_input_channel);

			// Loop over all input connections to get the files (feature collections) for the current target layer.
			BOOST_FOREACH(const Layer::InputConnection& main_input_connection, main_inputs)
			{
				boost::optional<Layer::InputFile> input_file = main_input_connection.get_input_file();

				// If it's not a file (ie, it's a layer) then continue to the next file.
				// For reconstruct layers it should be a file though.
				if(!input_file)
				{
					continue;
				}

				// Check if the input feature collection matches ours.
				if (input_file->get_feature_collection() != feature_collection_ref)
				{
					continue;
				}

				boost::optional<ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_output =
						layer.get_layer_output<ReconstructLayerProxy>();
				if (!reconstruct_layer_output)
				{
					continue;
				}

				reconstruct_layer_outputs.push_back(reconstruct_layer_output.get());
				break;
			}
		}
	}
}


void
GPlatesAppLogic::LayerProxyUtils::find_reconstruct_layer_outputs_of_feature(
		std::vector<ReconstructLayerProxy::non_null_ptr_type> &reconstruct_layer_outputs,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const Reconstruction &reconstruction)
{
	if (!feature_ref.is_valid())
	{
		return;
	}

	// Get the reconstruct layer outputs.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> all_reconstruct_layer_proxies;
	reconstruction.get_active_layer_outputs<ReconstructLayerProxy>(all_reconstruct_layer_proxies);
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstruct_layer_proxy,
			all_reconstruct_layer_proxies)
	{
		// Get all features reconstructed by the current reconstruct layer.
		//
		// Note that we only consider non-topological features since a feature collection may contain a mixture
		// of topological and non-topological (thus creating reconstruct layer and topological layer).
		std::vector<GPlatesModel::FeatureHandle::weak_ref> features;
		reconstruct_layer_proxy->get_current_reconstructable_features(features);

		// See if any features match our feature.
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator features_iter = features.begin();
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator features_end = features.end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			if (feature_ref == *features_iter)
			{
				reconstruct_layer_outputs.push_back(reconstruct_layer_proxy);
				break;
			}
		}
	}
}


void
GPlatesAppLogic::LayerProxyUtils::find_reconstructed_feature_geometries_of_feature(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const Reconstruction &reconstruction)
{
	if (!feature_ref.is_valid())
	{
		return;
	}

	// Get the RFGs (and generate if not already) for all active ReconstructLayer's
	// that reconstruct our feature.
	// This is needed because we are about to search the feature for its RFGs and
	// if we don't generate the RFGs (and no one else does either) then they might not be found.
	//
	// The RFGs - we'll keep them active until we've finished searching for RFGs below.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> candidate_rfgs;
	std::vector<ReconstructHandle::type> reconstruct_handles;

	// Get the reconstruct layer outputs that reconstruct our feature.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_proxies;
	find_reconstruct_layer_outputs_of_feature(reconstruct_layer_proxies, feature_ref, reconstruction);

	// Get the RFGs from the layer outputs that reconstruct our feature.
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstruct_layer_proxy,
			reconstruct_layer_proxies)
	{
		// Get the reconstructed feature geometries from the current layer for the
		// current reconstruction time and anchor plate id.
		reconstruct_handles.push_back(
				reconstruct_layer_proxy->get_reconstructed_feature_geometries(candidate_rfgs));
	}

	// Iterate through all RFGs observing 'feature_ref' and that were reconstructed just now (above).
	ReconstructedFeatureGeometryFinder rfg_finder(
			boost::none/*reconstruction_tree_to_match*/,
			reconstruct_handles);
	rfg_finder.find_rfgs_of_feature(feature_ref);

	// Add the found RFGs to the caller's sequence.
	reconstructed_feature_geometries.insert(
			reconstructed_feature_geometries.end(),
			rfg_finder.found_rfgs_begin(),
			rfg_finder.found_rfgs_end());
}
