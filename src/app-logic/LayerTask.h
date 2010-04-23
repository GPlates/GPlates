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
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <boost/variant.hpp>

#include "ReconstructGraph.h"

#include "model/FeatureCollectionHandle.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"


namespace GPlatesAppLogic
{
	/**
	 * The data type that is input to or output from a layer.
	 *
	 * The three possible types are:
	 * 1) feature collection - typically used as the first level of input
	 *    to layers in the graph,
	 * 2) reconstruction geometries - typically output by layers and can be used
	 *    as inputs to other connected layers,
	 * 3) reconstruction tree - typically output by a layer that converts rotation
	 *    features (total reconstruction sequences) to a rotation tree that is used
	 *    as input to other layers for reconstruction purposes.
	 *
	 * NOTE: Keep the feature collection weak ref as the first bounded type because
	 * it is default-constructable (boost::variant default-constructs its first bounded type).
	 */
	typedef boost::variant<
			GPlatesModel::FeatureCollectionHandle::weak_ref,
			GPlatesModel::Reconstruction::non_null_ptr_type,
			GPlatesModel::ReconstructionTree::non_null_ptr_type
	> layer_data_type;


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
				ReconstructGraph::layer_input_channel_name_type,
				const layer_data_type *> input_data_type;


		virtual
		~LayerTask()
		{ }


		/**
		 * Returns the input channels expected by this task and the data types and arity
		 * for each channel.
		 */
		virtual
		std::vector<ReconstructGraph::input_channel_definition_type>
		get_input_channel_definitions() const = 0;


		/**
		 * Returns the data type that this task outputs.
		 */
		virtual
		ReconstructGraph::DataType
		get_output_definition() const = 0;


		/**
		 * Execute this task by processing the specified input data and storing
		 * the result in the specified output data.
		 *
		 * If the output is not written to, for example because the required inputs
		 * are not present, then returns false.
		 */
		virtual
		bool
		process(
				const input_data_type &input_data,
				layer_data_type &output_data,
				const double &reconstruction_time) = 0;

	protected:
		/**
		 * A utility function for derived classes to extract a specific bounded type
		 * from the variant @a layer_data_type objects in a channel into a container
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
				const ReconstructGraph::layer_input_channel_name_type &input_channel_name,
				const input_data_type &input_data)
		{
			// Get range of data objects assigned to 'input_channel_name'.
			const std::pair<input_data_type::const_iterator, input_data_type::const_iterator>
					input_chanel_name_range = input_data.equal_range(input_channel_name);

			// Copy the data to caller's sequence.
			input_data_type::const_iterator data_iter = input_chanel_name_range.first;
			const input_data_type::const_iterator data_end = input_chanel_name_range.second;
			for ( ; data_iter != data_end; ++data_iter)
			{
				const layer_data_type &layer_data = *data_iter->second;

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
	};
}


#endif // GPLATES_APP_LOGIC_LAYERTASK_H
