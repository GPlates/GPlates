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

#include "Layer.h"

#include "LayerTask.h"
#include "ReconstructGraph.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

GPlatesAppLogic::Layer::Layer(
		const boost::weak_ptr<ReconstructGraphImpl::Layer> &layer_impl) :
	d_impl(layer_impl)
{
}


bool
GPlatesAppLogic::Layer::is_active() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	return boost::shared_ptr<ReconstructGraphImpl::Layer>(d_impl)->is_active();
}


void
GPlatesAppLogic::Layer::activate(
		bool active)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	const boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	const bool previously_active = layer_impl->is_active();

	if (active != previously_active)
	{
		// We only activate/deactivate if the activation state changes.
		layer_impl->activate(active);

		// Get the ReconstructGraph to emit a signal if the active state changed.
		layer_impl->get_reconstruct_graph().emit_layer_activation_changed(*this, active);
	}
}


GPlatesAppLogic::LayerTaskType::Type
GPlatesAppLogic::Layer::get_type() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	return boost::shared_ptr<ReconstructGraphImpl::Layer>(d_impl)
			->get_layer_task().get_layer_type();
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::Layer::get_input_channel_types() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	return boost::shared_ptr<ReconstructGraphImpl::Layer>(d_impl)
			->get_layer_task().get_input_channel_types();
}


QString
GPlatesAppLogic::Layer::get_main_input_feature_collection_channel() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	return boost::shared_ptr<ReconstructGraphImpl::Layer>(d_impl)
			->get_layer_task().get_main_input_feature_collection_channel();
}


void
GPlatesAppLogic::Layer::set_layer_task(
		const boost::shared_ptr<LayerTask> &layer_task)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer>(d_impl)
			->set_layer_task(layer_task);
}


GPlatesAppLogic::Layer::InputConnection
GPlatesAppLogic::Layer::connect_input_to_file(
		const InputFile &input_file,
		const QString &input_data_channel)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);
	boost::shared_ptr<ReconstructGraphImpl::Data> input_file_impl(input_file.get_impl());

	// Connect the feature collection to the 'input_data_channel' of this layer.
	boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> input_connection_impl(
			new ReconstructGraphImpl::LayerInputConnection(
					input_file_impl, get_impl(), input_data_channel));

	layer_impl->get_input_connections().add_input_connection(
			input_data_channel, input_connection_impl);

	// Wrap the input connection in a weak reference for the caller.
	InputConnection input_connection(input_connection_impl);

	// Get the ReconstructGraph to emit a signal.
	layer_impl->get_reconstruct_graph().emit_layer_added_input_connection(
			*this, input_connection);

	return input_connection;
}


GPlatesAppLogic::Layer::InputConnection
GPlatesAppLogic::Layer::connect_input_to_layer_output(
		const Layer &layer_outputting_data,
		const QString &input_data_channel)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		layer_outputting_data.is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);
	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_outputting_data_impl(
			layer_outputting_data.get_impl());

	// See if we can make the new connection without introducing a cycle in the
	// dependency graph.
	if (ReconstructGraphImpl::detect_cycle_in_graph(
		layer_impl.get(), layer_outputting_data_impl.get()))
	{
		throw CycleDetectedInReconstructGraph(GPLATES_EXCEPTION_SOURCE);
	}

	boost::shared_ptr<ReconstructGraphImpl::Data> input =
			layer_outputting_data_impl->get_output_data();

	// Connect the feature collection to the 'input_data_channel' of this layer.
	boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> input_connection_impl(
			new ReconstructGraphImpl::LayerInputConnection(
					input, get_impl(), input_data_channel, layer_outputting_data_impl->is_active()));

	layer_impl->get_input_connections().add_input_connection(
			input_data_channel, input_connection_impl);

	// Wrap the input connection in a weak reference for the caller.
	InputConnection input_connection(input_connection_impl);

	// Get the ReconstructGraph to emit a signal.
	layer_impl->get_reconstruct_graph().emit_layer_added_input_connection(
			*this, input_connection);

	return input_connection;
}


