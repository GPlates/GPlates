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

#include "RasterLayerTask.h"

#include "ResolvedRaster.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRaster.h"


const char *GPlatesAppLogic::RasterLayerTask::RASTER_FEATURE_CHANNEL_NAME =
		"raster feature";
const char *GPlatesAppLogic::RasterLayerTask::POLYGON_FEATURES_CHANNEL_NAME =
		"polygon features";


bool
GPlatesAppLogic::RasterLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	// FIXME: Visit features in 'feature_collection' and return true if any
	// have a raster property.
	return false;
}


GPlatesAppLogic::RasterLayerTask::RasterLayerTask()
{
	// FIXME: When raster is a property/band of a feature then visit the feature
	// to get the raster and georeferencing instead of what's done below...

#if 1
	// Instead of temporarily getting the raster from the ViewState we'll just create
	// a simple raster covering the entire globe.
	const unsigned int raster_width = 32;
	const unsigned int raster_height = 64;
	d_georeferencing = GPlatesPropertyValues::Georeferencing::create(raster_width, raster_height);

	// Create the raster data (just a checkerboard pattern).
	const GPlatesGui::rgba8_t red(255, 0, 0, 255);
	const GPlatesGui::rgba8_t blue(0, 0, 255, 255);
	GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type rgba8_raster =
			GPlatesPropertyValues::Rgba8RawRaster::create(raster_width, raster_height);
	GPlatesGui::rgba8_t *const rgba8_raster_buf = rgba8_raster->data();
	for (unsigned int j = 0; j < raster_height; ++j)
	{
		for (unsigned int i = 0; i < raster_width; ++i)
		{
			rgba8_raster_buf[j * raster_width + i] = (i ^ j) ? red : blue;
		}
	}

	d_raster = rgba8_raster;
#endif
}


std::vector<GPlatesAppLogic::Layer::input_channel_definition_type>
GPlatesAppLogic::RasterLayerTask::get_input_channel_definitions() const
{
	std::vector<Layer::input_channel_definition_type> input_channel_definitions;

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

	//
	// Get the polygon features collection input (used to reconstruct the raster).
	//
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> polygon_features_collection;
	extract_input_channel_data(
			polygon_features_collection,
			POLYGON_FEATURES_CHANNEL_NAME,
			input_data);


	// Create a reconstruction geometry collection to store the resolved raster in.
	//
	// NOTE: We're not using a reconstruction tree since we're not reconstructing anything
	// so just specify the default reconstruction tree.
	//
	// FIXME: Probably need to fix it so that reconstruction geometry's don't have to
	// specify a reconstruction tree (since not all reconstruction geometry's are actually
	// reconstructed - they might simply be resolved from a time-dependent property value).
	ReconstructionGeometryCollection::non_null_ptr_type reconstruction_geometry_collection =
			ReconstructionGeometryCollection::create(default_reconstruction_tree);

	// FIXME: When raster is a property/band of a feature then visit the feature
	// to get the raster and georeferencing instead of what's done below...
#if 1
	GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type georeferencing =
			d_georeferencing.get();
	GPlatesPropertyValues::RawRaster::non_null_ptr_type raster = d_raster.get();
#endif

	// Create a resolved raster.
	ResolvedRaster::non_null_ptr_type resolved_raster =
			ResolvedRaster::create(default_reconstruction_tree, georeferencing, raster);

	reconstruction_geometry_collection->add_reconstruction_geometry(resolved_raster);

	return layer_task_data_type(
			static_cast<ReconstructionGeometryCollection::non_null_ptr_to_const_type>(
					reconstruction_geometry_collection));
}
