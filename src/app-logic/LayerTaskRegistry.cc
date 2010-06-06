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

#include <boost/foreach.hpp>

#include "LayerTaskRegistry.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::LayerTaskRegistry::LayerTaskType
GPlatesAppLogic::LayerTaskRegistry::register_layer_task_type(
		const create_layer_task_function_type &create_layer_task_function,
		const can_layer_task_process_feature_collection_function_type &
				can_layer_task_process_feature_collection_function,
		const get_layer_task_name_and_description_function_type &
				get_layer_task_name_and_description_function,
		const is_primary_layer_task_function_type &is_primary_layer_task_function)
{
	const boost::shared_ptr<const LayerTaskTypeInfo> layer_task_type(
			new LayerTaskTypeInfo(
					create_layer_task_function,
					can_layer_task_process_feature_collection_function,
					get_layer_task_name_and_description_function,
					is_primary_layer_task_function));

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


void
GPlatesAppLogic::LayerTaskRegistry::set_default_primary_layer_task_type(
		const LayerTaskType &layer_task_type)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		layer_task_type.is_valid(),
		GPLATES_ASSERTION_SOURCE);

	// Default layer task type must be a primary task type.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		layer_task_type.is_primary_task_type(),
		GPLATES_ASSERTION_SOURCE);

	d_default_layer_task_type = layer_task_type;
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
GPlatesAppLogic::LayerTaskRegistry::get_layer_task_types_that_can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_default_layer_task_type.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	std::vector<LayerTaskType> filtered_layer_task_types;

	// Iterate over the registered layer tasks.
	layer_task_type_seq_type::const_iterator layer_task_types_iter = d_layer_task_types.begin();
	const layer_task_type_seq_type::const_iterator layer_task_types_end = d_layer_task_types.end();
	for ( ; layer_task_types_iter != layer_task_types_end; ++layer_task_types_iter)
	{
		const boost::shared_ptr<const LayerTaskTypeInfo> &layer_task_type = *layer_task_types_iter;

		if (layer_task_type->can_layer_task_process_feature_collection_function(feature_collection))
		{
			filtered_layer_task_types.push_back(LayerTaskType(layer_task_type));
		}
	}

	// Use the default layer task type if no other types can be found.
	if (filtered_layer_task_types.empty())
	{
		filtered_layer_task_types.push_back(d_default_layer_task_type);
	}

	return filtered_layer_task_types;
}


std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType>
GPlatesAppLogic::LayerTaskRegistry::get_layer_task_types_that_can_process_feature_collections(
		const std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref> &feature_collections) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_default_layer_task_type.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	std::vector<LayerTaskType> filtered_layer_task_types;

	// Iterate over the registered layer tasks.
	layer_task_type_seq_type::const_iterator layer_task_types_iter = d_layer_task_types.begin();
	const layer_task_type_seq_type::const_iterator layer_task_types_end = d_layer_task_types.end();
	for ( ; layer_task_types_iter != layer_task_types_end; ++layer_task_types_iter)
	{
		const boost::shared_ptr<const LayerTaskTypeInfo> &layer_task_type = *layer_task_types_iter;

		bool layer_task_can_process_all_feature_collections = true;

		// Iterate over the feature collections.
		BOOST_FOREACH(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
				feature_collections)
		{
			if (!layer_task_type->can_layer_task_process_feature_collection_function(feature_collection))
			{
				layer_task_can_process_all_feature_collections = false;
				break;
			}
		}

		if (layer_task_can_process_all_feature_collections)
		{
			filtered_layer_task_types.push_back(LayerTaskType(layer_task_type));
		}
	}

	// Use the default layer task type if no other types can be found.
	if (filtered_layer_task_types.empty())
	{
		filtered_layer_task_types.push_back(d_default_layer_task_type);
	}

	return filtered_layer_task_types;
}


std::pair<QString, QString>
GPlatesAppLogic::LayerTaskRegistry::LayerTaskType::get_layer_task_name_and_description() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	const boost::shared_ptr<const LayerTaskTypeInfo> layer_task_type(d_impl);

	return layer_task_type->get_layer_task_name_and_description_function();
}


bool
GPlatesAppLogic::LayerTaskRegistry::LayerTaskType::is_primary_task_type() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	const boost::shared_ptr<const LayerTaskTypeInfo> layer_task_type(d_impl);

	return layer_task_type->is_primary_layer_task_function();
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


GPlatesAppLogic::LayerTaskRegistry::LayerTaskTypeInfo::LayerTaskTypeInfo(
		const create_layer_task_function_type &create_layer_task_function_,
		const can_layer_task_process_feature_collection_function_type &
				can_layer_task_process_feature_collection_function_,
		const get_layer_task_name_and_description_function_type &get_layer_task_description_function_,
		const is_primary_layer_task_function_type &is_primary_layer_task_function_) :
	create_layer_task_function(create_layer_task_function_),
	can_layer_task_process_feature_collection_function(
			can_layer_task_process_feature_collection_function_),
	get_layer_task_name_and_description_function(get_layer_task_description_function_),
	is_primary_layer_task_function(is_primary_layer_task_function_)
{
}
