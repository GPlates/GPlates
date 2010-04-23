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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTGRAPH_H
#define GPLATES_APP_LOGIC_RECONSTRUCTGRAPH_H

#include <string>
#include <list>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	class LayerTask;

	namespace ReconstructGraphImpl
	{
		class Layer;
		class LayerInputConnection;
		class Data;
	}


	class ReconstructGraph
	{
	public:
		/**
		 * The various types of layers distinguished by the type of processing
		 * they perform on their inputs.
		 */
		enum LayerType
		{
			/*
			 * Creates @a ReconstructedFeatureGeometry objects from features
			 * containing geometry properties and a reconstruction plate id.
			 */
			RECONSTRUCT_LAYER,
			/*
			 * Creates a @a ReconstructionTree from total reconstruction sequence features.
			 */
			ROTATE_LAYER,
			/*
			 * Creates @a ResolvedTopologicalBoundary objects from topological
			 * closed plate boundary features.
			 */
			TOPOLOGICAL_PLATE_POLYGON_LAYER,

			NUM_LAYER_TYPES // Must be last
		};

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
		 */
		enum DataType
		{
			FEATURE_COLLECTION_DATA,
			RECONSTRUCTED_GEOMETRY_COLLECTION_DATA,
			RECONSTRUCTION_TREE_DATA
		};

		/**
		 * Represents the number of data inputs allowed by a specific input channel of a layer.
		 *
		 * A layer can have one of more input channels representing different classifications
		 * of input data and each channel can have one or more data objects.
		 * The latter is what's determined here.
		 *
		 * For example the @a RECONSTRUCT_LAYER layer has a "rotation tree" input channel and a
		 * "reconstructable features" input channel.
		 * In the "reconstructable features" there can be multiple feature collections but
		 * in the rotation tree channel there can only be one reconstruction tree.
		 */
		enum ChannelDataArity
		{
			ONE_DATA_IN_CHANNEL,
			MULTIPLE_DATAS_IN_CHANNEL
		};

		/**
		 * Handle id to a feature collection added to the graph.
		 */
		typedef boost::weak_ptr<ReconstructGraphImpl::Data> feature_collection_id_type;

		/**
		 * Handle id to a layer added to the graph.
		 */
		typedef boost::weak_ptr<ReconstructGraphImpl::Layer> layer_id_type;

		/**
		 * Handle id to a connection between a data source and layer.
		 */
		typedef boost::weak_ptr<ReconstructGraphImpl::LayerInputConnection> layer_input_connection_id_type;

		/**
		 * Channel name identifies a specific input channel of a layer.
		 */
		typedef std::string layer_input_channel_name_type;

		/**
		 * Typedef for information describing the data type and arity allowed for
		 * a single input channel.
		 */
		typedef boost::tuple<
				ReconstructGraph::layer_input_channel_name_type,
				ReconstructGraph::DataType,
				ReconstructGraph::ChannelDataArity> input_channel_definition_type;


		feature_collection_id_type
		add_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		void
		remove_feature_collection(
				const feature_collection_id_type &feature_collection_id);


		/**
		 * Adds a new layer to the graph.
		 *
		 * The layer will need to be connected to a feature collection or another layer.
		 *
		 * You can create @a layer_task using @a LayerTaskRegistry.
		 */
		layer_id_type
		add_layer(
				const boost::shared_ptr<LayerTask> &layer_task);


		void
		remove_layer(
				const layer_id_type &layer_id);


		std::vector<input_channel_definition_type>
		get_layer_input_channel_definitions(
				const layer_id_type &layer_id) const;


		DataType
		get_layer_output_definition(
				const layer_id_type &layer_id) const;


		layer_input_connection_id_type
		connect_layer_input_to_feature_collection(
				const feature_collection_id_type &feature_collection_id,
				const layer_id_type &layer_id_inputting_data,
				const layer_input_channel_name_type &layer_input_data_channel);


		layer_input_connection_id_type
		connect_layer_input_to_layer_output(
				const layer_id_type &layer_id_outputting_data,
				const layer_id_type &layer_id_inputting_data,
				const layer_input_channel_name_type &layer_input_data_channel);


		/**
		 * Disconnects an input data source from a layer.
		 *
		 * There are two situations where this can occur:
		 * 1) A feature collection that is used as input to a layer,
		 * 2) A layer output that is used as input to another layer.
		 */
		void
		disconnect_layer_input(
				const layer_input_connection_id_type &layer_input_connection_id);

	private:
		typedef std::list< boost::shared_ptr<ReconstructGraphImpl::Layer> >
				layer_ptr_seq_type;

		typedef std::list< boost::shared_ptr<ReconstructGraphImpl::Data> >
				feature_collection_ptr_seq_type;

		typedef std::list< boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> >
				layer_input_connection_ptr_seq_type;


		feature_collection_ptr_seq_type d_feature_collections;
		layer_ptr_seq_type d_layers;
		layer_input_connection_ptr_seq_type d_layer_input_connections;


		/**
		 * Remove all input connections linked to @a layer.
		 *
		 * Each deleted connection will in turn unlink @a layer from an input data source.
		 */
		void
		remove_layer_input_connections(
				ReconstructGraphImpl::Layer *layer);

		void
		remove_layer_input_connection(
				ReconstructGraphImpl::LayerInputConnection *layer_input_connection);

		/**
		 * Remove all connections linked to @a feature_collection_data.
		 *
		 * Each deleted connection will in turn unlink the feature collection from a layer.
		 */
		void
		remove_data_output_connections(
				ReconstructGraphImpl::Data *feature_collection_data);
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTGRAPH_H
