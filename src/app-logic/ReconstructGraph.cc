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

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "ReconstructGraph.h"
#include "ReconstructGraphImpl.h"


namespace GPlatesAppLogic
{
	using namespace ReconstructGraphImpl;
}


GPlatesAppLogic::ReconstructGraph::feature_collection_id_type
GPlatesAppLogic::ReconstructGraph::add_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	boost::shared_ptr<Data> feature_collection_data(
			new Data(feature_collection));

	d_feature_collections.push_back(feature_collection_data);

	return feature_collection_id_type(feature_collection_data);
}


void
GPlatesAppLogic::ReconstructGraph::remove_feature_collection(
		const feature_collection_id_type &feature_collection_id)
{
	// Throws 'bad_weak_ptr' if 'feature_collection_id_type' is invalid meaning the client
	// passed in ids that refer to layers that have already been deleted.
	boost::shared_ptr<Data> feature_collection_data(feature_collection_id);

	// Remove all connections linked to 'feature_collection_data'.
	// Each deleted connection will in turn unlink the feature collection from a layer.
	remove_data_output_connections(feature_collection_data.get());

	// Remove the feature collection after removing the connections since removing
	// the connections relies on the feature collection existing.
	d_feature_collections.remove(feature_collection_data);
}


GPlatesAppLogic::ReconstructGraph::layer_id_type
GPlatesAppLogic::ReconstructGraph::add_layer(
		const boost::shared_ptr<LayerTask> &layer_task)
{
	boost::shared_ptr<Layer> new_layer(new Layer(layer_task));

	d_layers.push_back(new_layer);

	return layer_id_type(new_layer);
}


void
GPlatesAppLogic::ReconstructGraph::remove_layer(
		const layer_id_type &layer_id)
{
	// Throws 'bad_weak_ptr' if 'layer_id_type' is invalid meaning the client
	// passed in ids that refer to layers that have already been deleted.
	boost::shared_ptr<Layer> layer(layer_id);

	// Remove all connections linked to inputs of 'layer'.
	// Each deleted connection will in turn unlink an input data from 'layer'.
	remove_layer_input_connections(layer.get());

	// Remove all connections linked to the output data of 'layer'.
	// Each deleted connection will in turn unlink the output data from another layer.
	remove_data_output_connections(layer->get_output_data());

	// Remove the layer after removing the connections since removing
	// the connections relies on the layer existing.
	d_layers.remove(layer);
}


std::vector<GPlatesAppLogic::ReconstructGraph::input_channel_definition_type>
GPlatesAppLogic::ReconstructGraph::get_layer_input_channel_definitions(
		const layer_id_type &layer_id) const
{
	// Throws 'bad_weak_ptr' if 'layer_id_type' is invalid meaning the client
	// passed in ids that refer to layers that have already been deleted.
	boost::shared_ptr<Layer> layer(layer_id);

	return layer->get_layer_task()->get_input_channel_definitions();
}


GPlatesAppLogic::ReconstructGraph::DataType
GPlatesAppLogic::ReconstructGraph::get_layer_output_definition(
		const layer_id_type &layer_id) const
{
	// Throws 'bad_weak_ptr' if 'layer_id_type' is invalid meaning the client
	// passed in ids that refer to layers that have already been deleted.
	boost::shared_ptr<Layer> layer(layer_id);

	return layer->get_layer_task()->get_output_definition();
}


GPlatesAppLogic::ReconstructGraph::layer_input_connection_id_type
GPlatesAppLogic::ReconstructGraph::connect_layer_input_to_feature_collection(
		const feature_collection_id_type &feature_collection_id,
		const layer_id_type &layer_id_inputting_data,
		const layer_input_channel_name_type &layer_input_data_channel)
{
	// Throws 'bad_weak_ptr' if 'feature_collection_id_type' or 'layer_id_type' is invalid
	// meaning the client passed in ids that refer to layers that have already been deleted.
	boost::shared_ptr<Data> feature_collection_data(feature_collection_id);
	boost::shared_ptr<Layer> layer_inputing_data(layer_id_inputting_data);

	// Connect the output data of the outputting layer to
	// the 'layer_input_data_channel' of the inputting layer.
	// The constructor of 'LayerInputConnection' does the linking.
	boost::shared_ptr<LayerInputConnection> connection(new LayerInputConnection(
			feature_collection_data.get(),
			layer_inputing_data.get(),
			layer_input_data_channel));

	d_layer_input_connections.push_back(connection);

	return layer_input_connection_id_type(connection);
}