void
GPlatesAppLogic::Layer::disconnect_input_from_file(
		const InputFile &input_file,
		const QString &input_data_channel)
{
	// Get the channel input connections.
	std::vector<InputConnection> channel_inputs = get_channel_inputs(input_data_channel);

	// Find the input connection with the given input file.
	BOOST_FOREACH(InputConnection &input_connection, channel_inputs)
	{
		boost::optional<InputFile> current_input_file = input_connection.get_input_file();
		if (current_input_file && *current_input_file == input_file)
		{
			input_connection.disconnect();
			break;
		}
	}
}


void
GPlatesAppLogic::Layer::disconnect_input_from_layer_output(
		const Layer &layer_outputting_data,
		const QString &input_data_channel)
{
	// Get the channel input connections.
	std::vector<InputConnection> channel_inputs = get_channel_inputs(input_data_channel);

	// Find the input connection with the given layer.
	BOOST_FOREACH(InputConnection &input_connection, channel_inputs)
	{
		boost::optional<Layer> current_input_layer = input_connection.get_input_layer();
		if (current_input_layer && *current_input_layer == layer_outputting_data)
		{
			input_connection.disconnect();
			break;
		}
	}
}


void
GPlatesAppLogic::Layer::disconnect_channel_inputs(
		const QString &input_data_channel)
{
	// Get the channel input connections.
	std::vector<InputConnection> channel_inputs = get_channel_inputs(input_data_channel);

	// Disconnect all input connections.
	BOOST_FOREACH(InputConnection &input_connection, channel_inputs)
	{
		input_connection.disconnect();
	}
}


std::vector<GPlatesAppLogic::Layer::InputConnection>
GPlatesAppLogic::Layer::get_channel_inputs(
		const QString &input_data_channel) const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	// Get the input connections for the input data channel.
	const ReconstructGraphImpl::LayerInputConnections::connection_seq_type input_connection_impls =
			layer_impl->get_input_connections().get_input_connections(input_data_channel);

	// Return the input connections as weak references.
	std::vector<Layer::InputConnection> input_connections;
	input_connections.reserve(input_connection_impls.size());
	BOOST_FOREACH(
			const boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> &input_connection_impl,
			input_connection_impls)
	{
		input_connections.push_back(Layer::InputConnection(input_connection_impl));
	}

	return input_connections;
}


std::vector<GPlatesAppLogic::Layer::InputConnection>
GPlatesAppLogic::Layer::get_all_inputs() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	// Get all input connections.
	const ReconstructGraphImpl::LayerInputConnections::connection_seq_type input_connection_impls =
			layer_impl->get_input_connections().get_input_connections();

	// Return the input connections as weak references.
	std::vector<Layer::InputConnection> input_connections;
	input_connections.reserve(input_connection_impls.size());
	BOOST_FOREACH(
			const boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> &input_connection_impl,
			input_connection_impls)
	{
		input_connections.push_back(Layer::InputConnection(input_connection_impl));
	}

	return input_connections;
}


GPlatesAppLogic::LayerTaskParams &
GPlatesAppLogic::Layer::get_layer_task_params()
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	return layer_impl->get_layer_task_params();
}


boost::optional<GPlatesAppLogic::LayerProxy::non_null_ptr_type>
GPlatesAppLogic::Layer::get_layer_output() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_valid(),
			GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	// If the current layer is not active then don't return the layer proxy.
	// Otherwise the caller can ask the layer proxy to do some processing effectively
	// making the layer active.
	if (!layer_impl->is_active())
	{
		return boost::none;
	}

	// Get the layer proxy.
	const boost::optional<LayerProxy::non_null_ptr_type> layer_proxy =
			layer_impl->get_output_data()->get_layer_proxy();

	// The output of a layer should always be a layer proxy.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_proxy,
			GPLATES_ASSERTION_SOURCE);

	// Return the layer proxy converted to a pointer-to-const.
	return layer_proxy.get();
}


GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type
GPlatesAppLogic::Layer::get_layer_proxy_handle() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_valid(),
			GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	// Get the layer proxy.
	const boost::optional<LayerProxy::non_null_ptr_type> layer_proxy =
			layer_impl->get_output_data()->get_layer_proxy();

	// The output of a layer should always be a layer proxy.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_proxy,
			GPLATES_ASSERTION_SOURCE);

	// Return the layer proxy converted to a pointer-to-const.
	return layer_proxy.get();
}


bool
GPlatesAppLogic::Layer::get_auto_created() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_valid(),
			GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	return layer_impl->get_auto_created();
}


void
GPlatesAppLogic::Layer::set_auto_created(
		bool auto_created)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_valid(),
			GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

	layer_impl->set_auto_created(auto_created);
}


GPlatesAppLogic::FeatureCollectionFileState::file_reference
GPlatesAppLogic::Layer::InputFile::get_file() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::Data> input_data_impl(d_impl);

	const boost::optional<FeatureCollectionFileState::file_reference> input_file =
			input_data_impl->get_input_file();

	// The data object should be an input file.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		input_file,
		GPLATES_ASSERTION_SOURCE);

	return input_file.get();
}


const QString &
GPlatesAppLogic::Layer::InputConnection::get_input_channel_name() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	return boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection>(d_impl)
			->get_input_channel_name();
}


GPlatesAppLogic::Layer
GPlatesAppLogic::Layer::InputConnection::get_layer() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> input_connection_impl(d_impl);

	return Layer(input_connection_impl->get_layer_receiving_input());
}


boost::optional<GPlatesAppLogic::Layer::InputFile>
GPlatesAppLogic::Layer::InputConnection::get_input_file() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> input_connection_impl(d_impl);

	const boost::shared_ptr<ReconstructGraphImpl::Data> data_connected_to_input =
			input_connection_impl->get_input_data();

	// If the input connection is not connected to an input file...
	if (!data_connected_to_input->get_input_file())
	{
		return boost::none;
	}

	// Return a weak reference to the input file.
	return InputFile(data_connected_to_input);
}


boost::optional<GPlatesAppLogic::Layer>
GPlatesAppLogic::Layer::InputConnection::get_input_layer() const
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> input_connection_impl(d_impl);

	const boost::optional< boost::weak_ptr<ReconstructGraphImpl::Layer> > layer_connected_to_input =
			input_connection_impl->get_input_data()->get_outputting_layer();

	// If the input connection is not connected to the output of another layer...
	if (!layer_connected_to_input)
	{
		return boost::none;
	}

	// Return a weak reference to the layer.
	return Layer(layer_connected_to_input.get());
}


void
GPlatesAppLogic::Layer::InputConnection::disconnect()
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		is_valid(),
		GPLATES_ASSERTION_SOURCE);

	// Get the layer that owns 'this' connection.
	Layer layer = get_layer();

	// Get the ReconstructGraph to emit a signal.
	const boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(layer.get_impl());
	ReconstructGraph &reconstruct_graph = layer_impl->get_reconstruct_graph();
	reconstruct_graph.emit_layer_about_to_remove_input_connection(layer, *this);

	// NOTE: this will make 'this' invalid (see @a is_valid) upon returning since
	// there will be no more owning references to 'input_connection_impl'.
	{
		// NOTE: We place this in its own scope because when 'disconnect_from_parent_layer()'
		// is called then 'input_connection_impl' will be the last reference to the input
		// connection impl and we want that to be destroyed before we signal that the layer
		// connection has been removed. And that is because the destructor of
		// 'ReconstructGraphImpl::LayerInputConnection' internally notifies any connected layers
		// that the connection is being removed and we want that app-logic state to be consistent
		// before we let the outside world know of the disconnection via signals.
		boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> input_connection_impl(d_impl);
		input_connection_impl->disconnect_from_parent_layer();
	}

	// Get the ReconstructGraph to emit another signal.
	reconstruct_graph.emit_layer_removed_input_connection(layer);
}
