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

#include "RasterLayerTask.h"

#include "AgeGridRaster.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedRaster.h"

#include "model/FeatureVisitor.h"

#include "property-values/Georeferencing.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/RawRaster.h"
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
				if (propname && *propname == BAND_NAMES &&
					!contains_age_band_name(gpml_raster_band_names.band_names()))
				{
					d_seen_gpml_raster_band_names = true;
				}
			}
		}

	private:

		bool
		contains_age_band_name(
				const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names)
		{
			static const GPlatesPropertyValues::TextContent AGE_BAND_NAME(UnicodeString("age"));
			BOOST_FOREACH(const GPlatesPropertyValues::XsString::non_null_ptr_to_const_type &xs_string, raster_band_names)
			{
				if (xs_string->value() == AGE_BAND_NAME)
				{
					return true;
				}
			}

			return false;
		}

		bool d_seen_gml_rectified_grid;
		bool d_seen_gml_file;
		bool d_seen_gpml_raster_band_names;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;
		
		bool d_collection_has_raster_feature;
	};


	/**
	 * Visits a raster feature and extracts property information required to resolve the raster.
	 *
	 * The heuristic that we're using here is that it is a raster feature if there
	 * is all of the following:
	 *  - GmlRectifiedGrid inside a GpmlConstantValue inside a gpml:domainSet
	 *    top level property.
	 *  - GmlFile inside a GpmlConstantValue or a GpmlPiecewiseAggregation inside
	 *    a gpml:rangeSet top level property.
	 *  - GpmlRasterBandNames (not inside any time dependent structure) inside a
	 *    gpml:bandNames top level property.
	 */
	class ExtractRasterProperties :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		ExtractRasterProperties(
				const double &reconstruction_time) :
			d_reconstruction_time(reconstruction_time),
			d_inside_constant_value(false),
			d_inside_piecewise_aggregation(false)
		{  }


		const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
		get_georeferencing() const
		{
			return d_georeferencing;
		}

		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_proxied_rasters() const
		{
			return d_proxied_rasters;
		}

		const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
		get_raster_band_names() const
		{
			return d_raster_band_names;
		}


		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			d_georeferencing = boost::none;
			d_proxied_rasters = boost::none;
			d_raster_band_names = boost::none;

			return true;
		}

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
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
				const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
						time_window.valid_time();

				// If the time window period contains the current reconstruction time then visit.
				// The time periods should be mutually exclusive - if we happen to be it
				// two time periods then we're probably right on the boundary between the two
				// and then it doesn't really matter which one we choose.
				if (time_period->contains(d_reconstruction_time))
				{
					time_window.time_dependent_value()->accept_visitor(*this);
				}
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
					d_georeferencing = gml_rectified_grid.convert_to_georeferencing();
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
					d_proxied_rasters = gml_file.proxied_raw_rasters();
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
					d_raster_band_names = gpml_raster_band_names.band_names();
				}
			}
		}

	private:
		GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time;

		/**
		 * The georeferencing for the raster - currently treated as a constant value over time.
		 */
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> d_georeferencing;

		/**
		 * The proxied rasters of the time-resolved GmlFile (in the case of time-dependent rasters).
		 *
		 * The band name will be used to look up the correct raster in the presentation code.
		 * The user-selected band name is not accessible here since this is app-logic code.
		 */
		boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > d_proxied_rasters;

		/**
		 * The list of band names - one for each proxied raster.
		 */
		boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> d_raster_band_names;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;
	};
}


const char *GPlatesAppLogic::RasterLayerTask::RASTER_FEATURE_CHANNEL_NAME =
		"Raster feature";
const char *GPlatesAppLogic::RasterLayerTask::POLYGON_FEATURES_CHANNEL_NAME =
		"Polygon features";
const char *GPlatesAppLogic::RasterLayerTask::AGE_GRID_RASTER_CHANNEL_NAME =
		"Age grid feature";


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


