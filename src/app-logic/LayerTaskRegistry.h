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
#include <utility>
#include <vector>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <QString>

#include "LayerTaskType.h"

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
		//! Forward declaration of a private nested class type.
		class LayerTaskTypeInfo;

	public:
		/**
		 * Wrapper around a layer task type.
		 */
		class LayerTaskType
		{
		public:
			explicit
			LayerTaskType(
					const boost::weak_ptr<const LayerTaskTypeInfo> &impl =
							boost::weak_ptr<const LayerTaskTypeInfo>()) :
				d_impl(impl)
			{  }

			/**
			 * Returns true if this layer task type is still valid and has not been unregistered.
			 */
			bool
			is_valid() const
			{
				return !d_impl.expired();
			}

			/**
			 * Creates an instance of a @a LayerTask from this layer task type.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			boost::shared_ptr<LayerTask>
			create_layer_task() const;

			/**
			 * Returns the type of the layer task as a member of an enumeration.
			 */
			GPlatesAppLogic::LayerTaskType::Type
			get_layer_type() const;

			//! Used by implementation.
			const boost::weak_ptr<const LayerTaskTypeInfo> &
			get_impl() const
			{
				return d_impl;
			}

		private:
			boost::weak_ptr<const LayerTaskTypeInfo> d_impl;
		};


		/**
		 * Typedef for a function to create a @a LayerTask.
		 *
		 * Function takes no arguments and returns a shared_ptr to a @a LayerTask.
		 */
		typedef boost::function< boost::shared_ptr<LayerTask> () > create_layer_task_function_type;

		/**
		 * Typedef for a function used to see if a @a LayerTask should be auto-created to
		 * process a feature collection when it is loaded.
		 *
		 * Function takes a @a FeatureCollectionHandle weak_ref as an argument and
		 * returns a boolean.
		 */
		typedef boost::function< bool (const GPlatesModel::FeatureCollectionHandle::const_weak_ref &) >
				should_auto_create_layer_task_for_loaded_file_function_type;


		/**
		 * Register a @a LayerTask type.
		 *
		 * This includes a function to create the @a LayerTask derived type and
		 * a function to determine whether that type should be auto-created when
		 * a feature collection is loaded.
		 */
		LayerTaskType
		register_layer_task_type(
				const create_layer_task_function_type &create_layer_task_function,
				const should_auto_create_layer_task_for_loaded_file_function_type &
						should_auto_create_layer_task_for_loaded_file_function,
				GPlatesAppLogic::LayerTaskType::Type layer_type);


		/**
		 * Unregisters @a LayerTask type.
		 */
		void
		unregister_layer_task_type(
				const LayerTaskType &);


		/**
		 * Returns a sequence of all registered @a LayerTask types.
		 */
		std::vector<LayerTaskType>
		get_all_layer_task_types() const;


		/**
		 * Returns a sequence of @a LayerTask types that should be created automatically,
		 * as opposed to manually created by the user, as a result of @a feature_collection
		 * having been loaded.
		 */
		std::vector<LayerTaskType>
		get_layer_task_types_to_auto_create_for_loaded_file(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection) const;

	private:
		/**
		 * Contains layer-task-specific functions provided by the client.
		 */
		class LayerTaskTypeInfo
		{
		public:
			LayerTaskTypeInfo(
					const create_layer_task_function_type &create_layer_task_function_,
					const should_auto_create_layer_task_for_loaded_file_function_type &
							can_layer_task_process_feature_collection_function_,
					GPlatesAppLogic::LayerTaskType::Type layer_type_);

			create_layer_task_function_type create_layer_task_function;

			should_auto_create_layer_task_for_loaded_file_function_type 
					should_auto_create_layer_task_for_loaded_file_function;

			GPlatesAppLogic::LayerTaskType::Type layer_type;
		};

		//! Typedef for a sequence of layer task types.
		typedef std::list< boost::shared_ptr<const LayerTaskTypeInfo> > layer_task_type_seq_type;


		layer_task_type_seq_type d_layer_task_types;
	};


	/**
	 * Register the default layer tasks with @a layer_task_registry.
	 *
	 * NOTE: any new @a LayerTask derived types needs to have a registration entry
	 * added inside @a register_layer_task_types.
	 */
	void
	register_default_layer_task_types(
			LayerTaskRegistry &layer_task_registry);
}

#endif // GPLATES_APP_LOGIC_LAYERTASKREGISTRY_H
