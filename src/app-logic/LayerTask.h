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
 
#ifndef GPLATES_APP_LOGIC_LAYERTASK_H
#define GPLATES_APP_LOGIC_LAYERTASK_H

#include <map>
#include <utility>
#include <vector>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <QString>

#include "Layer.h"
#include "LayerTaskDataType.h"
#include "LayerTaskType.h"
#include "ReconstructionTree.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	/**
	 * Abstract interface for processing input feature collections, reconstructed geometries
	 * and/or reconstruction trees into a single output.
	 */
	class LayerTask
	{
	public:
		/**
		 * Typedef for input data in the form of a mappings of channel
		 * data objects belonging to the channel.
		 */
		typedef std::multimap<
				QString,
				const layer_task_data_type *> input_data_type;


		virtual
		~LayerTask()
		{ }


		/**
		 * Returns the channel name used by all layer tasks that require an input reconstruction tree.
		 *
		 * This is in the base layer task class so all layer tasks can access it.
		 */
		static
		const char *
		get_reconstruction_tree_channel_name();


		/**
		 * Returns the type of this layer task.
		 *
		 * This is useful for customising the visual representation of each type of
		 * layer task.
		 */
		virtual
		LayerTaskType::Type
		get_layer_type() const = 0;


		/**
		 * Returns the input channels expected by this task and the data types and arity
		 * for each channel.
		 */
		virtual
		std::vector<Layer::input_channel_definition_type>
		get_input_channel_definitions() const = 0;


		/**
		 * Returns the main input feature collection channel used by this layer task.
		 *
		 * This is the channel containing the feature collection(s) used to determine the
		 * layer tasks that are applicable to this layer.
		 *
		 * This can be used by the GUI to list available layer tasks to the user.
		 *
		 * NOTE: The data type of the input channel, as returned by @a get_input_channel_definitions,
		 * must be Layer::INPUT_FEATURE_COLLECTION_DATA.
		 */
		virtual
		QString
		get_main_input_feature_collection_channel() const = 0;


		/**
		 * Returns the data type that this task outputs.
		 */
		virtual
		Layer::LayerOutputDataType
		get_output_definition() const = 0;


		/**
		 * Returns true if this layer task is a topological layer task.
		 *
		 * A topological layer task is one that processes topological features which
		 * in turn reference reconstructed features (possibly from other layers that aren't
		 * connected to the topological layer).
		 */
		virtual
		bool
		is_topological_layer_task() const = 0;
		

		/**
		 * Execute this task by processing the specified input data and storing
		 * the result in the specified output data.
		 *
		 * NOTE: If processing cannot succeed, for example because the required inputs
		 * are not present, then false.
		 */
		virtual
		boost::optional<layer_task_data_type>
		process(
				const input_data_type &input_data,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchored_plate_id,
				const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree) = 0;

	protected:
		/**
		 * A utility function for derived classes to extract a specific bounded type
		 * from the variant @a layer_task_data_type objects in a channel into a container
		 * of the type ContainerType.
		 *
		 * The variant bounded type to be extracted is determined by the container's element type.
		 * The container type must support the 'push_back()' method.
		 *
		 * Example usage:
		 *   std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> feature_collections;
		 *   extract_input_channel_data(feature_collections, "reconstructable features", input_data);
		 */
		template <typename ContainerType>
		static
		void
		extract_input_channel_data(
				ContainerType &input_channel_data,
				const QString &input_channel_name,
				const input_data_type &input_data,
				// This function can only be called if the value_type of the
				// template parameter ContainerType is one of the bounded types
				// in layer_task_data_types.
				//
				// If the compiler can't find an appropriate overload of this
				// function, then make sure the type stored in your container
				// (in 'input_channel_data') is one of the types in layer_task_data_types.
				typename boost::enable_if<
					boost::mpl::contains<layer_task_data_types, typename ContainerType::value_type>
				>::type *dummy = NULL)
		{
			// Get range of data objects assigned to 'input_channel_name'.
			const std::pair<input_data_type::const_iterator, input_data_type::const_iterator>
					input_chanel_name_range = input_data.equal_range(input_channel_name);

			// Copy the data to caller's sequence.
			input_data_type::const_iterator data_iter = input_chanel_name_range.first;
			const input_data_type::const_iterator data_end = input_chanel_name_range.second;
			for ( ; data_iter != data_end; ++data_iter)
			{
				const layer_task_data_type &layer_data = *data_iter->second;

				// The type to be stored in the caller's container.
				typedef typename ContainerType::value_type variant_bounded_type;

				// Extract the specified bounded type from the variant.
				const variant_bounded_type *variant_bounded_data =
						boost::get<variant_bounded_type>(&layer_data);

				// If the bounded type expected is what is contained in the variant
				// then add to the caller's container. This should always be the case
				// unless the layer tasks were setup incorrectly.
				if (variant_bounded_data)
				{
					input_channel_data.push_back(*variant_bounded_data);
				}
			}
		}


		/**
		 * Extracts a reconstruction tree from the input channel
		 * 'get_reconstruction_tree_channel_name()' if there is one,
		 * otherwise returns @a default_reconstruction_tree.
		 *
		 * Returns false if more than one reconstruction is tree found in the channel.
		 */
		boost::optional<ReconstructionTree::non_null_ptr_to_const_type>
		extract_reconstruction_tree(
				const input_data_type &input_data,
				const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree);
	};
}


#endif // GPLATES_APP_LOGIC_LAYERTASK_H
