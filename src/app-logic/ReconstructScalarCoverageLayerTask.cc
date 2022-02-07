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

#include <algorithm>

#include "ReconstructScalarCoverageLayerTask.h"

#include "AppLogicUtils.h"
#include "LayerProxyUtils.h"
#include "ReconstructScalarCoverageLayerProxy.h"
#include "ScalarCoverageFeatureProperties.h"


GPlatesAppLogic::ReconstructScalarCoverageLayerTask::ReconstructScalarCoverageLayerTask() :
	d_reconstruct_scalar_coverage_layer_proxy(
			ReconstructScalarCoverageLayerProxy::create())
{
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

	// If the layer task params have been modified then update our layer proxy.
	if (d_layer_task_params.d_set_scalar_type_called)
	{
		d_reconstruct_scalar_coverage_layer_proxy->set_current_scalar_type(
				d_layer_task_params.d_scalar_type);

		d_layer_task_params.d_set_scalar_type_called = false;
	}

	// If the layer task params have been modified then update our layer proxy.
	if (d_layer_task_params.d_set_reconstruct_scalar_coverage_params_called)
	{
		d_reconstruct_scalar_coverage_layer_proxy->set_current_reconstruct_scalar_coverage_params(
				d_layer_task_params.d_reconstruct_scalar_coverage_params);

		d_layer_task_params.d_set_reconstruct_scalar_coverage_params_called = false;
	}

	// Update the layer task params in case the layer proxy changed (due to its dependency layers changing).
	d_layer_task_params.update(
			d_reconstruct_scalar_coverage_layer_proxy->get_current_scalar_type(),
			d_reconstruct_scalar_coverage_layer_proxy->get_scalar_types());
}


GPlatesAppLogic::LayerProxy::non_null_ptr_type
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::get_layer_proxy()
{
	return d_reconstruct_scalar_coverage_layer_proxy;
}


GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::Params() :
	d_scalar_type(GPlatesPropertyValues::ValueObjectType::create_gpml("")),
	d_set_scalar_type_called(false),
	d_set_reconstruct_scalar_coverage_params_called(false)
{
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::update(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types)
{
	d_scalar_type = scalar_type;
	d_scalar_types = scalar_types;

	// Is the selected scalar type is one of the available scalar types in the scalar coverage features?
	// If not, then change the scalar type to be the first of the available scalar types.
	if (!scalar_types.empty() &&
		std::find(scalar_types.begin(), scalar_types.end(), scalar_type) == scalar_types.end())
	{
		// Set the scalar type using the default scalar type index of zero.
		d_scalar_type = d_scalar_types.front();
	}

	// TODO: Notify observers (such as ReconstructScalarCoverageVisualLayerParams) that the scalar type(s)
	// have changed - since it might need to update itself.
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::set_scalar_type(
		const GPlatesPropertyValues::ValueObjectType &scalar_type)
{
	d_scalar_type = scalar_type;

	d_set_scalar_type_called = true;
	emit_modified();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::set_reconstruct_scalar_coverage_params(
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	d_reconstruct_scalar_coverage_params = reconstruct_scalar_coverage_params;

	d_set_reconstruct_scalar_coverage_params_called = true;
	emit_modified();
}
