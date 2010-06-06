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

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTaskTypes.h"

#include "LayerTask.h"
#include "LayerTaskRegistry.h"
#include "ReconstructionLayerTask.h"
#include "ReconstructLayerTask.h"
#include "TopologyBoundaryResolverLayerTask.h"
#include "TopologyNetworkResolverLayerTask.h"

#include "model/FeatureCollectionHandle.h"


void
GPlatesAppLogic::LayerTaskTypes::register_layer_task_types(
		LayerTaskRegistry &layer_task_registry,
		const ApplicationState &application_state)
{
	// Layer task that generates reconstruction trees.
	layer_task_registry.register_layer_task_type(
			&ReconstructionLayerTask::create_layer_task,
			&ReconstructionLayerTask::can_process_feature_collection,
			&ReconstructionLayerTask::get_name_and_description,
			&ReconstructionLayerTask::is_primary_layer_task_type);

	// Layer task that reconstructs geometries.
	const LayerTaskRegistry::LayerTaskType reconstruct_layer_task_type =
			layer_task_registry.register_layer_task_type(
					&ReconstructLayerTask::create_layer_task,
					&ReconstructLayerTask::can_process_feature_collection,
					&ReconstructLayerTask::get_name_and_description,
					&ReconstructLayerTask::is_primary_layer_task_type);

	// Layer task to resolve topological closed plate boundaries.
	layer_task_registry.register_layer_task_type(
			&TopologyBoundaryResolverLayerTask::create_layer_task,
			&TopologyBoundaryResolverLayerTask::can_process_feature_collection,
			&TopologyBoundaryResolverLayerTask::get_name_and_description,
			&TopologyBoundaryResolverLayerTask::is_primary_layer_task_type);

	// Layer task to resolve topological networks.
	layer_task_registry.register_layer_task_type(
			&TopologyNetworkResolverLayerTask::create_layer_task,
			&TopologyNetworkResolverLayerTask::can_process_feature_collection,
			&TopologyNetworkResolverLayerTask::get_name_and_description,
			&TopologyNetworkResolverLayerTask::is_primary_layer_task_type);

	//
	// Set the layer task type to use when no registered layer task types can process
	// a feature collection.
	//
	// NOTE: This must be a primary layer task type so that when a file is first loaded
	// a layer can always be created automatically for it. An exception is thrown if it's not.
	//

	layer_task_registry.set_default_primary_layer_task_type(reconstruct_layer_task_type);
}
