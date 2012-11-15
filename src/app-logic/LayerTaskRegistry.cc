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

#include "LayerTaskRegistry.h"

#include "ApplicationState.h"
#include "CoRegistrationLayerTask.h"
#include "LayerTask.h"
#include "LayerTaskRegistry.h"
#include "RasterLayerTask.h"
#include "ReconstructionLayerTask.h"
#include "ReconstructLayerTask.h"
#include "ScalarField3DLayerTask.h"
#include "TopologyGeometryResolverLayerTask.h"
#include "TopologyNetworkResolverLayerTask.h"
#include "VelocityFieldCalculatorLayerTask.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include <boost/foreach.hpp>

GPlatesAppLogic::LayerTaskRegistry::LayerTaskType
GPlatesAppLogic::LayerTaskRegistry::register_layer_task_type(
		const create_layer_task_function_type &create_layer_task_function,
		const should_auto_create_layer_task_for_loaded_file_function_type &
				should_auto_create_layer_task_for_loaded_file_function,
		GPlatesAppLogic::LayerTaskType::Type layer_type)
{
	const boost::shared_ptr<const LayerTaskTypeInfo> layer_task_type(
			new LayerTaskTypeInfo(
					create_layer_task_function,
					should_auto_create_layer_task_for_loaded_file_function,
					layer_type));

	d_layer_task_types.push_back(layer_task_type);

	return LayerTaskType(layer_task_type);
}


void
GPlatesAppLogic::LayerTaskRegistry::unregister_layer_task_type(
		const LayerTaskType &layer_task_type_weak_ref)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		layer_task_type_weak_ref.is_valid(),
		GPLATES_ASSERTION_SOURCE);

	const boost::shared_ptr<const LayerTaskTypeInfo> layer_task_type(
			layer_task_type_weak_ref.get_impl());

	d_layer_task_types.remove(layer_task_type);
}


std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType>
GPlatesAppLogic::LayerTaskRegistry::get_all_layer_task_types() const
{
	std::vector<LayerTaskType> layer_task_types;

	BOOST_FOREACH(
			const boost::shared_ptr<const LayerTaskTypeInfo> &layer_task_type,
			d_layer_task_types)
	{
		layer_task_types.push_back(LayerTaskType(layer_task_type));
	}

	return layer_task_types;
}


std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType>
GPlatesAppLogic::LayerTaskRegistry::get_layer_task_types_to_auto_create_for_loaded_file(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection) const
{
	std::vector<LayerTaskType> filtered_layer_task_types;

	// Iterate over the registered layer tasks.
	layer_task_type_seq_type::const_iterator layer_task_types_iter = d_layer_task_types.begin();
	const layer_task_type_seq_type::const_iterator layer_task_types_end = d_layer_task_types.end();
	for ( ; layer_task_types_iter != layer_task_types_end; ++layer_task_types_iter)
	{
		const boost::shared_ptr<const LayerTaskTypeInfo> &layer_task_type = *layer_task_types_iter;

		if (layer_task_type->should_auto_create_layer_task_for_loaded_file_function(feature_collection))
		{
			filtered_layer_task_types.push_back(LayerTaskType(layer_task_type));
		}
	}

	return filtered_layer_task_types;
}


boost::shared_ptr<GPlatesAppLogic::LayerTask>
GPlatesAppLogic::LayerTaskRegistry::LayerTaskType::create_layer_task() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	const boost::shared_ptr<const LayerTaskTypeInfo> layer_task_type(d_impl);

	return layer_task_type->create_layer_task_function();
}


GPlatesAppLogic::LayerTaskType::Type
GPlatesAppLogic::LayerTaskRegistry::LayerTaskType::get_layer_type() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	const boost::shared_ptr<const LayerTaskTypeInfo> layer_task_type(d_impl);

	return layer_task_type->layer_type;
}


GPlatesAppLogic::LayerTaskRegistry::LayerTaskTypeInfo::LayerTaskTypeInfo(
		const create_layer_task_function_type &create_layer_task_function_,
		const should_auto_create_layer_task_for_loaded_file_function_type &
				should_auto_create_layer_task_for_loaded_file_function_,
		GPlatesAppLogic::LayerTaskType::Type layer_type_) :
	create_layer_task_function(create_layer_task_function_),
	should_auto_create_layer_task_for_loaded_file_function(
			should_auto_create_layer_task_for_loaded_file_function_),
	layer_type(layer_type_)
{
}


void
GPlatesAppLogic::register_default_layer_task_types(
		LayerTaskRegistry &layer_task_registry,
		ApplicationState &application_state)
{
	//
	// NOTE: The order in which layer tasks are registered does *not* matter.
	//

	// Layer task that generates reconstruction trees.
	layer_task_registry.register_layer_task_type(
			&ReconstructionLayerTask::create_layer_task,
			&ReconstructionLayerTask::can_process_feature_collection,
			GPlatesAppLogic::LayerTaskType::RECONSTRUCTION);

	// Layer task that reconstructs geometries.
	const LayerTaskRegistry::LayerTaskType reconstruct_layer_task_type =
			layer_task_registry.register_layer_task_type(
					boost::bind(&ReconstructLayerTask::create_layer_task,
							boost::ref(application_state)),
					boost::bind(&ReconstructLayerTask::can_process_feature_collection,
							_1,
							boost::ref(application_state)),
					GPlatesAppLogic::LayerTaskType::RECONSTRUCT);

	// Layer task that reconstructs rasters.
	layer_task_registry.register_layer_task_type(
			&RasterLayerTask::create_layer_task,
			&RasterLayerTask::can_process_feature_collection,
			GPlatesAppLogic::LayerTaskType::RASTER);

	// Layer task that reconstructs rasters.
	layer_task_registry.register_layer_task_type(
			&ScalarField3DLayerTask::create_layer_task,
			&ScalarField3DLayerTask::can_process_feature_collection,
			GPlatesAppLogic::LayerTaskType::SCALAR_FIELD_3D);

	// Layer task to resolve topological geometries.
	layer_task_registry.register_layer_task_type(
			&TopologyGeometryResolverLayerTask::create_layer_task,
			&TopologyGeometryResolverLayerTask::can_process_feature_collection,
			GPlatesAppLogic::LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER);

	// Layer task to resolve topological networks.
	layer_task_registry.register_layer_task_type(
			&TopologyNetworkResolverLayerTask::create_layer_task,
			&TopologyNetworkResolverLayerTask::can_process_feature_collection,
			GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER);

	// Layer task to calculate velocity fields.
	layer_task_registry.register_layer_task_type(
			&VelocityFieldCalculatorLayerTask::create_layer_task,
			&VelocityFieldCalculatorLayerTask::can_process_feature_collection,
			GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR);

	// Layer task to do co-registration.
	layer_task_registry.register_layer_task_type(
			&CoRegistrationLayerTask::create_layer_task,
			&CoRegistrationLayerTask::can_process_feature_collection,
			GPlatesAppLogic::LayerTaskType::CO_REGISTRATION);
}
