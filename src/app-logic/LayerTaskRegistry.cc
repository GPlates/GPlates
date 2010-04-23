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

#include "LayerTaskRegistry.h"


GPlatesAppLogic::LayerTaskRegistry::layer_task_type_id_type
GPlatesAppLogic::LayerTaskRegistry::register_layer_task_type(
		const create_layer_task_function_type &create_layer_task_function,
		const can_layer_task_process_feature_collection_function_type &
				can_layer_task_process_feature_collection_function)
{
	boost::shared_ptr<LayerTaskType> layer_task_type(
			new LayerTaskType(
					create_layer_task_function,
					can_layer_task_process_feature_collection_function));

	d_layer_task_types.push_back(layer_task_type);

	return layer_task_type_id_type(layer_task_type);
}


void
GPlatesAppLogic::LayerTaskRegistry::unregister_layer_task_type(
		const layer_task_type_id_type &layer_task_type_id)
{
	// Throws 'bad_weak_ptr' if 'layer_task_type_id' is invalid meaning the client
	// passed in ids that refer to layer task types that have already been deleted.
	boost::shared_ptr<const LayerTaskType> layer_task_type(layer_task_type_id);

	d_layer_task_types.remove(layer_task_type);
}


std::vector<GPlatesAppLogic::LayerTaskRegistry::layer_task_type_id_type>
GPlatesAppLogic::LayerTaskRegistry::get_all_layer_task_types() const
{
	return std::vector<layer_task_type_id_type>(
			d_layer_task_types.begin(),
			d_layer_task_types.end());
}


std::vector<GPlatesAppLogic::LayerTaskRegistry::layer_task_type_id_type>
GPlatesAppLogic::LayerTaskRegistry::get_layer_task_types_that_can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection) const
{
	std::vector<layer_task_type_id_type> filtered_layer_task_types;

	layer_task_type_seq_type::const_iterator layer_task_types_iter = d_layer_task_types.begin();
	const layer_task_type_seq_type::const_iterator layer_task_types_end = d_layer_task_types.end();
	for ( ; layer_task_types_iter != layer_task_types_end; ++layer_task_types_iter)
	{
		const boost::shared_ptr<const LayerTaskType> &layer_task_type = *layer_task_types_iter;

		if (layer_task_type->can_layer_task_process_feature_collection_function(feature_collection))
		{
			filtered_layer_task_types.push_back(layer_task_type_id_type(layer_task_type));
		}
	}

	return filtered_layer_task_types;
}


boost::shared_ptr<GPlatesAppLogic::LayerTask>
GPlatesAppLogic::LayerTaskRegistry::create_layer_task(
		const layer_task_type_id_type &layer_task_type_id)
{
	// Throws 'bad_weak_ptr' if 'layer_task_type_id' is invalid meaning the client
	// passed in ids that refer to layer task types that have already been deleted.
	boost::shared_ptr<const LayerTaskType> layer_task_type(layer_task_type_id);

	return layer_task_type->create_layer_task_function();
}


GPlatesAppLogic::LayerTaskRegistry::LayerTaskType::LayerTaskType(
		const create_layer_task_function_type &create_layer_task_function_,
		const can_layer_task_process_feature_collection_function_type &
				can_layer_task_process_feature_collection_function_) :
	create_layer_task_function(create_layer_task_function_),
	can_layer_task_process_feature_collection_function(
			can_layer_task_process_feature_collection_function_)
{
}
