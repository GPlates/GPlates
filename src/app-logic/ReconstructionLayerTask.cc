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

#include "ApplicationState.h"
#include "ClassifyFeatureCollection.h"
#include "ReconstructUtils.h"


const char *GPlatesAppLogic::ReconstructionLayerTask::RECONSTRUCTION_FEATURES_CHANNEL_NAME =
		"reconstruction features";


bool
GPlatesAppLogic::ReconstructionLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	ClassifyFeatureCollection::classifications_type classification =
			ClassifyFeatureCollection::classify_feature_collection(feature_collection);

	// We're interested in a file if it contains reconstruction features.
	return classification.test(ClassifyFeatureCollection::RECONSTRUCTION);
}


GPlatesAppLogic::ReconstructionLayerTask::ReconstructionLayerTask(
		const ApplicationState &application_state) :
	d_application_state(application_state)
{
}


std::vector<GPlatesAppLogic::ReconstructGraph::input_channel_definition_type>
GPlatesAppLogic::ReconstructionLayerTask::get_input_channel_definitions() const
{
	std::vector<ReconstructGraph::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction features.
	input_channel_definitions.push_back(
			boost::make_tuple(
					RECONSTRUCTION_FEATURES_CHANNEL_NAME,
					ReconstructGraph::FEATURE_COLLECTION_DATA,
					ReconstructGraph::MULTIPLE_DATAS_IN_CHANNEL));
	
	return input_channel_definitions;
}


GPlatesAppLogic::ReconstructGraph::DataType
GPlatesAppLogic::ReconstructionLayerTask::get_output_definition() const
{
	return ReconstructGraph::RECONSTRUCTION_TREE_DATA;
}


bool
GPlatesAppLogic::ReconstructionLayerTask::process(
		const input_data_type &input_data,
		layer_data_type &output_data,
		const double &reconstruction_time)
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
	output_data = ReconstructUtils::create_reconstruction_tree(
			reconstruction_time,
			d_application_state.get_current_anchored_plate_id(),
			reconstruction_features_collection);

	return true;
}
