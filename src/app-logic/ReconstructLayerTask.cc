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

#include "ApplicationState.h"
#include "AppLogicUtils.h"
#include "LayerProxyUtils.h"
#include "ReconstructMethodRegistry.h"
#include "ReconstructUtils.h"


const QString GPlatesAppLogic::ReconstructLayerTask::RECONSTRUCTABLE_FEATURES_CHANNEL_NAME =
		"Reconstructable features";


bool
GPlatesAppLogic::ReconstructLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		ApplicationState &application_state)
{
	return ReconstructUtils::has_reconstructable_features(
			feature_collection,
			application_state.get_reconstruct_method_registry());
}


boost::shared_ptr<GPlatesAppLogic::ReconstructLayerTask>
GPlatesAppLogic::ReconstructLayerTask::create_layer_task(
		ApplicationState &application_state)
{
	return boost::shared_ptr<ReconstructLayerTask>(
			new ReconstructLayerTask(
					application_state.get_reconstruct_method_registry()));
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::ReconstructLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the reconstruction tree.
	input_channel_types.push_back(
			LayerInputChannelType(
					get_reconstruction_tree_channel_name(),
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RECONSTRUCTION));

	// Channel definition for the reconstructable features.
	input_channel_types.push_back(
			LayerInputChannelType(
					RECONSTRUCTABLE_FEATURES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL));
	
	return input_channel_types;
}


QString
GPlatesAppLogic::ReconstructLayerTask::get_main_input_feature_collection_channel() const
{
	return RECONSTRUCTABLE_FEATURES_CHANNEL_NAME;
}


void
GPlatesAppLogic::ReconstructLayerTask::activate(
		bool active)
{
	// If deactivated then specify an empty set of topological sections layer proxies.
	if (!active)
	{
		// This method flushes any RFGs contained in the layer proxy.
		// This is done so that topologies will no longer find the RFGs when they lookup observers
		// of topological section features.
		// This issue exists because the topology layers do not restrict topological sections
		// to their input channels (and hence have no input channels for topological sections).
		d_reconstruct_layer_proxy->reset_reconstructed_feature_geometry_caches();
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == RECONSTRUCTABLE_FEATURES_CHANNEL_NAME)
	{
		d_reconstruct_layer_proxy->add_reconstructable_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::remove_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == RECONSTRUCTABLE_FEATURES_CHANNEL_NAME)
	{
		d_reconstruct_layer_proxy->remove_reconstructable_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::modified_input_file(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == RECONSTRUCTABLE_FEATURES_CHANNEL_NAME)
	{
		// Let the reconstruct layer proxy know that one of the reconstructable feature collections has been modified.
		d_reconstruct_layer_proxy->modified_reconstructable_feature_collection(feature_collection);
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::add_input_layer_proxy_connection(
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
			// Stop using the default reconstruction layer proxy.
			d_using_default_reconstruction_layer_proxy = false;

			d_reconstruct_layer_proxy->set_current_reconstruction_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruction_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::remove_input_layer_proxy_connection(
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

			d_reconstruct_layer_proxy->set_current_reconstruction_layer_proxy(
					d_default_reconstruction_layer_proxy);
		}
	}
}


void
GPlatesAppLogic::ReconstructLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_reconstruct_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());

	// If the layer task params have been modified then update our reconstruct layer proxy.
	if (d_layer_task_params.d_non_const_get_reconstruct_params_called)
	{
		// NOTE: This should call the non-const version of 'Params::get_reconstruct_params()'
		// so that it doesn't think we're modifying it.
		d_reconstruct_layer_proxy->set_current_reconstruct_params(
				d_layer_task_params.get_reconstruct_params());

		d_layer_task_params.d_non_const_get_reconstruct_params_called = false;
	}

	// If our layer proxy is currently using the default reconstruction layer proxy then
	// tell our layer proxy about the new default reconstruction layer proxy.
	if (d_using_default_reconstruction_layer_proxy)
	{
		// Avoid setting it every update unless it's actually a different layer.
		if (reconstruction->get_default_reconstruction_layer_output() != d_default_reconstruction_layer_proxy)
		{
			d_reconstruct_layer_proxy->set_current_reconstruction_layer_proxy(
					reconstruction->get_default_reconstruction_layer_output());
		}
	}

	d_default_reconstruction_layer_proxy = reconstruction->get_default_reconstruction_layer_output();
}


GPlatesAppLogic::ReconstructLayerTask::Params::Params() :
	d_non_const_get_reconstruct_params_called(false)
{
}


const GPlatesAppLogic::ReconstructParams &
GPlatesAppLogic::ReconstructLayerTask::Params::get_reconstruct_params() const
{
	return d_reconstruct_params;
}


GPlatesAppLogic::ReconstructParams &
GPlatesAppLogic::ReconstructLayerTask::Params::get_reconstruct_params()
{
	d_non_const_get_reconstruct_params_called = true;

	// FIXME: Should probably call 'emit_modified()' but first need to change 'get_reconstruct_params()'
	// to 'set_reconstruct_params()' so that a signal is emitted *after* modifications have been made.
	// Currently this is not needed because 'SetVGPVisibilityDialog::handle_apply()'
	// explicitly does a reconstruction which ensures an update after all modifications are made.

	return d_reconstruct_params;
}
