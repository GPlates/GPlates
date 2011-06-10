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

#include "data-mining/CoRegConfigurationTable.h"
#include "data-mining/DataTable.h"
#include "data-mining/DataSelector.h"


const QString GPlatesAppLogic::CoRegistrationLayerTask::CO_REGISTRATION_SEED_GEOMETRIES_CHANNEL_NAME =
		"CoRegistration seed Channel";
const QString GPlatesAppLogic::CoRegistrationLayerTask::CO_REGISTRATION_TARGET_GEOMETRIES_CHANNEL_NAME =
		"CoRegistration input Channel";


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

	// Channel definition for the reconstruction tree.
	input_channel_types.push_back(
			LayerInputChannelType(
					get_reconstruction_tree_channel_name(),
					LayerInputChannelType::ONE_DATA_IN_CHANNEL));

	input_channel_types.push_back(
			LayerInputChannelType(
					CO_REGISTRATION_SEED_GEOMETRIES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					LayerTaskType::RECONSTRUCT));

	input_channel_types.push_back(
			LayerInputChannelType(
					CO_REGISTRATION_TARGET_GEOMETRIES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					LayerTaskType::RECONSTRUCT));

	return input_channel_types;
}


QString
GPlatesAppLogic::CoRegistrationLayerTask::get_main_input_feature_collection_channel() const
{
	// The main input feature collection channel is not used because we only accept
	// input from other layers - so this string should never be seen by users.
	return QString("Unused Input File Channel");
}


void
GPlatesAppLogic::CoRegistrationLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::CoRegistrationLayerTask::remove_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::CoRegistrationLayerTask::modified_input_file(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::CoRegistrationLayerTask::add_input_layer_proxy_connection(
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

			d_coregistration_layer_proxy->set_current_reconstruction_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruction_layer_proxy.get()));
		}
	}
	else if (input_channel_name == CO_REGISTRATION_SEED_GEOMETRIES_CHANNEL_NAME)
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
	else if (input_channel_name == CO_REGISTRATION_TARGET_GEOMETRIES_CHANNEL_NAME)
	{
		// The target geometries layer proxy.
		boost::optional<ReconstructLayerProxy *> reconstructed_target_geometries_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstructed_target_geometries_layer_proxy)
		{
			d_coregistration_layer_proxy->add_coregistration_target_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstructed_target_geometries_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::CoRegistrationLayerTask::remove_input_layer_proxy_connection(
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

			d_coregistration_layer_proxy->set_current_reconstruction_layer_proxy(
					d_default_reconstruction_layer_proxy);
		}
	}
	else if (input_channel_name == CO_REGISTRATION_SEED_GEOMETRIES_CHANNEL_NAME)
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
	else if (input_channel_name == CO_REGISTRATION_TARGET_GEOMETRIES_CHANNEL_NAME)
	{
		// The target geometries layer proxy.
		boost::optional<ReconstructLayerProxy *> reconstructed_target_geometries_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstructed_target_geometries_layer_proxy)
		{
			d_coregistration_layer_proxy->remove_coregistration_target_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstructed_target_geometries_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::CoRegistrationLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_coregistration_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());

	// If the layer task params have been modified then update our reconstruct layer proxy.
	if (d_layer_task_params.d_set_cfg_table_called)
	{
		// NOTE: Currently this doesn't really do anything because the layer proxy also
		// has a reference to the table - but later it will have a copy of the configuration
		// table and this will notify it that the copy should be updated.
		d_coregistration_layer_proxy->set_current_coregistration_configuration_table(
				*d_layer_task_params.d_cfg_table);

		d_layer_task_params.d_set_cfg_table_called = false;
	}

	// If our layer proxy is currently using the default reconstruction layer proxy then
	// tell our layer proxy about the new default reconstruction layer proxy.
	if (d_using_default_reconstruction_layer_proxy)
	{
		// Avoid setting it every update unless it's actually a different layer.
		if (reconstruction->get_default_reconstruction_layer_output() != d_default_reconstruction_layer_proxy)
		{
			d_coregistration_layer_proxy->set_current_reconstruction_layer_proxy(
					reconstruction->get_default_reconstruction_layer_output());
		}
	}

	d_default_reconstruction_layer_proxy = reconstruction->get_default_reconstruction_layer_output();


	//
	// Here we deviate from the usual way layer proxies work by getting the co-reg layer proxy
	// to do its processing here rather than the usual way of waiting until some client queries
	// the layer proxy (such as during export or perhaps some other layer that uses us as input
	// - currently there are none of these layers).
	//
	// TODO: After the 1.1 release I'll change this, but for now I'll keep it the way it was
	// for minimum disturbance.
	//

	if(!d_layer_task_params.d_call_back || !d_layer_task_params.d_cfg_table)
	{
		//This case means layer parameters haven't be initialized. We should not proceed further from here.
		return;
	}
	refresh_data(GPlatesDataMining::DataTable());

	// Get the co-registration result data for the current reconstruction time.
	boost::optional<CoRegistrationData::non_null_ptr_type> coregistration_data =
			d_coregistration_layer_proxy->get_coregistration_data();

	// Notify observers of the new results data.
	if (coregistration_data)
	{
		refresh_data(coregistration_data.get()->data_table());

#ifdef _DEBUG
		std::cout << coregistration_data.get()->data_table() << std::endl;
#endif
	}
}


void
GPlatesAppLogic::CoRegistrationLayerTask::refresh_data(
		const GPlatesDataMining::DataTable& table)
{
	GPlatesDataMining::DataSelector::set_data_table(table);

	if (d_layer_task_params.d_call_back)
	{
		d_layer_task_params.d_call_back(GPlatesDataMining::DataSelector::get_data_table());
	}
}
