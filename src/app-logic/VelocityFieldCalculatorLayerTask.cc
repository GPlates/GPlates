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

#include "VelocityFieldCalculatorLayerTask.h"
#include "TopologyBoundaryResolverLayerTask.h"

#include "AppLogicUtils.h"
#include "PlateVelocityUtils.h"


const char *GPlatesAppLogic::VelocityFieldCalculatorLayerTask::MESH_POINT_FEATURES_CHANNEL_NAME =
		"Mesh-point features";

const char *GPlatesAppLogic::VelocityFieldCalculatorLayerTask::POLYGON_FEATURES_CHANNEL_NAME =
		"Polygon features";


bool
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return PlateVelocityUtils::detect_velocity_mesh_nodes(feature_collection);
}


std::vector<GPlatesAppLogic::Layer::input_channel_definition_type>
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_input_channel_definitions() const
{
	std::vector<Layer::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction tree.
	input_channel_definitions.push_back(
			boost::make_tuple(
					get_reconstruction_tree_channel_name(),
					Layer::INPUT_RECONSTRUCTION_TREE_DATA,
					Layer::ONE_DATA_IN_CHANNEL));

	// Channel definition for the polygon features.
	input_channel_definitions.push_back(
			boost::make_tuple(
					POLYGON_FEATURES_CHANNEL_NAME,
					Layer::INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));

	// Channel definition for the mesh-point features.
	input_channel_definitions.push_back(
			boost::make_tuple(
					MESH_POINT_FEATURES_CHANNEL_NAME,
					Layer::INPUT_FEATURE_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));
	
	return input_channel_definitions;
}


QString
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_main_input_feature_collection_channel() const
{
	return MESH_POINT_FEATURES_CHANNEL_NAME;
}


GPlatesAppLogic::Layer::LayerOutputDataType
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::get_output_definition() const
{
	return Layer::OUTPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA;
}


boost::optional<GPlatesAppLogic::layer_task_data_type>
GPlatesAppLogic::VelocityFieldCalculatorLayerTask::process(
		const Layer &layer_handle /* the layer invoking this */,
		const input_data_type &input_data,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree)
{
	// Get the reconstruction tree input.
	boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree =
			extract_reconstruction_tree(
					input_data,
					default_reconstruction_tree);
	if (!reconstruction_tree)
	{
		// Expecting a single reconstruction tree.
		return boost::none;
	}

	// Get the polygon features collection input.
	std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type> reconstructed_polygons;
	extract_input_channel_data(
			reconstructed_polygons,
			POLYGON_FEATURES_CHANNEL_NAME,
			input_data);
	// For now, let's expect exactly ONE (not more, not less) ReconstructionGeometryCollection
	// of polygons.
	if (reconstructed_polygons.empty()) {
		return boost::none;
	}
	// else, we'll ignore any after the first.

	// Get the mesh-point features collection input.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> mesh_point_feature_collections;
	extract_input_channel_data(
			mesh_point_feature_collections,
			MESH_POINT_FEATURES_CHANNEL_NAME,
			input_data);

	// Calculate the velocity fields.
	const ReconstructionGeometryCollection::non_null_ptr_to_const_type
			velocity_fields =
					PlateVelocityUtils::solve_velocities(
							*reconstruction_tree,
							reconstruction_time,
							anchored_plate_id,
							mesh_point_feature_collections,
							*reconstructed_polygons.front());

	// Return the velocity fields.
	return layer_task_data_type(velocity_fields);
}
