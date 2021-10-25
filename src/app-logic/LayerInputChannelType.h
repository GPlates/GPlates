/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_LAYERCONNECTIONTYPE_H
#define GPLATES_APP_LOGIC_LAYERCONNECTIONTYPE_H

#include <vector>
#include <boost/optional.hpp>

#include "LayerInputChannelName.h"
#include "LayerTaskType.h"


namespace GPlatesAppLogic
{
	/**
	 * Information describing the input data types and arity allowed for a single input channel.
	 *
	 * The two types of input data are:
	 * - an input feature collection, or
	 * - the output of another layer.
	 */
	class LayerInputChannelType
	{
	public:

		/**
		 * Represents the number of data inputs allowed by a specific input channel of a layer.
		 *
		 * A layer can have one or more input channels representing different classifications
		 * of input data and each channel can have one or more data objects.
		 * The latter is what's determined here.
		 *
		 * For example the reconstruct layer has a "rotation tree" input channel and a
		 * "reconstructable features" input channel.
		 * In the "reconstructable features" channel there can be multiple feature collections
		 * but in the "rotation tree" channel there can only be one reconstruction tree.
		 */
		enum ChannelDataArity
		{
			ONE_DATA_IN_CHANNEL,
			MULTIPLE_DATAS_IN_CHANNEL
		};


		/**
		 * Represents whether, and how, to auto connect to an input layer.
		 */
		enum AutoConnect
		{
			DONT_AUTO_CONNECT,
			// The layer can only auto connect to another layer associated with the same main input file...
			LOCAL_AUTO_CONNECT,
			// The layer can auto connect to layers associated with any input file...
			GLOBAL_AUTO_CONNECT
		};

		/**
		 * Associates a layer input type with its auto-connect capability.
		 */
		struct InputLayerType
		{
			InputLayerType(
					LayerTaskType::Type layer_type_,
					AutoConnect auto_connect_ = DONT_AUTO_CONNECT) :
				layer_type(layer_type_),
				auto_connect(auto_connect_)
			{  }

			LayerTaskType::Type layer_type;
			AutoConnect auto_connect;
		};


		/**
		 * Constructor for an input channel to be connected to an input file.
		 */
		LayerInputChannelType(
				LayerInputChannelName::Type input_channel_name,
				ChannelDataArity channel_data_arity) :
			d_input_channel_name(input_channel_name),
			d_channel_data_arity(channel_data_arity)
		{  }


		/**
		 * Constructor for an input channel to be connected to the output of another layer.
		 *
		 * The types of layers is specified in @a layer_input_types.
		 *
		 * Note: The layer input types don't auto connect.
		 */
		LayerInputChannelType(
				LayerInputChannelName::Type input_channel_name,
				ChannelDataArity channel_data_arity,
				const std::vector<LayerTaskType::Type> &layer_input_types) :
			d_input_channel_name(input_channel_name),
			d_channel_data_arity(channel_data_arity),
			d_input_layer_types(
					std::vector<InputLayerType>(layer_input_types.begin(), layer_input_types.end()))
		{  }

		/**
		 * Convenience constructor for an input channel to be connected to the output
		 * of *one* type of layer only.
		 *
		 * Note: The layer input type does not auto connect.
		 */
		LayerInputChannelType(
				LayerInputChannelName::Type input_channel_name,
				ChannelDataArity channel_data_arity,
				LayerTaskType::Type layer_input_type) :
			d_input_channel_name(input_channel_name),
			d_channel_data_arity(channel_data_arity),
			d_input_layer_types(std::vector<InputLayerType>(1, layer_input_type))
		{  }


		/**
		 * Constructor for an input channel to be connected to the output of another layer.
		 *
		 * The types of layers is specified in @a layer_input_types.
		 */
		LayerInputChannelType(
				LayerInputChannelName::Type input_channel_name,
				ChannelDataArity channel_data_arity,
				const std::vector<InputLayerType> &layer_input_types) :
			d_input_channel_name(input_channel_name),
			d_channel_data_arity(channel_data_arity),
			d_input_layer_types(layer_input_types)
		{  }

		/**
		 * Convenience constructor for an input channel to be connected to the output
		 * of *one* type of layer only.
		 */
		LayerInputChannelType(
				LayerInputChannelName::Type input_channel_name,
				ChannelDataArity channel_data_arity,
				const InputLayerType &layer_input_type) :
			d_input_channel_name(input_channel_name),
			d_channel_data_arity(channel_data_arity),
			d_input_layer_types(std::vector<InputLayerType>(1, layer_input_type))
		{  }


		/**
		 * Returns the name of this input channel.
		 */
		LayerInputChannelName::Type
		get_input_channel_name() const
		{
			return d_input_channel_name;
		}


		/**
		 * Returns the input channel data arity.
		 */
		ChannelDataArity
		get_channel_data_arity() const
		{
			return d_channel_data_arity;
		}


		/**
		 * Returns a list of layer input data types that this channel can connect to.
		 *
		 * This is actually a list of layer types and defines the types of layers whose
		 * output can be connected on this input channel.
		 *
		 * If boost::none is returned then only input feature collections can be connected
		 * on this input channel.
		 */
		const boost::optional< std::vector<InputLayerType> > &
		get_input_layer_types() const
		{
			return d_input_layer_types;
		}


		/**
		 * Convenience function that returns true if can connect
		 * input feature collections (files) to this input channel.
		 */
		bool
		can_connect_to_input_feature_collections() const
		{
			return !d_input_layer_types;
		}

	private:
		LayerInputChannelName::Type d_input_channel_name;
		ChannelDataArity d_channel_data_arity;

		/**
		 * If this is boost::none then it means the layer input is from
		 * a feature collection (file) and not from the output of another layer.
		 */
		boost::optional< std::vector<InputLayerType> > d_input_layer_types;
	};
}

#endif // GPLATES_APP_LOGIC_LAYERCONNECTIONTYPE_H
