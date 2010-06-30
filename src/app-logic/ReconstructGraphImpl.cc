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

#include <boost/foreach.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/variant.hpp>

#include "ReconstructGraphImpl.h"

#include "LayerTask.h"
#include "ReconstructionGeometryCollection.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ReconstructGraphImpl::Data::Data(
		const FeatureCollectionFileState::file_reference &file) :
	d_data(layer_task_data_type(file.get_file().get_feature_collection())),
	d_state(file),
	d_active(true)
{
}


GPlatesAppLogic::ReconstructGraphImpl::Data::Data(
		const boost::optional<layer_task_data_type> &data) :
	d_data(data),
	d_state(boost::weak_ptr<Layer>()),
	d_active(true)
{
}


void
GPlatesAppLogic::ReconstructGraphImpl::Data::activate_input_file(
		bool activate)
{
	// Ensure the constructor accepting a file was used to create 'this'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			boost::get<FeatureCollectionFileState::file_reference>(&d_state),
			GPLATES_ASSERTION_SOURCE);

	d_active = activate;
}


boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesAppLogic::ReconstructGraphImpl::Data::get_input_file() const
{
	// Attempt to cast to a file reference.
	// This will only succeed if the constructor accepting a file was used to create 'this'.
	const FeatureCollectionFileState::file_reference *input_file =
			boost::get<FeatureCollectionFileState::file_reference>(&d_state);

	// If we have no input file then return false.
	if (input_file == NULL)
	{
		return boost::none;
	}

	return *input_file;
}


boost::optional< boost::weak_ptr<GPlatesAppLogic::ReconstructGraphImpl::Layer> >
GPlatesAppLogic::ReconstructGraphImpl::Data::get_outputting_layer() const
{
	// Attempt to cast to a layer.
	// This will only succeed if the constructor accepting a file was not used to create 'this'.
	const boost::weak_ptr<Layer> *outputting_layer =
			boost::get< boost::weak_ptr<Layer> >(&d_state);

	// If we have no valid outputting layer then return false.
	if (outputting_layer == NULL || outputting_layer->expired())
	{
		return boost::none;
	}

	return *outputting_layer;
}


void
GPlatesAppLogic::ReconstructGraphImpl::Data::set_outputting_layer(
		const boost::weak_ptr<Layer> &outputting_layer)
{
	// Before we set the outputting layer we should ensure the constructor accepting
	// a file was not used to create 'this'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			boost::get< boost::weak_ptr<Layer> >(&d_state),
			GPLATES_ASSERTION_SOURCE);

	// Must also be a valid layer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!outputting_layer.expired(),
			GPLATES_ASSERTION_SOURCE);

	d_state = outputting_layer;
}


boost::optional<GPlatesAppLogic::layer_task_data_type>
GPlatesAppLogic::ReconstructGraphImpl::Data::get_data() const
{
	if (!d_active)
	{
		return boost::none;
	}

	return d_data;
}


void
GPlatesAppLogic::ReconstructGraphImpl::Data::set_data(
		const boost::optional<layer_task_data_type> &data)
{
	// Make sure we are the output of a layer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			get_outputting_layer(),
			GPLATES_ASSERTION_SOURCE);

	d_data = data;
}


void
GPlatesAppLogic::ReconstructGraphImpl::Data::disconnect_output_connections()
{
	// Get all the input connections that are connected to 'this' to disconnect.
	// This will effectively destroy those connections which will in turn
	// get us to remove them from our output list.
	// So we'll make a copy of our output connections to avoid having
	// items removed from it while we are traversing it.
	connection_seq_type output_connections(d_output_connections);
	BOOST_FOREACH(LayerInputConnection *input_connection, output_connections)
	{
		input_connection->disconnect_from_parent_layer();
	}

	// Our 'd_output_connections' sequence should now be empty.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		d_output_connections.empty(),
		GPLATES_ASSERTION_SOURCE);
}


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
		const boost::shared_ptr<Data> &input_data,
		const boost::weak_ptr<Layer> &layer_receiving_input,
		const QString &layer_input_channel_name) :
	d_input_data(input_data),
	d_layer_receiving_input(layer_receiving_input),
	d_layer_input_channel_name(layer_input_channel_name)
{
	d_input_data->add_output_connection(this);
}


GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::~LayerInputConnection()
{
	// Get the input data to disconnect from us.
	// If we are the only owning reference of the input data then when this destructor
	// has finished the input data will also get destroyed.
	d_input_data->remove_output_connection(this);

	// No need to disconnect from parent layer since parent owns us and parent is either
	// destroying all its input connections (because the parent is being destroyed) or
	// we have been explicitly disconnected from parent using 'disconnect_from_parent_layer()'
	// (which disconnects us from the parent which in turn causes us to be destroyed as our
	// parent has the only owning reference to us).
}


void
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::disconnect_from_parent_layer()
{
	// Calling this will effectively destroy 'this' since our parent layer has the only
	// owning reference to 'this'.
	boost::shared_ptr<Layer>(d_layer_receiving_input)
			->get_input_connections().remove_input_connection(d_layer_input_channel_name, this);
}


void
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::add_input_connection(
		const QString &input_channel_name,
		const boost::shared_ptr<LayerInputConnection> &input_connection)
{
	d_connections.insert(
			input_connection_map_type::value_type(input_channel_name, input_connection));
}


