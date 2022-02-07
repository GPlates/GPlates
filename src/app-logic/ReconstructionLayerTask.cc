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


bool
GPlatesAppLogic::ReconstructionLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return ReconstructUtils::has_reconstruction_features(feature_collection);
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::ReconstructionLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the reconstruction features.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::RECONSTRUCTION_FEATURES,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL));

	return input_channel_types;
}


GPlatesAppLogic::LayerInputChannelName::Type
GPlatesAppLogic::ReconstructionLayerTask::get_main_input_feature_collection_channel() const
{
	return LayerInputChannelName::RECONSTRUCTION_FEATURES;
}


void
GPlatesAppLogic::ReconstructionLayerTask::add_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTION_FEATURES)
	{
		d_reconstruction_layer_proxy->add_reconstruction_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructionLayerTask::remove_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
		
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTION_FEATURES)
	{
		d_reconstruction_layer_proxy->remove_reconstruction_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructionLayerTask::modified_input_file(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTION_FEATURES)
	{
		// Let the reconstruction layer proxy know that one of the rotation feature collections has been modified.
		d_reconstruction_layer_proxy->modified_reconstruction_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructionLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_reconstruction_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());
	d_reconstruction_layer_proxy->set_current_anchor_plate_id(reconstruction->get_anchor_plate_id());
}
