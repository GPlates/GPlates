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


const char *GPlatesAppLogic::ReconstructLayerTask::RECONSTRUCTABLE_FEATURES_CHANNEL_NAME =
		"reconstructable features";


std::pair<QString, QString>
GPlatesAppLogic::ReconstructLayerTask::get_name_and_description()
{
	return std::make_pair(
		"Geometry Reconstruction",

		"Geometries in this layer will be reconstructed when "
		"this layer is connected to a reconstruction tree layer");
}


bool
GPlatesAppLogic::ReconstructLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return ReconstructUtils::has_reconstructable_features(feature_collection);
}


std::vector<GPlatesAppLogic::Layer::input_channel_definition_type>
GPlatesAppLogic::ReconstructLayerTask::get_input_channel_definitions() const
{
	std::vector<Layer::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction tree.
	input_channel_definitions.push_back(
			boost::make_tuple(
					get_reconstruction_tree_channel_name(),
					Layer::INPUT_RECONSTRUCTION_TREE_DATA,
					Layer::ONE_DATA_IN_CHANNEL));

	// Channel definition for the reconstructable features.
	input_channel_definitions.push_back(
			boost::make_tuple(
					RECONSTRUCTABLE_FEATURES_CHANNEL_NAME,
					Layer::INPUT_FEATURE_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));
	
	return input_channel_definitions;
}


QString
GPlatesAppLogic::ReconstructLayerTask::get_main_input_feature_collection_channel() const
{
	return RECONSTRUCTABLE_FEATURES_CHANNEL_NAME;
}


GPlatesAppLogic::Layer::LayerOutputDataType
GPlatesAppLogic::ReconstructLayerTask::get_output_definition() const
{
	return Layer::OUTPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA;
}


boost::optional<GPlatesAppLogic::layer_task_data_type>
GPlatesAppLogic::ReconstructLayerTask::process(
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
	const ReconstructionGeometryCollection::non_null_ptr_to_const_type
			reconstruction_geometry_collection =
					ReconstructUtils::reconstruct(
							reconstruction_tree.get(),
							reconstructable_features_collection);

	// Return the reconstruction geometry collection.
	return layer_task_data_type(reconstruction_geometry_collection);
}
