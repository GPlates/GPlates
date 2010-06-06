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


const char *GPlatesAppLogic::TopologyNetworkResolverLayerTask::TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME =
		"topological network features";


std::pair<QString, QString>
GPlatesAppLogic::TopologyNetworkResolverLayerTask::get_name_and_description()
{
	return std::make_pair(
		"Resolve topological networks at a geological time",

		"Deforming regions will be dynamically formed by referencing topological section "
		"features, that have been reconstructed to a geological time, and triangulating the "
		"convex hull region defined by these reconstructed sections while excluding any "
		"micro-block sections from the triangulation");
}


bool
GPlatesAppLogic::TopologyNetworkResolverLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return TopologyUtils::has_topological_network_features(feature_collection);
}


std::vector<GPlatesAppLogic::Layer::input_channel_definition_type>
GPlatesAppLogic::TopologyNetworkResolverLayerTask::get_input_channel_definitions() const
{
	std::vector<Layer::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction tree.
	input_channel_definitions.push_back(
			boost::make_tuple(
					get_reconstruction_tree_channel_name(),
					Layer::INPUT_RECONSTRUCTION_TREE_DATA,
					Layer::ONE_DATA_IN_CHANNEL));

	// Channel definition for the topological network features.
	input_channel_definitions.push_back(
			boost::make_tuple(
					TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME,
					Layer::INPUT_FEATURE_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));

	// There is a third input - the reconstructed topological section geometries referenced by
	// the topological networks.
	// But we don't need an input channel for them because:
	//
	// The referenced reconstructed feature geometries that comprise the topological sections
	// will be obtained by referencing the weak observers of those referenced features
	// (ReconstructedFeatureGeometry is a weak observer of a feature).
	// And this requires no special input channel (since we can just get the reconstructed
	// feature geometries directly from the topological section feature themselves provided
	// they've already been reconstructed).
	// We will however restrict our search of those reconstructed geometries to only those
	// that were reconstructed with the above ReconstructionTree. This allows the possibility
	// of having more than one set of topological networks, each using a different
	// rotation model (ReconstructionTree) - for example for visual comparison on the globe.

	return input_channel_definitions;
}


QString
GPlatesAppLogic::TopologyNetworkResolverLayerTask::get_main_input_feature_collection_channel() const
{
	return TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME;
}


GPlatesAppLogic::Layer::LayerOutputDataType
GPlatesAppLogic::TopologyNetworkResolverLayerTask::get_output_definition() const
{
	return Layer::OUTPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA;
}


boost::optional<GPlatesAppLogic::layer_task_data_type>
GPlatesAppLogic::TopologyNetworkResolverLayerTask::process(
		const input_data_type &input_data,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree)
{
	//
	// Get the reconstruction tree input.
	//
	boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree =
			extract_reconstruction_tree(
					input_data,
					default_reconstruction_tree);
	if (!reconstruction_tree)
	{
		// Expecting a single reconstruction tree.
		return boost::none;
	}
	
	//
	// Get the topological network features collection input.
	//
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> topological_network_features_collection;
	extract_input_channel_data(
			topological_network_features_collection,
			TOPOLOGICAL_NETWORK_FEATURES_CHANNEL_NAME,
			input_data);

	//
	// Resolve the topological networks.
	//
	// The reconstruction tree is used to limit our search for reconstructed topological
	// section geometries to those that were reconstructed with 'reconstruction_tree'.
	//
	const ReconstructionGeometryCollection::non_null_ptr_to_const_type
			resolved_topology_networks_geometry_collection =
					TopologyUtils::resolve_topological_networks(
							reconstruction_tree.get(),
							topological_network_features_collection);

	// Return the resolved topology networks.
	return layer_task_data_type(resolved_topology_networks_geometry_collection);
}
