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
			 * Returns true if this task type can be used to create a layer automatically
			 * when a file is first loaded (without any user interaction).
			 *
			 * These task types should form an orthogonal set in that only one task from
			 * the primary set should be able to process any particular feature.
			 * This allows multiple layers to automatically be created for a single
			 * loaded file and have each feature in the file be processed by only one of the layers.
			 *
			 * The user can be allowed to select any task type for an existing layer though.
			 */
			bool
			is_primary_task_type() const;

			/**
			 * Creates an instance of a @a LayerTask from this layer task type.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			boost::shared_ptr<LayerTask>
			create_layer_task() const;


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
		 * Typedef for a function used to see if a @a LayerTask can process any features
		 * in a feature collection.
		 *
		 * Function takes a @a FeatureCollectionHandle weak_ref as an argument and
		 * returns a boolean.
		 */
		typedef boost::function< bool (const GPlatesModel::FeatureCollectionHandle::const_weak_ref &) >
				can_layer_task_process_feature_collection_function_type;

		/**
		 * Typedef for a function to return true if a layer task type is a primary task type.
		 *
		 * Function takes no arguments and returns a bool.
		 */
		typedef boost::function< bool () > is_primary_layer_task_function_type;


		/**
		 * Register a @a LayerTask type.
		 *
		 * This includes a function to create the @a LayerTask derived type and
		 * a function to determine whether that type will be able to process
		 * features in specific feature collections.
		 */
		LayerTaskType
		register_layer_task_type(
				const create_layer_task_function_type &create_layer_task_function,
				const can_layer_task_process_feature_collection_function_type &
						can_layer_task_process_feature_collection_function,
				const is_primary_layer_task_function_type &is_primary_layer_task_function);


		/**
		 * Unregisters @a LayerTask type.
		 */
		void
		unregister_layer_task_type(
				const LayerTaskType &);


		/**
		 * Sets the layer task type to use when no registered layer task types can process
		 * a feature collection.
		 *
		 * This method must be called before @a get_layer_task_types_that_can_process_feature_collection
		 * or @a get_layer_task_types_that_can_process_feature_collections can be called.
		 *
		 * @throws PreconditionViolationError if @a layer_task_type is not a primary task type.
		 */
		void
		set_default_primary_layer_task_type(
				const LayerTaskType &layer_task_type);


		/**
		 * Returns a sequence of all registered @a LayerTask types.
		 */
		std::vector<LayerTaskType>
		get_all_layer_task_types() const;


		/**
		 * Returns a sequence of @a LayerTask types that can process @a feature_collection.
		 *
		 * If no registered layer task types can process @a feature_collection then the
		 * default layer task type (set by @a set_default_primary_layer_task_type) is returned.
		 * In this case the default layer task type (being one of the registered types already
		 * tested on @a feature_collection) will not be able to process @a feature_collection,
		 * in which case the layer will generate no output or output that is not reconstructed
		 * (for example, because there were no plate ids in the features).
		 *
		 * This is useful for a GUI that gives the user a list of compatible layer tasks
		 * they can use to process a feature collection loaded from a file.
		 *
		 * @throws PreconditionViolationError if @a set_default_primary_layer_task_type has not been called.
		 * This is to ensure that the returned vector of layer task types is never empty.
		 */
		std::vector<LayerTaskType>
		get_layer_task_types_that_can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection) const;


		/**
		 * Returns a sequence of @a LayerTask types that can process all feature collections
		 * in @a feature_collections.
		 *
		 * This is useful for a GUI that gives the user a list of compatible layer tasks
		 * they can use to process feature collections loaded from files.
		 *
		 * @throws PreconditionViolationError if @a set_default_primary_layer_task_type has not been called.
		 * This is to ensure that the returned vector of layer task types is never empty.
		 */
		std::vector<LayerTaskType>
		get_layer_task_types_that_can_process_feature_collections(
				const std::vector<GPlatesModel::FeatureCollectionHandle::const_weak_ref> &feature_collections) const;

	private:
		/**
		 * Contains layer-task-specific functions provided by the client.
		 */
		class LayerTaskTypeInfo
		{
		public:
			LayerTaskTypeInfo(
					const create_layer_task_function_type &create_layer_task_function_,
					const can_layer_task_process_feature_collection_function_type &
							can_layer_task_process_feature_collection_function_,
					const is_primary_layer_task_function_type &is_primary_layer_task_function_);

			create_layer_task_function_type create_layer_task_function;

			can_layer_task_process_feature_collection_function_type 
					can_layer_task_process_feature_collection_function;

			is_primary_layer_task_function_type is_primary_layer_task_function;
		};

		//! Typedef for a sequence of layer task types.
		typedef std::list< boost::shared_ptr<const LayerTaskTypeInfo> > layer_task_type_seq_type;


		layer_task_type_seq_type d_layer_task_types;

		/**
		 * The default layer task type to use when no layer tasks can process a feature collection.
		 */
		LayerTaskType d_default_layer_task_type;
	};


	/**
	 * Register the default layer tasks with @a layer_task_registry.
	 *
	 * One of the registered layers will also be set as the default layer that will be
	 * used when no layers can be found to process a feature collection.
	 * This default catch-all layer task will be the reconstruct layer task as it
	 * is the most common layer task and it is also a primary layer task (ie, can be
	 * used to automatically create a layer when a file is first loaded).
	 *
	 * NOTE: any new @a LayerTask derived types needs to have a registration entry
	 * added inside @a register_layer_task_types.
	 */
	void
	register_default_layer_task_types(
			LayerTaskRegistry &layer_task_registry);
}

#endif // GPLATES_APP_LOGIC_LAYERTASKREGISTRY_H
