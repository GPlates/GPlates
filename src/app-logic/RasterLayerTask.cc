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
#include "ResolvedRaster.h"

#include "model/FeatureVisitor.h"

#include "property-values/Georeferencing.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/TextContent.h"


namespace
{
	/**
	 * Visits a feature collection and determines whether the feature collection
	 * contains any raster features.
	 *
	 * The heuristic that we're using here is that it is a raster feature if there
	 * is all of the following:
	 *  - GmlRectifiedGrid inside a GpmlConstantValue inside a gpml:domainSet
	 *    top level property.
	 *  - GmlFile inside a GpmlConstantValue or a GpmlPiecewiseAggregation inside
	 *    a gpml:rangeSet top level property.
	 *  - any proxied raw raster (for any band) in the GmlFile is initialised.
	 *    TODO: Check only the band number that this layer task is interested in.
	 *          Although maybe not because the user could switch the band number in
	 *          the layer controls and this class is designed to test if a raster layer
	 *          can process the input feature.
	 *          So probably better to leave as is and just check that any band can be processed.
	 *  - GpmlRasterBandNames (not inside any time dependent structure) inside a
	 *    gpml:bandNames top level property.
	 */
	class CanResolveRasterFeature :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		CanResolveRasterFeature() :
			d_seen_gml_rectified_grid(false),
			d_seen_gml_file(false),
			d_seen_gpml_raster_band_names(false),
			d_inside_constant_value(false),
			d_inside_piecewise_aggregation(false),
			d_collection_has_raster_feature(false)
		{  }

		bool
		collection_has_raster_feature() const
		{
			return d_collection_has_raster_feature;
		}

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			d_seen_gml_rectified_grid = false;
			d_seen_gml_file = false;
			d_seen_at_least_one_valid_proxied_raw_raster = false;
			d_seen_gpml_raster_band_names = false;

			return true;
		}

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			if (d_seen_gml_rectified_grid &&
					d_seen_gml_file &&
					d_seen_at_least_one_valid_proxied_raw_raster &&
					d_seen_gpml_raster_band_names)
			{
				d_collection_has_raster_feature = true;
			}
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			d_inside_constant_value = true;
			gpml_constant_value.value()->accept_visitor(*this);
			d_inside_constant_value = false;
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			d_inside_piecewise_aggregation = true;
			std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows =
				gpml_piecewise_aggregation.time_windows();
			BOOST_FOREACH(const GPlatesPropertyValues::GpmlTimeWindow &time_window, time_windows)
			{
				time_window.time_dependent_value()->accept_visitor(*this);
			}
			d_inside_piecewise_aggregation = false;
		}

		virtual
		void
		visit_gml_rectified_grid(
				const GPlatesPropertyValues::GmlRectifiedGrid &gml_rectified_grid)
		{
			static const GPlatesModel::PropertyName DOMAIN_SET =
				GPlatesModel::PropertyName::create_gpml("domainSet");

			if (d_inside_constant_value)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == DOMAIN_SET)
				{
					d_seen_gml_rectified_grid = true;
				}
			}
		}

		virtual
		void
		visit_gml_file(
				const GPlatesPropertyValues::GmlFile &gml_file)
		{
			static const GPlatesModel::PropertyName RANGE_SET =
				GPlatesModel::PropertyName::create_gpml("rangeSet");

			if (d_inside_constant_value || d_inside_piecewise_aggregation)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == RANGE_SET)
				{
					d_seen_gml_file = true;

					// Make sure we have at least one initialised proxied raw raster for a band.
					// If we have at least one then it means we can process something (even if
					// it's only one band).
					const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_rasters =
							gml_file.proxied_raw_rasters();
					BOOST_FOREACH(
							const GPlatesPropertyValues::RawRaster::non_null_ptr_type &proxied_raster,
							proxied_rasters)
					{
						if (GPlatesPropertyValues::RawRasterUtils::has_proxied_data(*proxied_raster))
						{
							d_seen_at_least_one_valid_proxied_raw_raster = true;
							break;
						}
					}
				}
			}
		}

		virtual
		void
		visit_gpml_raster_band_names(
				const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names)
		{
			static const GPlatesModel::PropertyName BAND_NAMES =
				GPlatesModel::PropertyName::create_gpml("bandNames");

			if (!d_inside_constant_value && !d_inside_piecewise_aggregation)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == BAND_NAMES)
				{
					d_seen_gpml_raster_band_names = true;
				}
			}
		}

	private:
		bool d_seen_gml_rectified_grid;
		bool d_seen_gml_file;
		bool d_seen_at_least_one_valid_proxied_raw_raster;
		bool d_seen_gpml_raster_band_names;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;
		
		bool d_collection_has_raster_feature;
	};
}


const QString GPlatesAppLogic::RasterLayerTask::RASTER_FEATURE_CHANNEL_NAME =
		"Raster feature";
