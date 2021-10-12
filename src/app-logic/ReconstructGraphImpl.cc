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

#include "ReconstructGraphImpl.h"


void
GPlatesAppLogic::ReconstructGraphImpl::Data::add_output_connection(
		LayerInputConnection *layer_input_connection)
{
	d_output_connections.push_back(layer_input_connection);
}


void
GPlatesAppLogic::ReconstructGraphImpl::Data::remove_output_connection(
		LayerInputConnection *layer_input_connection)
{
	d_output_connections.remove(layer_input_connection);
}


GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::LayerInputConnection(
		Data *input_data,
		Layer *layer,
		const ReconstructGraph::layer_input_channel_name_type &layer_input_channel_name) :
	d_input_data(input_data),
	d_layer(layer),
	d_layer_input_channel_name(layer_input_channel_name)
{
	d_input_data->add_output_connection(this);

	d_layer->get_input_connections().add_input_connection(
			layer_input_channel_name, this);
}


GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::~LayerInputConnection()
{
	d_input_data->remove_output_connection(this);

	d_layer->get_input_connections().remove_input_connection(
			d_layer_input_channel_name, this);
}


void
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::add_input_connection(
		const ReconstructGraph::layer_input_channel_name_type &input_channel_name,
		LayerInputConnection *input_connection)
{
	d_connections.insert(
			input_connection_map_type::value_type(input_channel_name, input_connection));
}


void
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::remove_input_connection(
		const ReconstructGraph::layer_input_channel_name_type &input_channel_name,
		LayerInputConnection *input_connection)
{
	// Get range of connections assigned to 'input_channel_name'.
	const std::pair<input_connection_map_type::iterator, input_connection_map_type::iterator>
			input_chanel_name_range = d_connections.equal_range(input_channel_name);

	// In that range look for 'input_connection'.
	const input_connection_map_type::iterator input_connection_iter =
			std::find(input_chanel_name_range.first, input_chanel_name_range.second,
					input_connection_map_type::value_type(input_channel_name, input_connection));

	// If found then erase it.
	if (input_connection_iter != input_chanel_name_range.second)
	{
		d_connections.erase(input_connection_iter);
	}
}


GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::connection_seq_type
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::get_input_connections() const
{
	connection_seq_type connections;

	// Copy connection pointers to sequence that will be returned to the caller.
	input_connection_map_type::const_iterator connection_iter = d_connections.begin();
	const input_connection_map_type::const_iterator connection_end = d_connections.end();
	for ( ; connection_iter != connection_end; ++connection_iter)
	{
		connections.push_back(connection_iter->second);
	}

	return connections;
}


GPlatesAppLogic::ReconstructGraphImpl::Layer::Layer(
		const boost::shared_ptr<LayerTask> &layer_task)
	: d_layer_task(layer_task)
{
	// Default value for 'd_output_data' is what we want.
	// It'll get written to later when the layer task is processed.
}


void
GPlatesAppLogic::ReconstructGraphImpl::Layer::execute(
		const double &reconstruction_time)
{
	//
	// Get this layer's input data.
	//
	LayerTask::input_data_type input_data;

	// Iterate through this layer's input connections and copy the data pointed at
	// to a new channel mapping that is more convenient for the layer task to process.
	// It is more convenient in that the layer task doesn't have to traverse the various
	// graph structures and which it should not have to know about.
	const LayerInputConnections::input_connection_map_type &input_connection_map =
			d_input_data.get_input_connection_map();
	LayerInputConnections::input_connection_map_type::const_iterator input_connection_iter =
			input_connection_map.begin();
	LayerInputConnections::input_connection_map_type::const_iterator input_connection_end =
			input_connection_map.end();
	for ( ; input_connection_iter != input_connection_end; ++input_connection_iter)
	{
		const ReconstructGraph::layer_input_channel_name_type &input_channel_name =
				input_connection_iter->first;
		const LayerInputConnection *layer_input_connection = input_connection_iter->second;

		input_data.insert(std::make_pair(
				input_channel_name,
				&layer_input_connection->get_input_data()->get_data()));
	}

	//
	// Get this layer's output data.
	//
	layer_data_type &output_data = d_output_data.get_data();

	// Set the output data to the default value before asking the layer task to process.
	// This clears the data in case the task does not write to it.
	// The default value is an invalid feature collection handle weak reference.
	output_data = layer_data_type();

	d_layer_task->process(input_data, output_data, reconstruction_time);
}
