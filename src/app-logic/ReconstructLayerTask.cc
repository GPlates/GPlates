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

#include "ReconstructLayerTask.h"

#include "AppLogicUtils.h"
#include "ReconstructUtils.h"

#include "feature-visitors/GeometryTypeFinder.h"


const char *GPlatesAppLogic::ReconstructLayerTask::RECONSTRUCTION_TREE_CHANNEL_NAME =
		"reconstruction tree";
const char *GPlatesAppLogic::ReconstructLayerTask::RECONSTRUCTABLE_FEATURES_CHANNEL_NAME =
		"reconstructable features";


bool
GPlatesAppLogic::ReconstructLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// If there are any features containing geometry then we can process them.
	GPlatesFeatureVisitors::GeometryTypeFinder geometry_type_finder;

	AppLogicUtils::visit_feature_collection(feature_collection, geometry_type_finder);

	return geometry_type_finder.has_found_geometries();
}


std::vector<GPlatesAppLogic::ReconstructGraph::input_channel_definition_type>
GPlatesAppLogic::ReconstructLayerTask::get_input_channel_definitions() const
{
	std::vector<ReconstructGraph::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction tree.
	input_channel_definitions.push_back(
			boost::make_tuple(
					RECONSTRUCTION_TREE_CHANNEL_NAME,
					ReconstructGraph::RECONSTRUCTION_TREE_DATA,
					ReconstructGraph::ONE_DATA_IN_CHANNEL));

	// Channel definition for the reconstructable features.
	input_channel_definitions.push_back(
			boost::make_tuple(
					RECONSTRUCTABLE_FEATURES_CHANNEL_NAME,
					ReconstructGraph::FEATURE_COLLECTION_DATA,
					ReconstructGraph::MULTIPLE_DATAS_IN_CHANNEL));
	
	return input_channel_definitions;
}


GPlatesAppLogic::ReconstructGraph::DataType
GPlatesAppLogic::ReconstructLayerTask::get_output_definition() const
{
	return ReconstructGraph::RECONSTRUCTED_GEOMETRY_COLLECTION_DATA;
}


bool
GPlatesAppLogic::ReconstructLayerTask::process(
		const input_data_type &input_data,
		layer_data_type &output_data,
		const double &/*reconstruction_time*/)
{
	//
	// Get the reconstruction tree input.
	//
	std::vector<GPlatesModel::ReconstructionTree::non_null_ptr_type> reconstruction_trees;
	extract_input_channel_data(
			reconstruction_trees,
			RECONSTRUCTION_TREE_CHANNEL_NAME,
			input_data);

	if (reconstruction_trees.size() != 1)
	{
		// Expecting a single reconstruction tree.
		return false;
	}
	const GPlatesModel::ReconstructionTree::non_null_ptr_type reconstruction_tree =
			reconstruction_trees.front();
	
	//
	// Get the reconstructable features collection input.
	//
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstructable_features_collection;
	extract_input_channel_data(
			reconstructable_features_collection,
			RECONSTRUCTABLE_FEATURES_CHANNEL_NAME,
			input_data);

	//
	// Perform the actual reconstruction using the reconstruction tree.
	//
	output_data = ReconstructUtils::create_reconstruction(
			reconstruction_tree,
			reconstructable_features_collection);

	return true;
}
