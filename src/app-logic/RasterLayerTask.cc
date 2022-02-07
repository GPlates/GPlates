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

#include <QDebug>

#include "RasterLayerTask.h"

#include "ExtractRasterFeatureProperties.h"
#include "LayerProxyUtils.h"
#include "ReconstructLayerProxy.h"


bool
GPlatesAppLogic::RasterLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return contains_raster_feature(feature_collection);
}


GPlatesAppLogic::RasterLayerTask::RasterLayerTask() :
	d_layer_params(RasterLayerParams::create()),
	d_raster_layer_proxy(RasterLayerProxy::create())
{
	// Notify our layer output whenever the layer params are modified.
	QObject::connect(
			d_layer_params.get(), SIGNAL(modified_band_name(GPlatesAppLogic::RasterLayerParams &)),
			this, SLOT(handle_band_name_modified(GPlatesAppLogic::RasterLayerParams &)));
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::RasterLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// NOTE: There's no channel definition for a reconstruction tree - a rotation layer is not needed.

	// Channel definition for the raster feature.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::RASTER_FEATURE,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL));
	
	// Channel definition for the reconstructed polygons.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::RECONSTRUCTED_POLYGONS,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					LayerTaskType::RECONSTRUCT));
	
	// Channel definition for the age grid raster.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::AGE_GRID_RASTER,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RASTER));
	
	// Channel definition for the normal map raster.
	input_channel_types.push_back(
			LayerInputChannelType(
					LayerInputChannelName::NORMAL_MAP_RASTER,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RASTER));

	return input_channel_types;
}


GPlatesAppLogic::LayerInputChannelName::Type
GPlatesAppLogic::RasterLayerTask::get_main_input_feature_collection_channel() const
{
	return LayerInputChannelName::RASTER_FEATURE;
}


void
GPlatesAppLogic::RasterLayerTask::add_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RASTER_FEATURE)
	{
		// A raster feature collection should have only one feature.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
		if (features_iter == features_end)
		{
			qWarning() << "Raster feature collection contains no features.";
			return;
		}

		// Set the raster feature in the raster layer proxy.
		const GPlatesModel::FeatureHandle::weak_ref feature_ref = (*features_iter)->reference();

		// Let the layer params know of the new raster feature.
		d_layer_params->set_raster_feature(feature_ref);

		// Let the raster layer proxy know of the raster and let it know of the new parameters.
		d_raster_layer_proxy->set_current_raster_feature(feature_ref, *d_layer_params);

		// A raster feature collection should have only one feature.
		if (++features_iter != features_end)
		{
			qWarning() << "Raster feature collection contains more than one feature - "
					"ignoring all but the first.";
		}
	}
}


void
GPlatesAppLogic::RasterLayerTask::remove_input_file_connection(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RASTER_FEATURE)
	{
		// A raster feature collection should have only one feature.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
		if (features_iter == features_end)
		{
			qWarning() << "Raster feature collection contains no features.";
			return;
		}

		// Let the layer params know of that there's now no raster feature because it may
		// need to change the raster band name (to an empty string) for example.
		d_layer_params->set_raster_feature(boost::none);

		// Set the raster feature to none in the raster layer proxy and let it know
		// of the new parameters.
		d_raster_layer_proxy->set_current_raster_feature(boost::none, *d_layer_params);

		// A raster feature collection should have only one feature.
		if (++features_iter != features_end)
		{
			qWarning() << "Raster feature collection contains more than one feature - "
					"ignoring all but the first.";
		}
	}
}


void
GPlatesAppLogic::RasterLayerTask::modified_input_file(
		LayerInputChannelName::Type input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == LayerInputChannelName::RASTER_FEATURE)
	{
		// The feature collection has been modified which means it may have a new feature such as when
		// a file is reloaded (same feature collection but all features are removed and reloaded).
		// So we have to assume the existing raster feature is no longer valid so we need to set
		// the raster feature again.
		//
		// This is pretty much the same as 'add_input_file_connection()'.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
		if (features_iter == features_end)
		{
			// A raster feature collection should have one feature.
			qWarning() << "Modified raster feature collection contains no features.";
			return;
		}

		// Set the raster feature in the raster layer proxy.
		const GPlatesModel::FeatureHandle::weak_ref feature_ref = (*features_iter)->reference();

		// Let the layer params know of the new raster feature.
		d_layer_params->set_raster_feature(feature_ref);

		// Let the raster layer proxy know of the raster and let it know of the new parameters.
		d_raster_layer_proxy->set_current_raster_feature(feature_ref, *d_layer_params);

		// A raster feature collection should have only one feature.
		if (++features_iter != features_end)
		{
			qWarning() << "Modified raster feature collection contains more than one feature - "
					"ignoring all but the first.";
		}
	}
}


void
GPlatesAppLogic::RasterLayerTask::add_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTED_POLYGONS)
	{
		// Make sure the input layer proxy is a reconstruct layer proxy.
		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_raster_layer_proxy->add_current_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::AGE_GRID_RASTER)
	{
		// Make sure the input layer proxy is a raster layer proxy.
		boost::optional<RasterLayerProxy *> raster_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						RasterLayerProxy>(layer_proxy);
		if (raster_layer_proxy)
		{
			d_raster_layer_proxy->set_current_age_grid_raster_layer_proxy(
					GPlatesUtils::get_non_null_pointer(raster_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::NORMAL_MAP_RASTER)
	{
		// Make sure the input layer proxy is a raster layer proxy.
		boost::optional<RasterLayerProxy *> raster_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						RasterLayerProxy>(layer_proxy);
		if (raster_layer_proxy)
		{
			d_raster_layer_proxy->set_current_normal_map_raster_layer_proxy(
					GPlatesUtils::get_non_null_pointer(raster_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::RasterLayerTask::remove_input_layer_proxy_connection(
		LayerInputChannelName::Type input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == LayerInputChannelName::RECONSTRUCTED_POLYGONS)
	{
		// Make sure the input layer proxy is a reconstruct layer proxy.
		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_raster_layer_proxy->remove_current_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}
	}
	else if (input_channel_name == LayerInputChannelName::AGE_GRID_RASTER)
	{
		// Make sure the input layer proxy is a raster layer proxy.
		boost::optional<const RasterLayerProxy *> raster_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						const RasterLayerProxy>(layer_proxy);
		if (raster_layer_proxy)
		{
			d_raster_layer_proxy->set_current_age_grid_raster_layer_proxy(boost::none);
		}
	}
	else if (input_channel_name == LayerInputChannelName::NORMAL_MAP_RASTER)
	{
		// Make sure the input layer proxy is a raster layer proxy.
		boost::optional<RasterLayerProxy *> raster_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						RasterLayerProxy>(layer_proxy);
		if (raster_layer_proxy)
		{
			d_raster_layer_proxy->set_current_normal_map_raster_layer_proxy(boost::none);
		}
	}
}


void
GPlatesAppLogic::RasterLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_raster_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());
}


GPlatesAppLogic::LayerProxy::non_null_ptr_type
GPlatesAppLogic::RasterLayerTask::get_layer_proxy()
{
	return d_raster_layer_proxy;
}


void
GPlatesAppLogic::RasterLayerTask::handle_band_name_modified(
		RasterLayerParams &layer_params)
{
	// Update our raster layer proxy.
	d_raster_layer_proxy->set_current_raster_band_name(layer_params);
}
