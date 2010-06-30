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

#include "ReconstructionLayerTask.h"

#include "ReconstructUtils.h"


const char *GPlatesAppLogic::ReconstructionLayerTask::RECONSTRUCTION_FEATURES_CHANNEL_NAME =
		"reconstruction features";


std::pair<QString, QString>
GPlatesAppLogic::ReconstructionLayerTask::get_name_and_description()
{
	return std::make_pair(
		"Reconstruction Tree Generator",

		"A plate-reconstruction hierarchy of total reconstruction poles "
		"which can be used to reconstruct geometries in other layers");
}


bool
GPlatesAppLogic::ReconstructionLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return ReconstructUtils::has_reconstruction_features(feature_collection);
}


std::vector<GPlatesAppLogic::Layer::input_channel_definition_type>
GPlatesAppLogic::ReconstructionLayerTask::get_input_channel_definitions() const
{
	std::vector<Layer::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction features.
	input_channel_definitions.push_back(
			boost::make_tuple(
					RECONSTRUCTION_FEATURES_CHANNEL_NAME,
					Layer::INPUT_FEATURE_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));
	
	return input_channel_definitions;
}


QString
GPlatesAppLogic::ReconstructionLayerTask::get_main_input_feature_collection_channel() const
{
	return RECONSTRUCTION_FEATURES_CHANNEL_NAME;
}


GPlatesAppLogic::Layer::LayerOutputDataType
GPlatesAppLogic::ReconstructionLayerTask::get_output_definition() const
{
	return Layer::OUTPUT_RECONSTRUCTION_TREE_DATA;
}


boost::optional<GPlatesAppLogic::layer_task_data_type>
GPlatesAppLogic::ReconstructionLayerTask::process(
		const input_data_type &input_data,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		const ReconstructionTree::non_null_ptr_to_const_type &/*default_reconstruction_tree*/)
{
	//
	// Get the reconstruction features collection input.
	//
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstruction_features_collection;
	extract_input_channel_data(
			reconstruction_features_collection,
			RECONSTRUCTION_FEATURES_CHANNEL_NAME,
			input_data);

	//
	// Create the reconstruction tree.
	//
	const ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			ReconstructUtils::create_reconstruction_tree(
					reconstruction_time,
					anchored_plate_id,
					reconstruction_features_collection);

	// Return the reconstruction tree.
	return layer_task_data_type(reconstruction_tree);
}