const QString GPlatesAppLogic::RasterLayerTask::RECONSTRUCTED_POLYGONS_CHANNEL_NAME =
		"Reconstructed polygons";
const QString GPlatesAppLogic::RasterLayerTask::AGE_GRID_RASTER_CHANNEL_NAME =
		"Age grid raster";


bool
GPlatesAppLogic::RasterLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	CanResolveRasterFeature visitor;
	for (GPlatesModel::FeatureCollectionHandle::const_iterator iter = feature_collection->begin();
			iter != feature_collection->end(); ++iter)
	{
		visitor.visit_feature(iter);
	}
	return visitor.collection_has_raster_feature();
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

	// Channel definition for the raster feature.
	input_channel_types.push_back(
			LayerInputChannelType(
					RASTER_FEATURE_CHANNEL_NAME,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL));
	
	// Channel definition for the reconstructed polygons.
	input_channel_types.push_back(
			LayerInputChannelType(
					RECONSTRUCTED_POLYGONS_CHANNEL_NAME,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RECONSTRUCT));
	
	// Channel definition for the age grid raster.
	input_channel_types.push_back(
			LayerInputChannelType(
					AGE_GRID_RASTER_CHANNEL_NAME,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL,
					LayerTaskType::RASTER));

	return input_channel_types;
}


QString
GPlatesAppLogic::RasterLayerTask::get_main_input_feature_collection_channel() const
{
	return RASTER_FEATURE_CHANNEL_NAME;
}


void
GPlatesAppLogic::RasterLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == RASTER_FEATURE_CHANNEL_NAME)
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
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == RASTER_FEATURE_CHANNEL_NAME)
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
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == RASTER_FEATURE_CHANNEL_NAME)
	{
		// Let the layer task params know that the raster feature has been modified.
		d_layer_task_params.raster_feature_modified();

		// Let the raster layer proxy know the raster feature has been modified.
		d_raster_layer_proxy->modified_raster_feature(d_layer_task_params);
	}
}


void
GPlatesAppLogic::RasterLayerTask::add_input_layer_proxy_connection(
		const QString &input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == RECONSTRUCTED_POLYGONS_CHANNEL_NAME)
	{
		// Make sure the input layer proxy is a reconstruct layer proxy.
		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			// We only allow one input reconstructed polygon layer.
			d_raster_layer_proxy->set_current_reconstructed_polygons_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}
	}
	else if (input_channel_name == AGE_GRID_RASTER_CHANNEL_NAME)
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
}


void
GPlatesAppLogic::RasterLayerTask::remove_input_layer_proxy_connection(
		const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == RECONSTRUCTED_POLYGONS_CHANNEL_NAME)
	{
		// Make sure the input layer proxy is a reconstruct layer proxy.
		boost::optional<const ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<
						const ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			// We only allow one input reconstructed polygon layer.
			d_raster_layer_proxy->set_current_reconstructed_polygons_layer_proxy(boost::none);
		}
	}
	else if (input_channel_name == AGE_GRID_RASTER_CHANNEL_NAME)
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
}


void
GPlatesAppLogic::RasterLayerTask::update(
		const Layer &layer_handle /* the layer invoking this */,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		const ReconstructionLayerProxy::non_null_ptr_type &default_reconstruction_layer_proxy)
{
	d_raster_layer_proxy->set_current_reconstruction_time(reconstruction_time);

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


const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
GPlatesAppLogic::RasterLayerTask::Params::get_georeferencing() const
{
	return d_georeferencing;
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
GPlatesAppLogic::RasterLayerTask::Params::raster_feature_modified()
{
	updated_raster_feature();
}


void
GPlatesAppLogic::RasterLayerTask::Params::updated_raster_feature()
{
	// If there is no raster feature then clear everything.
	if (!d_raster_feature)
	{
		d_band_name = GPlatesUtils::UnicodeString();
		d_band_names.clear();
		d_georeferencing = boost::none;

		return;
	}

	GPlatesAppLogic::ExtractRasterFeatureProperties visitor;
	visitor.visit_feature(d_raster_feature.get());

	// Get the georeferencing.
	d_georeferencing = visitor.get_georeferencing();

	// If there are no raster band names...
	if (!visitor.get_raster_band_names() ||
		visitor.get_raster_band_names().get().empty())
	{
		d_band_name = GPlatesUtils::UnicodeString();
		d_band_names.clear();
		return;
	}

	d_band_names = visitor.get_raster_band_names().get();

	// Is the selected band name one of the available bands in the raster?
	// If not, then change the band name to be the first of the available bands.
	boost::optional<std::size_t> band_name_index_opt =
			find_raster_band_name(d_band_names, d_band_name);
	if (!band_name_index_opt)
	{
		// Set the band name using the default band index of zero.
		d_band_name = d_band_names[0]->value();
	}

	// TODO: Notify observers (such as RasterVisualLayerParams) that the band name(s) have
	// changed - since it might need to update the colour palette if a new raw raster band is used.
}
