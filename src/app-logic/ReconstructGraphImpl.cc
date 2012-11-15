/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/variant.hpp>

#include "ReconstructGraphImpl.h"

#include "LayerTask.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


#include <boost/foreach.hpp>
GPlatesAppLogic::ReconstructGraphImpl::Data::Data(
		const FeatureCollectionFileState::file_reference &file) :
	d_data(file)
{
}


GPlatesAppLogic::ReconstructGraphImpl::Data::Data(
		const LayerProxy::non_null_ptr_type &layer_proxy) :
	d_data(layer_proxy)
{
}


boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesAppLogic::ReconstructGraphImpl::Data::get_input_file() const
{
	// Attempt to cast to a file reference.
	// This will only succeed if the constructor accepting a file was used to create 'this'.
	const FeatureCollectionFileState::file_reference *input_file =
			boost::get<FeatureCollectionFileState::file_reference>(&d_data);

	// If we have no input file then return false.
	if (input_file == NULL)
	{
		return boost::none;
	}

	return *input_file;
}


boost::optional<GPlatesAppLogic::LayerProxy::non_null_ptr_type>
GPlatesAppLogic::ReconstructGraphImpl::Data::get_layer_proxy() const
{
	// Attempt to cast to a layer proxy.
	// This will only succeed if the constructor accepting a layer proxy was used to create 'this'.
	const LayerProxy::non_null_ptr_type *layer_proxy =
			boost::get<LayerProxy::non_null_ptr_type>(&d_data);

	// If we have no layer proxy then return false.
	if (layer_proxy == NULL)
	{
		return boost::none;
	}

	return *layer_proxy;
}


boost::optional< boost::weak_ptr<GPlatesAppLogic::ReconstructGraphImpl::Layer> >
GPlatesAppLogic::ReconstructGraphImpl::Data::get_outputting_layer() const
{
	// If we have no valid outputting layer then return false.
	if (!d_outputting_layer || d_outputting_layer->expired())
	{
		return boost::none;
	}

	return *d_outputting_layer;
}


void
GPlatesAppLogic::ReconstructGraphImpl::Data::set_outputting_layer(
		const boost::weak_ptr<Layer> &outputting_layer)
{
	// Before we set the outputting layer we should ensure that
	// the constructor accepting a file was *not* used to create 'this'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			boost::get<LayerProxy::non_null_ptr_type>(&d_data),
			GPLATES_ASSERTION_SOURCE);

	// Must also be a valid layer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!outputting_layer.expired(),
			GPLATES_ASSERTION_SOURCE);

	d_outputting_layer = outputting_layer;
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
		const QString &layer_input_channel_name,
		bool is_input_layer_active) :
	d_input_data(input_data),
	d_layer_receiving_input(layer_receiving_input),
	d_layer_input_channel_name(layer_input_channel_name),
	d_is_input_layer_active(is_input_layer_active),
	d_can_access_layer_receiving_input(true)
{
	// Input data should be non-NULL.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		d_input_data,
		GPLATES_ASSERTION_SOURCE);

	d_input_data->add_output_connection(this);

	//
	// Notify the layer task of the layer receiving input from this connection
	// that a new connection is being made.
	//

	const boost::shared_ptr<Layer> layer_receiving_input_shared_ptr(d_layer_receiving_input);
	if (!layer_receiving_input_shared_ptr)
	{
		return;
	}
	LayerTask &layer_task = layer_receiving_input_shared_ptr->get_layer_task();

	// First see if the input data refers to an input file.
	const boost::optional<FeatureCollectionFileState::file_reference> input_file =
			d_input_data->get_input_file();
	if (input_file)
	{
		layer_task.add_input_file_connection(
				d_layer_input_channel_name,
				input_file->get_file().get_feature_collection());

		// Register a model callback so we know when the input file has been modified.
		d_callback_input_feature_collection = input_file->get_file().get_feature_collection();
		d_callback_input_feature_collection.attach_callback(new FeatureCollectionModified(this));
	}
	else
	{
		// It's not an input file so it should be a layer proxy (the output of another layer).
		const boost::optional<LayerProxy::non_null_ptr_type> input_layer_proxy =
				d_input_data->get_layer_proxy();
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				input_layer_proxy,
				GPLATES_ASSERTION_SOURCE);

		// If the input layer is active then tell the output layer task to connect to the input layer.
		if (d_is_input_layer_active)
		{
			layer_task.add_input_layer_proxy_connection(
					d_layer_input_channel_name,
					input_layer_proxy.get());
		}
	}
}


GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::~LayerInputConnection()
{
	//
	// Notify the layer task of the layer receiving input from this connection
	// that the connection is being removed.
	//

	if (d_can_access_layer_receiving_input)
	{
		const boost::shared_ptr<Layer> layer_receiving_input_shared_ptr(d_layer_receiving_input);
		if (layer_receiving_input_shared_ptr)
		{
			LayerTask &layer_task = layer_receiving_input_shared_ptr->get_layer_task();

			// First see if the input data refers to an input file.
			const boost::optional<FeatureCollectionFileState::file_reference> input_file =
					d_input_data->get_input_file();
			if (input_file)
			{
				layer_task.remove_input_file_connection(
						d_layer_input_channel_name,
						input_file->get_file().get_feature_collection());
			}
			else
			{
				// It's not an input file so it should be a layer proxy (the output of another layer).
				const boost::optional<LayerProxy::non_null_ptr_type> input_layer_proxy =
						d_input_data->get_layer_proxy();
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						input_layer_proxy,
						GPLATES_ASSERTION_SOURCE);

				// If the input layer is active then tell the output layer task to disconnect from the input layer.
				if (d_is_input_layer_active)
				{
					layer_task.remove_input_layer_proxy_connection(
							d_layer_input_channel_name,
							input_layer_proxy.get());

					d_is_input_layer_active = false;
				}
			}
		}
	}

	//
	// Now continue on with destruction.
	//

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
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::input_layer_activated(
		bool active)
{
	// Since we're tracking the activation state of the input layer we should
	// not be getting out of sync with it.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			active != d_is_input_layer_active,
			GPLATES_ASSERTION_SOURCE);

	d_is_input_layer_active = active;

	if (d_can_access_layer_receiving_input)
	{
		const boost::shared_ptr<Layer> layer_receiving_input_shared_ptr(d_layer_receiving_input);
		if (layer_receiving_input_shared_ptr)
		{
			LayerTask &layer_task = layer_receiving_input_shared_ptr->get_layer_task();

			// The input data should refer to a layer proxy (the output of another layer).
			const boost::optional<LayerProxy::non_null_ptr_type> input_layer_proxy =
					d_input_data->get_layer_proxy();
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					input_layer_proxy,
					GPLATES_ASSERTION_SOURCE);

			// Tell the layer proxy to add or remove the input layer proxy.
			// NOTE: The layer connection is still in place but the layer task thinks
			// that the connection has been made or lost.
			if (d_is_input_layer_active)
			{
				// The input layer has just been activated.
				layer_task.add_input_layer_proxy_connection(
						d_layer_input_channel_name,
						input_layer_proxy.get());
			}
			else
			{
				// The input layer has just been deactivated.
				layer_task.remove_input_layer_proxy_connection(
						d_layer_input_channel_name,
						input_layer_proxy.get());
			}
		}
	}
}


void
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::layer_receiving_input_is_being_destroyed()
{
	// We can no longer access the layer receiving with our weak_ptr because it is being
	// destroyed and hence a shared_ptr to it could be in a partially invalid state.
	d_can_access_layer_receiving_input = false;
}


void
GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::modified_input_feature_collection()
{
	//
	// Notify the layer task of the layer receiving input from this connection
	// that the input file (feature collection) has been modified.
	//

	if (d_can_access_layer_receiving_input)
	{
		const boost::shared_ptr<Layer> layer_receiving_input_shared_ptr(d_layer_receiving_input);
		if (layer_receiving_input_shared_ptr)
		{
			LayerTask &layer_task = layer_receiving_input_shared_ptr->get_layer_task();

			// Make sure this data refers to an input file.
			const boost::optional<FeatureCollectionFileState::file_reference> input_file =
					d_input_data->get_input_file();
			if (input_file)
			{
				layer_task.modified_input_file(
						d_layer_input_channel_name,
						input_file->get_file().get_feature_collection());
			}
		}
	}
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
		ReconstructGraph &reconstruct_graph,
		bool auto_created) :
	d_reconstruct_graph(&reconstruct_graph),
	d_layer_task(layer_task),
	d_output_data(new Data(layer_task->get_layer_proxy())),
	d_active(true),
	d_auto_created(auto_created)
{
	// Layer task should be non-NULL.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		d_layer_task,
		GPLATES_ASSERTION_SOURCE);
}


GPlatesAppLogic::ReconstructGraphImpl::Layer::~Layer()
{
	// Get our output data to disconnect from its output connections.
	d_output_data->disconnect_output_connections();

	// Iterate over the input connections and tell them not to reference us via their weak_ptr
	// because we are being destroyed and the shared_ptr to us could be in a partially invalid state.
	const LayerInputConnections::connection_seq_type input_layer_connections =
			d_input_data.get_input_connections();

	// Iterate over the input connections of 'input_layer'.
	BOOST_FOREACH(
			const boost::shared_ptr<LayerInputConnection> &input_layer_connection,
			input_layer_connections)
	{
		input_layer_connection->layer_receiving_input_is_being_destroyed();
	}
}


void
GPlatesAppLogic::ReconstructGraphImpl::Layer::activate(
		bool active)
{
	// If the activation state isn't changing then do nothing.
	if (active == d_active)
	{
		return;
	}

	d_active = active;

	// Let any layer connections, connected to our output data, know that we are
	// now active/inactive. This message will get delivered to the layer tasks of those
	// layer connections so that they know whether to access our output data or not.
	// If we're inactive then they should not access our output data.
	const Data::connection_seq_type &output_connections = d_output_data->get_output_connections();
	BOOST_FOREACH(LayerInputConnection *output_connection, output_connections)
	{
		output_connection->input_layer_activated(active);
	}

	// Notify the layer task of the change in active state.
	d_layer_task->activate(active);
}


GPlatesAppLogic::LayerTaskParams &
GPlatesAppLogic::ReconstructGraphImpl::Layer::get_layer_task_params()
{
	return d_layer_task->get_layer_task_params();
}


bool
GPlatesAppLogic::ReconstructGraphImpl::detect_cycle_in_graph(
		const Layer *originating_layer,
		const Layer *input_layer)
{
	// UPDATE: From a purely graph point-of-view cycles are actually allowed.
	// For example, a raster can use an age-grid during reconstruction but also the age-grid
	// can use the raster as a normal map for its surface lighting. This is a cycle but it's
	// OK because there's a disconnect between a layer's input and output. In this example
	// there's a disconnect in the raster layer between the age-grid input and the normal map
	// output - they are unrelated and don't depend on each other. So in this example while there
	// is a cycle in the connection graph there is no actual cycle in the dependencies.
	// TODO: For now we'll just disable cycle checking - if it's reintroduced it'll need to
	// be smarter and get help from the layer proxies to determine dependency cycles.
#if 0
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
#endif

	return false;
}