GPlatesAppLogic::ReconstructGraph::layer_input_connection_id_type
GPlatesAppLogic::ReconstructGraph::connect_layer_input_to_layer_output(
		const layer_id_type &layer_id_outputting_data,
		const layer_id_type &layer_id_inputting_data,
		const layer_input_channel_name_type &layer_input_data_channel)
{
	// Throws 'bad_weak_ptr' if 'layer_id_type' is invalid meaning the client
	// passed in ids that refer to layers that have already been deleted.
	boost::shared_ptr<Layer> layer_outputing_data(layer_id_outputting_data);
	boost::shared_ptr<Layer> layer_inputing_data(layer_id_inputting_data);

	// Connect the output data of the outputting layer to
	// the 'layer_input_data_channel' of the inputting layer.
	// The constructor of 'LayerInputConnection' does the linking.
	boost::shared_ptr<LayerInputConnection> connection(new LayerInputConnection(
			layer_outputing_data->get_output_data(),
			layer_inputing_data.get(),
			layer_input_data_channel));

	d_layer_input_connections.push_back(connection);

	return layer_input_connection_id_type(connection);
}


void
GPlatesAppLogic::ReconstructGraph::disconnect_layer_input(
		const layer_input_connection_id_type &layer_input_connection_id)
{
	// Throws 'bad_weak_ptr' if 'layer_input_connection_id' is invalid meaning the client
	// passed in ids that refer to layer connections that have already been deleted.
	boost::shared_ptr<LayerInputConnection> layer_input_connection(layer_input_connection_id);

	// The destructor of 'LayerInputConnection' should do the unlinking.
	d_layer_input_connections.remove(layer_input_connection);
}


void
GPlatesAppLogic::ReconstructGraph::remove_layer_input_connections(
		Layer *layer)
{
	// Remove all input connections linked to 'layer'.
	// Each deleted connection will in turn unlink 'layer' from an input data source.
	const LayerInputConnections::connection_seq_type &connections =
			layer->get_input_connections().get_input_connections();
	LayerInputConnections::connection_seq_type::const_iterator layer_input_iter =
			connections.begin();
	const LayerInputConnections::connection_seq_type::const_iterator layer_input_end =
			connections.end();
	for ( ; layer_input_iter != layer_input_end; ++layer_input_iter)
	{
		LayerInputConnection *layer_input_connection = *layer_input_iter;

		remove_layer_input_connection(layer_input_connection);
	}
}


void
GPlatesAppLogic::ReconstructGraph::remove_data_output_connections(
		Data *feature_collection_data)
{
	// Remove all connections linked to 'feature_collection_data'.
	// Each deleted connection will in turn unlink the feature collection from a layer.
	Data::connection_seq_type::const_iterator data_output_iter =
			feature_collection_data->get_output_connections().begin();
	const Data::connection_seq_type::const_iterator data_output_end =
			feature_collection_data->get_output_connections().end();
	for ( ; data_output_iter != data_output_end; ++data_output_iter)
	{
		LayerInputConnection *layer_input_connection = *data_output_iter;

		remove_layer_input_connection(layer_input_connection);
	}
}


void
GPlatesAppLogic::ReconstructGraph::remove_layer_input_connection(
		LayerInputConnection *layer_input_connection)
{
	// When removing the connection we need to compare the raw connection pointer
	// with boost shared pointers stored in the list of owned connections.
	// Removing should release the shared pointer (which should have reference count
	// of one) which should call destructor of 'LayerInputConnection' which will
	// unlink the feature collection from a layer.
	d_layer_input_connections.remove_if(
			layer_input_connection ==
					boost::lambda::bind(
							&boost::shared_ptr<LayerInputConnection>::get,
							boost::lambda::_1));
}
