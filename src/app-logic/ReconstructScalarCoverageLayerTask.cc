/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "ReconstructScalarCoverageLayerTask.h"

#include "AppLogicUtils.h"
#include "LayerProxyUtils.h"
#include "ReconstructScalarCoverageLayerProxy.h"
#include "ScalarCoverageFeatureProperties.h"


GPlatesAppLogic::ReconstructScalarCoverageLayerTask::ReconstructScalarCoverageLayerTask() :
	d_reconstruct_scalar_coverage_layer_proxy(ReconstructScalarCoverageLayerProxy::create()),
	d_layer_params(ReconstructScalarCoverageLayerParams::create(d_reconstruct_scalar_coverage_layer_proxy))
{
	// Notify our layer output whenever the layer params are modified.
	QObject::connect(
			d_layer_params.get(), SIGNAL(modified_reconstruct_scalar_coverage_params(GPlatesAppLogic::ReconstructScalarCoverageLayerParams &)),
			this, SLOT(handle_reconstruct_scalar_coverage_params_modified(GPlatesAppLogic::ReconstructScalarCoverageLayerParams &)));
}


bool
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return ScalarCoverageFeatureProperties::contains_scalar_coverage_feature(feature_collection);
}


boost::shared_ptr<GPlatesAppLogic::ReconstructScalarCoverageLayerTask>
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::create_layer_task()
{
	return boost::shared_ptr<ReconstructScalarCoverageLayerTask>(
			new ReconstructScalarCoverageLayerTask());
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for reconstructing coverages.
	std::vector<LayerInputChannelType::InputLayerType> domain_input_channel_types;
	domain_input_channel_types.push_back(
			LayerInputChannelType::InputLayerType(
					LayerTaskType::RECONSTRUCT,
					// Auto-connect to the domain (local means associated with same input file)...
					LayerInputChannelType::LOCAL_AUTO_CONNECT));
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::RECONSTRUCTED_SCALAR_COVERAGE_DOMAINS,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					domain_input_channel_types));
	
	return input_channel_types;
}


GPlatesAppLogic::LayerInputChannelName::Type
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::get_main_input_feature_collection_channel() const
{
	// The main input feature collection channel is not used because we only accept input from other layers.
	return LayerInputChannelName::UNUSED;
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::add_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::remove_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::modified_input_file(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// This layer type does not connect to any input files so nothing to do.
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::add_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTED_SCALAR_COVERAGE_DOMAINS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_reconstruct_scalar_coverage_layer_proxy->add_reconstructed_domain_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::remove_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTED_SCALAR_COVERAGE_DOMAINS)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_reconstruct_scalar_coverage_layer_proxy->remove_reconstructed_domain_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_reconstruct_scalar_coverage_layer_proxy->set_current_reconstruction_time(
			reconstruction->get_reconstruction_time());

	// Update the layer params in case the layer proxy changed (due to its dependency layers changing).
	// Note that this layer does not connect to any files and so we don't get directly notified of
	// changes to the features in the connected files - so we rely on our dependency layers instead.
	d_layer_params->update();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::handle_reconstruct_scalar_coverage_params_modified(
		ReconstructScalarCoverageLayerParams &layer_params)
{
	// Update our reconstruct scalar coverage layer proxy.
	d_reconstruct_scalar_coverage_layer_proxy->set_current_reconstruct_scalar_coverage_params(
			layer_params.get_reconstruct_scalar_coverage_params());
}