std::vector<GPlatesAppLogic::Layer::input_channel_definition_type>
GPlatesAppLogic::RasterLayerTask::get_input_channel_definitions() const
{
	std::vector<Layer::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction tree.
	input_channel_definitions.push_back(
			boost::make_tuple(
					get_reconstruction_tree_channel_name(),
					Layer::INPUT_RECONSTRUCTION_TREE_DATA,
					Layer::ONE_DATA_IN_CHANNEL));

	// Channel definition for the raster feature.
	input_channel_definitions.push_back(
			boost::make_tuple(
					RASTER_FEATURE_CHANNEL_NAME,
					Layer::INPUT_FEATURE_COLLECTION_DATA,
					Layer::ONE_DATA_IN_CHANNEL));
	
	// Channel definition for the polygon features (used to reconstruct the raster).
	input_channel_definitions.push_back(
			boost::make_tuple(
					POLYGON_FEATURES_CHANNEL_NAME,
					Layer::INPUT_FEATURE_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));
	
	// Channel definition for the age grid feature.
	input_channel_definitions.push_back(
			boost::make_tuple(
					AGE_GRID_RASTER_CHANNEL_NAME,
					Layer::INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA,
					Layer::ONE_DATA_IN_CHANNEL));

	return input_channel_definitions;
}


QString
GPlatesAppLogic::RasterLayerTask::get_main_input_feature_collection_channel() const
{
	return RASTER_FEATURE_CHANNEL_NAME;
}


GPlatesAppLogic::Layer::LayerOutputDataType
GPlatesAppLogic::RasterLayerTask::get_output_definition() const
{
	return Layer::OUTPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA;
}


boost::optional<GPlatesAppLogic::layer_task_data_type>
GPlatesAppLogic::RasterLayerTask::process(
		const Layer &layer_handle /* the layer invoking this */,
		const input_data_type &input_data,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree)
{
	//
	// Get the reconstruction tree input.
	//
	boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree =
			extract_reconstruction_tree(
					input_data,
					default_reconstruction_tree);
	if (!reconstruction_tree)
	{
		// Expecting a single reconstruction tree.
		return boost::none;
	}

	//
	// Get the raster features collection input.
	//
	// NOTE: Raster layers are special in that only one raster feature should exist
	// in the input feature collection.
	//
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> raster_feature_collection;
	extract_input_channel_data(
			raster_feature_collection,
			RASTER_FEATURE_CHANNEL_NAME,
			input_data);

	if (raster_feature_collection.size() != 1 ||
		raster_feature_collection[0]->size() != 1)
	{
		// Expecting a single raster feature.
		return boost::none;
	}
	GPlatesModel::FeatureHandle::weak_ref raster_feature =
			(*raster_feature_collection[0]->begin())->reference();

	// Extract the georeferencing and raster data.
	ExtractRasterProperties extract_raster_properties(reconstruction_time);
	extract_raster_properties.visit_feature(raster_feature);

	const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
			georeferencing = extract_raster_properties.get_georeferencing();
	if (!georeferencing)
	{
		// We need georeferencing information to display rasters.
		return boost::none;
	}

	const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
			proxied_rasters = extract_raster_properties.get_proxied_rasters();
	if (!proxied_rasters || proxied_rasters->empty())
	{
		// We at least one proxied raster.
		return boost::none;
	}

	const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
			raster_band_names = extract_raster_properties.get_raster_band_names();
	if (!raster_band_names || raster_band_names->empty())
	{
		// We at least one band name.
		return boost::none;
	}


	//
	// Get the age grid raster input.
	//
	std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type> age_grid_rasters;
	extract_input_channel_data(
			age_grid_rasters,
			AGE_GRID_RASTER_CHANNEL_NAME,
			input_data);

	// The optional age grid variables we extract if we find an age grid raster input.
	boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> age_grid_georeferencing;
	boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > age_grid_proxied_rasters;
	boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> age_grid_raster_band_names;

	if (age_grid_rasters.size() == 1)
	{
		ReconstructionGeometryCollection::const_iterator recon_geom_iter = age_grid_rasters[0]->begin();
		ReconstructionGeometryCollection::const_iterator recon_geom_end = age_grid_rasters[0]->end();
		if (recon_geom_iter != recon_geom_end)
		{
			// Make sure the reconstruction geometry is an age grid raster.
			ReconstructionGeometry::non_null_ptr_to_const_type recon_geom = *recon_geom_iter;

			// We're expecting only one age grid raster.
			++recon_geom_iter;
			if (recon_geom_iter == recon_geom_end)
			{
				boost::optional<const AgeGridRaster *> age_grid_raster_opt =
						ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
								const AgeGridRaster>(recon_geom);
				if (age_grid_raster_opt)
				{
					const AgeGridRaster *age_grid_raster = age_grid_raster_opt.get();

					age_grid_georeferencing = age_grid_raster->get_georeferencing();
					age_grid_proxied_rasters = age_grid_raster->get_proxied_rasters();
					age_grid_raster_band_names = age_grid_raster->get_raster_band_names();
				}
			}
		}
	}

	// Update the polygon rotations using the new reconstruction tree.
	update_reconstruct_raster_polygons(reconstruction_tree.get(), input_data);


	// Create a reconstruction geometry collection to store the resolved raster in.
	ReconstructionGeometryCollection::non_null_ptr_type reconstruction_geometry_collection =
			ReconstructionGeometryCollection::create(reconstruction_tree.get());

	// Need to cast boost::optional containing non_null_ptr_type to
	// a boost::optional containing non_null_ptr_to_const_type.
	boost::optional<ReconstructRasterPolygons::non_null_ptr_to_const_type>
			reconstruct_raster_polygons;
	if (d_reconstruct_raster_polygons)
	{
		reconstruct_raster_polygons = d_reconstruct_raster_polygons.get();
	}

	// Create a resolved raster.
	ResolvedRaster::non_null_ptr_type resolved_raster =
			ResolvedRaster::create(
					*raster_feature.handle_ptr(),
					layer_handle,
					reconstruction_time,
					reconstruction_tree.get(),
					georeferencing.get(),
					proxied_rasters.get(),
					raster_band_names.get(),
					reconstruct_raster_polygons,
					age_grid_georeferencing,
					age_grid_proxied_rasters,
					age_grid_raster_band_names);

	reconstruction_geometry_collection->add_reconstruction_geometry(resolved_raster);

	return layer_task_data_type(
			static_cast<ReconstructionGeometryCollection::non_null_ptr_to_const_type>(
					reconstruction_geometry_collection));
}


