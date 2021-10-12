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
 
#ifndef GPLATES_APP_LOGIC_LAYERTASKREGISTRY_H
#define GPLATES_APP_LOGIC_LAYERTASKREGISTRY_H

#include <list>
#include <vector>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	class LayerTask;

	/**
	 * Manages registration of functions used to create @a LayerTask types and
	 * handles calling those functions to create the @a LayerTask objects.
	 */
	class LayerTaskRegistry
	{
	private:
		//! This nested class is meant to be opaque.
		class LayerTaskType;

	public:
		/**
		 * Typedef for a handle to (information about) a type of @a LayerTask.
		 */
		typedef boost::weak_ptr<const LayerTaskType> layer_task_type_id_type;

		/**
		 * Typedef for a function to create a @a LayerTask.
		 *
		 * Function takes no arguments and returns a shared_ptr to a @a LayerTask.
		 */
		typedef boost::function< boost::shared_ptr<LayerTask> () > create_layer_task_function_type;

		/**
		 * Typedef for a function used to see if a @a LayerTask can process any features
		 * in a feature collection.
		 *
		 * Function takes a @a FeatureCollectionHandle weak_ref as an argument and
		 * returns a boolean.
		 */
		typedef boost::function< bool (const GPlatesModel::FeatureCollectionHandle::weak_ref &) >
				can_layer_task_process_feature_collection_function_type;


		/**
		 * Register a @a LayerTask type.
		 *
		 * This includes a function to create the @a LayerTask derived type and
		 * a function to determine whether that type will be able to process
		 * features in specific feature collections.
		 */
		layer_task_type_id_type
		register_layer_task_type(
				const create_layer_task_function_type &create_layer_task_function,
				const can_layer_task_process_feature_collection_function_type &
						can_layer_task_process_feature_collection_function);


		/**
		 * Unregisters @a LayerTask type.
		 */
		void
		unregister_layer_task_type(
				const layer_task_type_id_type &);


		/**
		 * Returns a sequence of all registered @a LayerTask types.
		 */
		std::vector<layer_task_type_id_type>
		get_all_layer_task_types() const;


		/**
		 * Returns a sequence of @a LayerTask types that can process @a feature_collection.
		 *
		 * This is useful for a GUI that gives the user a list of compatible layer tasks
		 * they can use to process a feature collection loaded from a file.
		 */
		std::vector<layer_task_type_id_type>
		get_layer_task_types_that_can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection) const;


		/**
		 * Creates an instance of a @a LayerTask type using @a layer_task_type_id to
		 * identify the type.
		 */
		boost::shared_ptr<LayerTask>
		create_layer_task(
				const layer_task_type_id_type &layer_task_type_id);

	private:
		/**
		 * Contains layer-task-specific functions provided by the client.
		 */
		class LayerTaskType
		{
		public:
			LayerTaskType(
					const create_layer_task_function_type &create_layer_task_function_,
					const can_layer_task_process_feature_collection_function_type &
							can_layer_task_process_feature_collection_function_);

			create_layer_task_function_type create_layer_task_function;

			can_layer_task_process_feature_collection_function_type 
					can_layer_task_process_feature_collection_function;
		};

		typedef std::list< boost::shared_ptr<const LayerTaskType> > layer_task_type_seq_type;


		layer_task_type_seq_type d_layer_task_types;
	};
}

#endif // GPLATES_APP_LOGIC_LAYERTASKREGISTRY_H
