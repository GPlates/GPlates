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
#include <cmath>
#include <limits>

#include "ReconstructScalarCoverageLayerTask.h"

#include "AppLogicUtils.h"
#include "LayerProxyUtils.h"
#include "ReconstructScalarCoverageLayerProxy.h"
#include "ScalarCoverageFeatureProperties.h"


GPlatesAppLogic::ReconstructScalarCoverageLayerTask::ReconstructScalarCoverageLayerTask() :
	d_layer_task_params(*this),
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

	// Update the layer task params in case the layer proxy changed (due to its dependency layers changing).
	// Note that this layer does not connect to any files and so we don't get directly notified of
	// changes to the features in the connected files - so we rely on our dependency layers instead.
	d_layer_task_params.update();
}


GPlatesAppLogic::LayerProxy::non_null_ptr_type
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::get_layer_proxy()
{
	return d_reconstruct_scalar_coverage_layer_proxy;
}


GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::Params(
		ReconstructScalarCoverageLayerTask &layer_task) :
	d_layer_task(layer_task)
{
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::update()
{
	// If we're not up-to-date with respect to the layer proxy...
	//
	// We need this since we don't get notified *directly* of changes in the Reconstruct layer
	// that our Reconstruct Scalar Coverage layer is connected to. For example, if the
	// scalar coverage features are reloaded from file they might no longer contain
	// the currently selected scalar type and so we'll have to change it.
	if (!d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->get_subject_token().is_observer_up_to_date(
			d_layer_proxy_observer_token))
	{
		// Is the selected scalar type one of the available scalar types in the scalar coverage features?
		// If not, then change the scalar type to be the first of the available scalar types.
		// This can happen if the scalar coverage features are reloaded from file and no longer contain
		// the currently selected scalar type.
		const GPlatesPropertyValues::ValueObjectType &current_scalar_type =
				d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->get_current_scalar_type();
		const std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types = get_scalar_types();
		if (!scalar_types.empty() &&
			std::find(scalar_types.begin(), scalar_types.end(), current_scalar_type) == scalar_types.end())
		{
			// Set the current scalar type using the default scalar type index of zero.
			d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->set_current_scalar_type(scalar_types.front());
		}

		// We are now up-to-date with respect to the layer proxy.
		//
		// Note that we do this after setting the scalar type on the layer proxy since that just
		// invalidates its subject token again and hence we'll (incorrectly) always need updating.
		d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->get_subject_token().update_observer(
				d_layer_proxy_observer_token);

		// We always emit modified signal if we were out-of-date with Reconstruct layer since
		// it could have changed the list of scalar types, the scalar statistics, etc.
		emit_modified();
	}
}


const GPlatesPropertyValues::ValueObjectType &
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::get_scalar_type() const
{
	// Update the current scalar type if the scalar coverage feature has changed.
	//
	// We need this since we don't get notified *directly* of changes in the Reconstruct layer
	// that our Reconstruct Scalar Coverage layer is connected to. For example, if the
	// scalar coverage features are reloaded from file they might no longer contain
	// the currently selected scalar type.
	//
	// This update also ensures that the we will have a valid current-scalar-type if
	// we're currently being called by ReconstructScalarCoverageVisualLayerParams
	// (to re-map its colour palette according to the scalar coverage statistics).
	const_cast<Params *>(this)->update();

	return d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->get_current_scalar_type();
}


const std::vector<GPlatesPropertyValues::ValueObjectType> &
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::get_scalar_types() const
{
	return d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->get_scalar_types();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::set_scalar_type(
		const GPlatesPropertyValues::ValueObjectType &scalar_type)
{
	if (d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->get_current_scalar_type() == scalar_type)
	{
		return;
	}

	d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->set_current_scalar_type(scalar_type);

	emit_modified();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::set_reconstruct_scalar_coverage_params(
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	if (d_reconstruct_scalar_coverage_params == reconstruct_scalar_coverage_params)
	{
		return;
	}

	d_reconstruct_scalar_coverage_params = reconstruct_scalar_coverage_params;

	d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->set_current_reconstruct_scalar_coverage_params(
			d_reconstruct_scalar_coverage_params);

	emit_modified();
}


const std::vector<GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage> &
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::get_scalar_coverages() const
{
	return d_layer_task.d_reconstruct_scalar_coverage_layer_proxy->get_scalar_coverages();
}


boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics>
GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params::get_scalar_statistics(
		const GPlatesPropertyValues::ValueObjectType &scalar_type) const
{
	// Scalar statistics.
	//
	// mean = M = sum(Xi) / N
	// std_dev  = sqrt[(sum(Xi^2) / N - M^2]
	//
	// ...where N is total number of scalar samples.
	//
	unsigned int num_scalars = 0;
	double scalar_min = (std::numeric_limits<double>::max)();
	double scalar_max = (std::numeric_limits<double>::min)();
	double scalar_sum = 0;
	double scalar_sum_squares = 0;

	// Iterate over the coverages to find the set of unique scalar types.
	const std::vector<ScalarCoverageFeatureProperties::Coverage> &scalar_coverages = get_scalar_coverages();
	std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator coverages_iter = scalar_coverages.begin();
	std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator coverages_end = scalar_coverages.end();
	for ( ; coverages_iter != coverages_end; ++coverages_iter)
	{
		const ScalarCoverageFeatureProperties::Coverage &coverage = *coverages_iter;

		// See if any scalar types in the current coverage match.
		const unsigned int num_scalar_types = coverage.range.size();
		for (unsigned int scalar_type_index = 0; scalar_type_index < num_scalar_types; ++scalar_type_index)
		{
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type
					coverage_range = coverage.range[scalar_type_index];
			if (coverage_range->value_object_type() != scalar_type)
			{
				continue;
			}

			num_scalars += coverage_range->coordinates_len();

			// Update the scalar statistics from the scalar values.
			GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinate_list_type::const_iterator
					coordinates_iter = coverage_range->coordinates_begin();
			GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinate_list_type::const_iterator
					coordinates_end = coverage_range->coordinates_end();
			for ( ; coordinates_iter != coordinates_end; ++coordinates_iter)
			{
				const double scalar = *coordinates_iter;
				scalar_sum += scalar;
				scalar_sum_squares += scalar * scalar;

				if (scalar < scalar_min)
				{
					scalar_min = scalar;
				}
				if (scalar > scalar_max)
				{
					scalar_max = scalar;
				}
			}

			break;
		}
	}

	if (num_scalars == 0)
	{
		// There were no scalar coverages of the requested scalar type.
		return boost::none;
	}

	const double scalar_mean = scalar_sum / num_scalars;
	const double scalar_standard_deviation =
			std::sqrt((scalar_sum_squares / num_scalars) - (scalar_mean * scalar_mean));

	return GPlatesPropertyValues::ScalarCoverageStatistics(
			scalar_min,
			scalar_max,
			scalar_mean,
			scalar_standard_deviation);
}