void
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::remove_input_connection(
		const QString &input_channel_name,
		LayerInputConnection *input_connection)
{
	// Get range of connections assigned to 'input_channel_name'.
	const std::pair<input_connection_map_type::iterator, input_connection_map_type::iterator>
			input_chanel_name_range = d_connections.equal_range(input_channel_name);

	// In that range look for 'input_connection'.
	input_connection_map_type::iterator connections_iter = input_chanel_name_range.first;
	input_connection_map_type::iterator connections_end = input_chanel_name_range.second;
	for ( ; connections_iter != connections_end; ++connections_iter)
	{
		if (connections_iter->second.get() == input_connection)
		{
			// If found then erase it.
			d_connections.erase(connections_iter);
			return;
		}
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


GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::connection_seq_type
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnections::get_input_connections(
		const QString &input_channel_name) const
{
	connection_seq_type connections;

	// Get range of connections assigned to 'input_channel_name'.
	const std::pair<input_connection_map_type::const_iterator, input_connection_map_type::const_iterator>
			input_chanel_name_range = d_connections.equal_range(input_channel_name);

	// Copy connection pointers to sequence that will be returned to the caller.
	input_connection_map_type::const_iterator connection_iter = input_chanel_name_range.first;
	const input_connection_map_type::const_iterator connection_end = input_chanel_name_range.second;
	for ( ; connection_iter != connection_end; ++connection_iter)
	{
		connections.push_back(connection_iter->second);
	}

	return connections;
}


GPlatesAppLogic::ReconstructGraphImpl::Layer::Layer(
		const boost::shared_ptr<LayerTask> &layer_task,
		ReconstructGraph &reconstruct_graph) :
	d_reconstruct_graph(&reconstruct_graph),
	d_layer_task(layer_task),
	d_output_data(new Data()),
	d_active(true)
{
	// Default value for 'd_output_data' is what we want.
	// It'll get written to later when the layer task is processed.
}


GPlatesAppLogic::ReconstructGraphImpl::Layer::~Layer()
{
	// Get our output data to disconnect from its output connections.
	d_output_data->disconnect_output_connections();
}


// gcc isn't liking the BOOST_MPL_ASSERT.
#if defined(__GNUC__)
#	pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
void
GPlatesAppLogic::ReconstructGraphImpl::Layer::execute(
		Reconstruction &reconstruction,
		GPlatesModel::integer_plate_id_type anchored_plate_id)
{
	// If this layer is not active then set the output data as empty.
	if (!d_active)
	{
		d_output_data->set_data(boost::none);
		return;
	}

	//
	// Get this layer's input data.
	//
	LayerTask::input_data_type input_data_collection;

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
		const QString &input_channel_name = input_connection_iter->first;
		const boost::shared_ptr<LayerInputConnection> &layer_input_connection =
				input_connection_iter->second;

		// If the input connection has data then add it to out input data collection.
		const boost::optional<layer_task_data_type> &input_data =
				layer_input_connection->get_input_data()->get_data();
		if (input_data)
		{
			input_data_collection.insert(std::make_pair(input_channel_name, &input_data.get()));
		}
	}

	// Process the layer's task.
	boost::optional<layer_task_data_type> layer_task_output =
			d_layer_task->process(
					input_data_collection,
					reconstruction.get_reconstruction_time(),
					anchored_plate_id,
					reconstruction.get_default_reconstruction_tree());

	// Store the layer task output in the layer's output data.
	d_output_data->set_data(layer_task_output);

	// See if this layer's output data type is a reconstruction geometry collection.
	// If so then add it to the reconstruction.
	if (layer_task_output)
	{
		typedef ReconstructionGeometryCollection::non_null_ptr_to_const_type
				reconstruction_geometry_collection_type;

		// Generate compile time error if the bounded type in the 'layer_task_data_type' variant
		// that corresponds to reconstruction geometry collections is changed.
		BOOST_MPL_ASSERT((boost::mpl::contains<
				layer_task_data_types,
				reconstruction_geometry_collection_type>));

		const reconstruction_geometry_collection_type *reconstruction_geometry_collection =
				boost::get<reconstruction_geometry_collection_type>(&layer_task_output.get());
		if (reconstruction_geometry_collection)
		{
			reconstruction.add_reconstruction_geometries(*reconstruction_geometry_collection);
		}
	}
}
#if defined(__GNUC__)
#	pragma GCC diagnostic error "-Wold-style-cast"
#endif


void
GPlatesAppLogic::ReconstructGraphImpl::Layer::activate(
		bool active)
{
	d_active = active;
}


bool
GPlatesAppLogic::ReconstructGraphImpl::detect_cycle_in_graph(
		const Layer *originating_layer,
		const Layer *input_layer)
{
	const LayerInputConnections::connection_seq_type input_layer_connections =
			input_layer->get_input_connections().get_input_connections();

	// Iterate over the input connections of 'input_layer'.
	BOOST_FOREACH(
			const boost::shared_ptr<LayerInputConnection> &input_layer_connection,
			input_layer_connections)
	{
		// See if the current input connection connects to the output of another layer.
		const boost::optional< boost::weak_ptr<Layer> > input_layer_child_weak_ref =
				input_layer_connection->get_input_data()->get_outputting_layer();
		if (input_layer_child_weak_ref)
		{
			const boost::shared_ptr<Layer> input_layer_child(input_layer_child_weak_ref.get());

			// See if the child layer is the originating layer.
			// If so then we've detected a cycle.
			if (input_layer_child.get() == originating_layer)
			{
				// A cycle was detected.
				return true;
			}

			// Recursively propagate along the dependency graph until it ends or forms a cycle.
			if (detect_cycle_in_graph(originating_layer, input_layer_child.get()))
			{
				// We detected a cycle so return immediately.
				return true;
			}
		}
	}

	return false;
}
