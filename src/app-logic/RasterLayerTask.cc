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

#include <algorithm>
#include <boost/foreach.hpp>
#include <QDebug>

#include "RasterLayerTask.h"

#include "ExtractRasterFeatureProperties.h"
#include "RasterLayerProxy.h"
#include "ReconstructLayerProxy.h"

#include "model/FeatureVisitor.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/TextContent.h"


bool
GPlatesAppLogic::RasterLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return contains_raster_feature(feature_collection);
}


GPlatesAppLogic::RasterLayerTask::RasterLayerTask() :
	d_raster_layer_proxy(RasterLayerProxy::create())
{
	// Defined in ".cc" file because non_null_ptr_type requires complete type for its destructor.
	// Data member destructors can get called if exception thrown in this constructor.
}


GPlatesAppLogic::RasterLayerTask::~RasterLayerTask()
{
	// Defined in ".cc" file because non_null_ptr_type requires complete type for its destructor.
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

		// Let the layer task params know of the new raster feature.
		d_layer_task_params.set_raster_feature(feature_ref);

		// Let the raster layer proxy know of the raster and let it know of the new parameters.
		d_raster_layer_proxy->set_current_raster_feature(feature_ref, d_layer_task_params);

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

		// Let the layer task params know of that there's now no raster feature because it may
		// need to change the raster band name (to an empty string) for example.
		d_layer_task_params.set_raster_feature(boost::none);

		// Set the raster feature to none in the raster layer proxy and let it know
		// of the new parameters.
		d_raster_layer_proxy->set_current_raster_feature(boost::none, d_layer_task_params);

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

		// Let the layer task params know of the new raster feature.
		d_layer_task_params.set_raster_feature(feature_ref);

		// Let the raster layer proxy know of the raster and let it know of the new parameters.
		d_raster_layer_proxy->set_current_raster_feature(feature_ref, d_layer_task_params);

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

	// If the layer task params have been modified then update our raster layer proxy.
	if (d_layer_task_params.d_set_band_name_called)
	{
		d_layer_task_params.d_set_band_name_called = false;

		d_raster_layer_proxy->set_current_raster_band_name(d_layer_task_params);
	}
}


GPlatesAppLogic::LayerProxy::non_null_ptr_type
GPlatesAppLogic::RasterLayerTask::get_layer_proxy()
{
	return d_raster_layer_proxy;
}


GPlatesAppLogic::RasterLayerTask::Params::Params() :
	d_band_name(GPlatesUtils::UnicodeString()),
	d_set_band_name_called(false)
{
}


void
GPlatesAppLogic::RasterLayerTask::Params::set_band_name(
		const GPlatesPropertyValues::TextContent &band_name)
{
	d_band_name = band_name;

	d_set_band_name_called = true;
	emit_modified();
}


const GPlatesPropertyValues::TextContent &
GPlatesAppLogic::RasterLayerTask::Params::get_band_name() const
{
	return d_band_name;
}


const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
GPlatesAppLogic::RasterLayerTask::Params::get_band_names() const
{
	return d_band_names;
}


GPlatesPropertyValues::RasterStatistics
GPlatesAppLogic::RasterLayerTask::Params::get_band_statistic() const
{
	return d_band_statistic;
}


const std::vector<GPlatesPropertyValues::RasterStatistics> &
GPlatesAppLogic::RasterLayerTask::Params::get_band_statistics() const
{
	return d_band_statistics;
}


const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
GPlatesAppLogic::RasterLayerTask::Params::get_georeferencing() const
{
	return d_georeferencing;
}


const boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> &
GPlatesAppLogic::RasterLayerTask::Params::get_spatial_reference_system() const
{
	return d_spatial_reference_system;
}


GPlatesPropertyValues::RasterType::Type
GPlatesAppLogic::RasterLayerTask::Params::get_raster_type() const
{
	return d_raster_type;
}


const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
GPlatesAppLogic::RasterLayerTask::Params::get_raster_feature() const
{
	return d_raster_feature;
}


void
GPlatesAppLogic::RasterLayerTask::Params::set_raster_feature(
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> raster_feature)
{
	d_raster_feature = raster_feature;

	updated_raster_feature();
}


void
GPlatesAppLogic::RasterLayerTask::Params::updated_raster_feature()
{
	// Clear everything (except band name) in case error (and return early).
	d_band_names.clear();
	d_band_statistic = GPlatesPropertyValues::RasterStatistics();
	d_band_statistics.clear();
	d_georeferencing = boost::none;
	d_spatial_reference_system = boost::none;
	d_raster_type = GPlatesPropertyValues::RasterType::UNKNOWN;

	// If there is no raster feature then clear everything.
	if (!d_raster_feature)
	{
		return;
	}

	// NOTE: We are visiting properties at (default) present day.
	// Raster statistics, for example, will change over time for time-dependent rasters.
	GPlatesAppLogic::ExtractRasterFeatureProperties visitor;
	visitor.visit_feature(d_raster_feature.get());

	// Get the georeferencing.
	d_georeferencing = visitor.get_georeferencing();

	// Get the spatial reference system.
	d_spatial_reference_system = visitor.get_spatial_reference_system();

	// If there are raster band names...
	boost::optional<std::size_t> band_name_index;
	if (visitor.get_raster_band_names() &&
		!visitor.get_raster_band_names()->empty())
	{
		d_band_names = visitor.get_raster_band_names().get();

		// Is the selected band name one of the available bands in the raster?
		// If not, then change the band name to be the first of the available bands.
		band_name_index = find_raster_band_name(d_band_names, d_band_name);
		if (!band_name_index)
		{
			// Set the band name using the default band index of zero.
			band_name_index = 0;
			d_band_name = d_band_names[band_name_index.get()]->value();
		}
	}

	if (visitor.get_proxied_rasters())
	{
		const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &proxied_rasters =
				visitor.get_proxied_rasters().get();

		BOOST_FOREACH(GPlatesPropertyValues::RawRaster::non_null_ptr_type proxied_raster, proxied_rasters)
		{
			GPlatesPropertyValues::RasterStatistics *raster_statistics =
					GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*proxied_raster);

			d_band_statistics.push_back(
					raster_statistics
							? *raster_statistics
							: GPlatesPropertyValues::RasterStatistics());

			// Get the raster type as an enumeration.
			// All band types should be the same - if they're not then the type will get set to the last band.
			d_raster_type = GPlatesPropertyValues::RawRasterUtils::get_raster_type(*proxied_raster);
		}

		if (band_name_index &&
			band_name_index.get() < d_band_statistics.size())
		{
			d_band_statistic = d_band_statistics[band_name_index.get()];
		}
	}

	// TODO: Notify observers (such as RasterVisualLayerParams) that the band name(s) have
	// changed - since it might need to update the colour palette if a new raw raster band is used.
}