void
GPlatesAppLogic::RasterLayerTask::update_reconstruct_raster_polygons(
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
		const input_data_type &input_data)
{
	//
	// Get the polygon features collection input (used to reconstruct the raster).
	//
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> polygon_features_collection;
	extract_input_channel_data(
			polygon_features_collection,
			POLYGON_FEATURES_CHANNEL_NAME,
			input_data);

	bool reconstruct_raster_polygons_have_changed = false;

	// Sort the sequence so we can compare with out current sequence of feature collections.
	std::sort(polygon_features_collection.begin(), polygon_features_collection.end());

	// See if any feature collections have been added or removed.
	//
	// TODO: Also need to listen for changes in the model in case an existing
	// feature collection has been modified since we were last called.
	if (d_current_polygon_features_collection != polygon_features_collection)
	{
		d_current_polygon_features_collection = polygon_features_collection;
		reconstruct_raster_polygons_have_changed = true;
	}

	// If there are no polygon features then we cannot reconstruct the raster.
	if (d_current_polygon_features_collection.empty())
	{
		d_reconstruct_raster_polygons = boost::none;
		return;
	}

	// If the polygon features have changed then create a new 'ReconstructRasterPolygons' object.
	if (reconstruct_raster_polygons_have_changed)
	{
		d_reconstruct_raster_polygons = ReconstructRasterPolygons::create(
				d_current_polygon_features_collection.begin(),
				d_current_polygon_features_collection.end(),
				reconstruction_tree);
		return;
	}

	// The polygon features have not changed so just update the rotations in the
	// existing 'ReconstructRasterPolygons' object using the new reconstruction tree.
	d_reconstruct_raster_polygons.get()->update_rotations(reconstruction_tree);
}
