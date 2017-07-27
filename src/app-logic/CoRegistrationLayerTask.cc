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
#include <boost/foreach.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/begin.hpp>
#include <boost/weak_ptr.hpp>

#include "AppLogicUtils.h"
#include "CoRegistrationLayerTask.h"
#include "CoRegistrationData.h"
#include "RasterLayerProxy.h"
#include "ReconstructLayerProxy.h"

#include "data-mining/CoRegConfigurationTable.h"
#include "data-mining/DataTable.h"
#include "data-mining/DataSelector.h"


GPlatesAppLogic::CoRegistrationLayerTask::CoRegistrationLayerTask() :
		d_layer_params(CoRegistrationLayerParams::create()),
		d_coregistration_layer_proxy(CoRegistrationLayerProxy::create())
{
	// Notify our layer output whenever the layer params are modified.
	QObject::connect(
			d_layer_params.get(), SIGNAL(modified_cfg_table(GPlatesAppLogic::CoRegistrationLayerParams &)),
			this, SLOT(handle_cfg_table_modified(GPlatesAppLogic::CoRegistrationLayerParams &)));
}


bool
GPlatesAppLogic::CoRegistrationLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return false;
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::CoRegistrationLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// NOTE: There's no channel definition for a reconstruction tree - a rotation layer is not needed.

	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::CO_REGISTRATION_SEED_GEOMETRIES,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					LayerTaskType::RECONSTRUCT));

	// Channel definition for the co-registration targets:
	// - reconstructed feature geometries, or
	// - reconstructed raster(s).
	std::vector<LayerTaskType::Type> target_input_channel_types;
	target_input_channel_types.push_back(LayerTaskType::RECONSTRUCT);
	target_input_channel_types.push_back(LayerTaskType::RASTER);
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::CO_REGISTRATION_TARGET_GEOMETRIES,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					target_input_channel_types));

	return input_channel_types;
}


GPlatesAppLogic::LayerInputChannelName::Type
GPlatesAppLogic::CoRegistrationLayerTask::get_main_input_feature_collection_channel() const
{
	// The main input feature collection channel is not used because we only accept input from other layers.
	return LayerInputChannelName::UNUSED;
}


void
GPlatesAppLogic::CoRegistrationLayerTask::add_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::CoRegistrationLayerTask::remove_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::CoRegistrationLayerTask::modified_input_file(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::CoRegistrationLayerTask::add_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::CO_REGISTRATION_SEED_GEOMETRIES)
	{
		// The seed geometries layer proxy.
		boost::optional<ReconstructLayerProxy *> reconstructed_seed_geometries_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstructed_seed_geometries_layer_proxy)
		{
			d_coregistration_layer_proxy->add_coregistration_seed_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstructed_seed_geometries_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::CO_REGISTRATION_TARGET_GEOMETRIES)
	{
		// The target reconstructed geometries layer proxy.
		boost::optional<ReconstructLayerProxy *> target_reconstructed_geometries_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (target_reconstructed_geometries_layer_proxy)
		{
			d_coregistration_layer_proxy->add_coregistration_target_layer_proxy(
					GPlatesUtils::get_non_null_pointer(target_reconstructed_geometries_layer_proxy.get()));
		}

		// The target raster layer proxy.
		boost::optional<RasterLayerProxy *> target_raster_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<RasterLayerProxy>(layer_proxy);
		if (target_raster_layer_proxy)
		{
			d_coregistration_layer_proxy->add_coregistration_target_layer_proxy(
					GPlatesUtils::get_non_null_pointer(target_raster_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::CoRegistrationLayerTask::remove_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::CO_REGISTRATION_SEED_GEOMETRIES)
	{
		// The seed geometries layer proxy.
		boost::optional<ReconstructLayerProxy *> reconstructed_seed_geometries_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstructed_seed_geometries_layer_proxy)
		{
			d_coregistration_layer_proxy->remove_coregistration_seed_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstructed_seed_geometries_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::CO_REGISTRATION_TARGET_GEOMETRIES)
	{
		// The target reconstructed geometries layer proxy.
		boost::optional<ReconstructLayerProxy *> target_reconstructed_geometries_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (target_reconstructed_geometries_layer_proxy)
		{
			d_coregistration_layer_proxy->remove_coregistration_target_layer_proxy(
					GPlatesUtils::get_non_null_pointer(target_reconstructed_geometries_layer_proxy.get()));
		}

		// The target raster layer proxy.
		boost::optional<RasterLayerProxy *> target_raster_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<RasterLayerProxy>(layer_proxy);
		if (target_raster_layer_proxy)
		{
			d_coregistration_layer_proxy->remove_coregistration_target_layer_proxy(
					GPlatesUtils::get_non_null_pointer(target_raster_layer_proxy.get()));
		}

		// Remove any configuration table rows (from our layer params) that have target rows matching
		// the layer we are removing.
		GPlatesDataMining::CoRegConfigurationTable new_cfg_table;
		const GPlatesDataMining::CoRegConfigurationTable &cfg_table = d_layer_params->get_cfg_table();
		for (std::size_t row = 0; row < cfg_table.size(); ++row)
		{
			const GPlatesDataMining::ConfigurationTableRow &cfg_row = cfg_table[row];
			if (!cfg_row.target_layer.is_valid())
			{
				continue;
			}

			boost::optional<LayerProxy::non_null_ptr_type> cfg_target_layer_proxy =
					cfg_row.target_layer.get_layer_output();
			if (!cfg_target_layer_proxy || // A layer about to be removed is first deactivated
				cfg_target_layer_proxy == layer_proxy)
			{
				continue;
			}

			// The current config row's target layer is valid and is not being removed so keep it.
			new_cfg_table.push_back(cfg_row);
		}

		if (new_cfg_table.size() != cfg_table.size())
		{
			new_cfg_table.optimize();
			d_layer_params->set_cfg_table(new_cfg_table);
		}
	}
}


void
GPlatesAppLogic::CoRegistrationLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_coregistration_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());

	// NOTE: Clients of co-registration (eg, the co-registration results dialog or co-registration
	// export) are expected to query the layer proxy to process/get co-registration results.
}


void
GPlatesAppLogic::CoRegistrationLayerTask::handle_cfg_table_modified(
		CoRegistrationLayerParams &layer_params)
{
	// Update our co-registration layer proxy.
	d_coregistration_layer_proxy->set_current_coregistration_configuration_table(layer_params.get_cfg_table());
}
