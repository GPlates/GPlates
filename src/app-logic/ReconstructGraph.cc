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

#include <algorithm>
#include <cstddef> // For std::size_t
#include <iterator>
#include <list>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include <QDebug>
#include "Serialization.h"


#include "ApplicationState.h"
#include "LayerProxyUtils.h"
#include "LayerTask.h"
#include "ReconstructGraph.h"
#include "ReconstructGraphImpl.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructUtils.h"
#include "Serialization.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ReconstructGraph::ReconstructGraph(
		ApplicationState &application_state) :
	d_application_state(application_state),
	d_identity_rotation_reconstruction_layer_proxy(
			ReconstructionLayerProxy::create(1/*max_num_reconstruction_trees_in_cache*/))
{
}


GPlatesAppLogic::Layer
GPlatesAppLogic::ReconstructGraph::add_layer(
		const boost::shared_ptr<LayerTask> &layer_task)
{
	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(
			new ReconstructGraphImpl::Layer(layer_task, *this));

	// Need to explicitly set the outputting layer for the output data.
	// Has to be done outside Layer constructor since it needs a weak reference
	// to the layer.
	layer_impl->get_output_data()->set_outputting_layer(layer_impl);

	// Keep a reference to the layer to keep it alive.
	d_layers.push_back(layer_impl);

	// Wrap in a weak ref for the caller and so we can use our own public interface.
	const Layer layer(layer_impl);

	// Let clients know of the new layer.
	emit layer_added(*this, layer);

	// Return the weak reference.
	return layer;
}


void
GPlatesAppLogic::ReconstructGraph::remove_layer(
		Layer layer)
{
	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		layer.is_valid(),
		GPLATES_ASSERTION_SOURCE);

	// If we're removing the default reconstruction tree layer then
	// set the new default reconstruction tree layer as none.
	if (layer == d_default_reconstruction_tree_layer)
	{
		set_default_reconstruction_tree_layer();
	}

	// Deactivate the layer which will emit a signal if the layer is currently active.
	layer.activate(false);

	// Let clients know the layer is about to be removed.
	emit layer_about_to_be_removed(*this, layer);

	// Convert from weak_ptr.
	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(layer.get_impl());

	// Remove the layer.
	d_layers.remove(layer_impl);

	// We have the last owning reference to the layer and so it will get destroyed here.
	layer_impl.reset();

	// Let clients know a layer has been removed.
	emit layer_removed(*this);
}


GPlatesAppLogic::Layer::InputFile
GPlatesAppLogic::ReconstructGraph::get_input_file(
		const FeatureCollectionFileState::file_reference input_file)
{
	// Search for the input file.
	input_file_ptr_map_type::const_iterator input_file_iter = d_input_files.find(input_file);

	// We should have all currently loaded files covered.
	// If the specified file cannot be found then something is broken.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			input_file_iter != d_input_files.end(),
			GPLATES_ASSERTION_SOURCE);

	// Return to caller as a weak reference.
	return Layer::InputFile(input_file_iter->second);
}


void
GPlatesAppLogic::ReconstructGraph::set_default_reconstruction_tree_layer(
		const Layer &default_reconstruction_tree_layer)
{
	// If the default reconstruction tree layer isn't changing then do nothing.
	if (default_reconstruction_tree_layer == d_default_reconstruction_tree_layer)
	{
		return;
	}

	const Layer prev_default_reconstruction_tree_layer = d_default_reconstruction_tree_layer;

	if (default_reconstruction_tree_layer.is_valid())
	{
		// Make sure we've been passed a reconstruction tree layer.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				default_reconstruction_tree_layer.get_type() == LayerTaskType::RECONSTRUCTION,
				GPLATES_ASSERTION_SOURCE);
		d_default_reconstruction_tree_layer = default_reconstruction_tree_layer;
	}
	else
	{
		// If the weak reference is invalid then there will be no default reconstruction tree layer.
		d_default_reconstruction_tree_layer = Layer();
	}

	// Let clients know of the new default reconstruction tree layer.
	emit default_reconstruction_tree_layer_changed(
			*this,
			prev_default_reconstruction_tree_layer,
			d_default_reconstruction_tree_layer);
}


GPlatesAppLogic::Layer
GPlatesAppLogic::ReconstructGraph::get_default_reconstruction_tree_layer() const
{
	return d_default_reconstruction_tree_layer;
}


GPlatesAppLogic::Reconstruction::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructGraph::update_layer_tasks(
		const double &reconstruction_time)
{
	// Create a Reconstruction to store the layer proxies of each *active* layer.
	// And the default reconstruction layer proxy will perform identity rotations unless we later
	// find a valid default reconstruction layer proxy.
	// NOTE: The specified reconstruction layer proxy will only get used if there are no
	// reconstruction tree layers loaded.
	// Also by keeping the same instance over time we avoid layers continually updating themselves,
	// when it's unnecessary, because they think the default reconstruction layer is constantly
	// being switched.
	// 
	// FIXME: Having to update the identity reconstruction layer proxy to prevent problems in other
	// area is just dodgy. This whole default reconstruction tree layer has to be reevaluated.
	d_identity_rotation_reconstruction_layer_proxy->set_current_reconstruction_time(reconstruction_time);
	d_identity_rotation_reconstruction_layer_proxy->set_current_anchor_plate_id(
			d_application_state.get_current_anchored_plate_id());
	Reconstruction::non_null_ptr_type reconstruction =
			Reconstruction::create(
					reconstruction_time,
					d_identity_rotation_reconstruction_layer_proxy);

	// Get a shared reference to the default reconstruction tree layer if there is one.
	layer_ptr_type default_reconstruction_tree_layer;
	if (d_default_reconstruction_tree_layer.is_valid() &&
		// FIXME: Should probably handle this elsewhere so we don't have to check here...
		d_default_reconstruction_tree_layer.get_type() == LayerTaskType::RECONSTRUCTION)
	{
		default_reconstruction_tree_layer =
				layer_ptr_type(d_default_reconstruction_tree_layer.get_impl());
	}

	// Traverse the dependency graph to determine the order in which layers should be executed
	// so that layers requiring input from other layers get executed later.
	const std::vector<layer_ptr_type> dependency_ordered_layers =
			get_layer_dependency_order(default_reconstruction_tree_layer);

	// Iterate over the layers and update them in the correct order.
	BOOST_FOREACH(const layer_ptr_type &layer, dependency_ordered_layers)
	{
		// Pass a layer handle, that clients can use, when executing layer tasks.
		const Layer layer_handle(layer);

		// Execute the layer task.
		layer->update_layer_task(
				layer_handle,
				*reconstruction,
				d_application_state.get_current_anchored_plate_id());

		// If the layer just executed is the default reconstruction layer then
		// store its output in the Reconstruction object.
		// The Reconstruction object will get queried by subsequent layers for the default
		// reconstruction layer proxy.
		// NOTE: There should be no danger of another layer querying the default reconstruction
		// layer proxy before its set due to the dependency order of execution of the layers.
		if (layer == default_reconstruction_tree_layer)
		{
			// Get the output of the default reconstruction tree layer.
			boost::optional<ReconstructionLayerProxy::non_null_ptr_type>
					default_reconstruction_tree_layer_proxy =
							layer_handle.get_layer_output<ReconstructionLayerProxy>();
			if (default_reconstruction_tree_layer_proxy)
			{
				// Store it in the output Reconstruction.
				reconstruction->set_default_reconstruction_layer_output(
						default_reconstruction_tree_layer_proxy.get());
			}
		}
	}

	return reconstruction;
}


std::vector<GPlatesAppLogic::ReconstructGraph::layer_ptr_type>
GPlatesAppLogic::ReconstructGraph::get_layer_dependency_order(
		const layer_ptr_type &default_reconstruction_tree_layer) const
{
	//
	// Iterate over the layers traverse the dependency graph to determine execution order.
	//
	const std::size_t num_layers = d_layers.size();

	// Add all the layers to a list.
	std::list<layer_ptr_type> layers(d_layers.begin(), d_layers.end());

	// The final sequence of dependency ordered layers to return to the caller.
	std::vector<layer_ptr_type> dependency_ordered_layers;
	dependency_ordered_layers.reserve(num_layers);

	// Keep track of all layers visited when traversing dependency graph below.
	std::set<layer_ptr_type> all_layers_visited;

	// If there's a default reconstruction tree layer then it should get executed first
	// since layer's can be implicitly connected to it (that is they can be connected to
	// it even though they have no explicit connections to it) - and since we only follow
	// explicit connections in the dependency graph we'll need to treat this as a special case.
	if (default_reconstruction_tree_layer)
	{
		// The default reconstruction tree layer will be in the non-topological layers.
		std::list<layer_ptr_type>::iterator default_recon_tree_layer_iter =
			std::find(layers.begin(), layers.end(), default_reconstruction_tree_layer);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				default_recon_tree_layer_iter != layers.end(),
				GPLATES_ASSERTION_SOURCE);

		// Remove from the other layers list and add it here instead.
		layers.erase(default_recon_tree_layer_iter);

		find_layer_dependency_order(
				default_reconstruction_tree_layer, dependency_ordered_layers, all_layers_visited);
	}

	// Next iterate iterate over the remaining layers to build a dependency ordering.
	BOOST_FOREACH(const layer_ptr_type &layer, layers)
	{
		find_layer_dependency_order(layer, dependency_ordered_layers, all_layers_visited);
	}

	// The number of layers returned should be equal to the total number of layers.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			dependency_ordered_layers.size() == num_layers,
			GPLATES_ASSERTION_SOURCE);

	return dependency_ordered_layers;
}


void
GPlatesAppLogic::ReconstructGraph::find_dependency_ancestors_of_layer(
		const layer_ptr_type &layer,
		std::set<layer_ptr_type> &ancestor_layers) const
{
	// Iterate over the output connections of 'layer'.
	const ReconstructGraphImpl::Data::connection_seq_type &layer_output_connections =
			layer->get_output_data()->get_output_connections();
	BOOST_FOREACH(
			const ReconstructGraphImpl::LayerInputConnection *layer_output_connection,
			layer_output_connections)
	{
		const layer_ptr_type parent_layer(layer_output_connection->get_layer_receiving_input());

		// Insert the current parent layer into the sequence of ancestor layers.
		// If it has already been visited/inserted then continue to the next parent layer.
		if (!ancestor_layers.insert(parent_layer).second)
		{
			continue;
		}

		// Traverse ancestor graph of the current parent layer.
		find_dependency_ancestors_of_layer(parent_layer, ancestor_layers);
	}
}


void
GPlatesAppLogic::ReconstructGraph::find_layer_dependency_order(
		const layer_ptr_type &layer,
		std::vector<layer_ptr_type> &dependency_ordered_layers,
		std::set<layer_ptr_type> &all_layers_visited) const
{
	// Attempt to add the current layer to list of all layers visited.
	if (!all_layers_visited.insert(layer).second)
	{
		// The current layer has already been visited so nothing needs to be done.
		return;
	}

	// Iterate over the input connections of 'layer'.
	const ReconstructGraphImpl::LayerInputConnections::connection_seq_type layer_input_connections =
			layer->get_input_connections().get_input_connections();
	BOOST_FOREACH(
			const boost::shared_ptr<ReconstructGraphImpl::LayerInputConnection> &layer_input_connection,
			layer_input_connections)
	{
		// See if the current input connection connects to the output of another layer.
		const boost::optional< boost::weak_ptr<ReconstructGraphImpl::Layer> > layer_child_weak_ref =
				layer_input_connection->get_input_data()->get_outputting_layer();
		if (layer_child_weak_ref)
		{
			const layer_ptr_type layer_child(layer_child_weak_ref.get());

			// Traverse the dependency graph of the current child layer.
			find_layer_dependency_order(
					layer_child, dependency_ordered_layers, all_layers_visited);
		}
	}

	// Add the current layer after all child layers have been added.
	dependency_ordered_layers.push_back(layer);
}


void
GPlatesAppLogic::ReconstructGraph::handle_file_state_files_added(
		FeatureCollectionFileState &file_state,
		const std::vector<FeatureCollectionFileState::file_reference> &new_files)
{
	BOOST_FOREACH(FeatureCollectionFileState::file_reference new_file, new_files)
	{
		// Wrap a new Data object around the file.
		const boost::shared_ptr<ReconstructGraphImpl::Data> input_file_impl(
				new ReconstructGraphImpl::Data(new_file));

		// Add to our internal mapping of file indices to InputFile's.
		std::pair<input_file_ptr_map_type::const_iterator, bool> insert_result =
				d_input_files.insert(std::make_pair(new_file, input_file_impl));

		// The file shouldn't already exist in the map.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				insert_result.second,
				GPLATES_ASSERTION_SOURCE);
	}
}


void
GPlatesAppLogic::ReconstructGraph::handle_file_state_file_about_to_be_removed(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileState::file_reference file_about_to_be_removed)
{
	// Search for the file that's about to be removed.
	const input_file_ptr_map_type::iterator input_file_iter =
			d_input_files.find(file_about_to_be_removed);

	// We should be able to find the file in our internal map.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			input_file_iter != d_input_files.end(),
			GPLATES_ASSERTION_SOURCE);

	// The input file corresponding to the file about to be removed.
	ReconstructGraphImpl::Data *input_file_impl = input_file_iter->second.get();

#if 1 // TODO: Remove this when the user can explicit remove a layer.
	// Iterate over all layers that are directly connected to the file about to be removed
	// and remove any layers that:
	//   1) reference that file on their *main* input channel, *and*
	//   2) have no other input files at the *main* input channel.
	// This effectively removes layers that currently have *one* input file on thei
	// main input channel that's about to be removed.
	// NOTE: This avoids removing layers that, by default, never have any input on their
	// main channel - this is a relatively new occurrence that wasn't initially planned for
	// (ie, all layers were expected to have some input on their main channel) but will
	// happen when the Small Circle layer is implemented - as this layer will have input
	// data entered by the user (in a dialog) and passed to the app-logic layer via a
	// LayerTaskParams derived class.
	std::vector<Layer> layers_to_remove;
	BOOST_FOREACH(
			ReconstructGraphImpl::LayerInputConnection *input_file_connection,
			input_file_impl->get_output_connections())
	{
		const layer_ptr_type layer_receiving_file_input(
				input_file_connection->get_layer_receiving_input());

		if (!layer_receiving_file_input)
		{
			continue;
		}

		Layer layer(layer_receiving_file_input);

		const QString main_input_channel = layer.get_main_input_feature_collection_channel();

		const std::vector<Layer::InputConnection> input_connections =
				layer.get_channel_inputs(main_input_channel);
		// We only remove layers that currently have one input file on the main channel
		// (that's about to be removed).
		if (input_connections.size() != 1)
		{
			continue;
		}

		// Make sure the input connects to a file rather than the output of another layer.
		boost::optional<Layer::InputFile> input_file = input_connections[0].get_input_file();
		if (!input_file)
		{
			continue;
		}

		// If the sole input file on the main channel matches the file about to be removed.
		if (input_file->get_file() == file_about_to_be_removed)
		{
			layers_to_remove.push_back(layer);
		}
	}

	// Remove any layers that need removing.
	BOOST_FOREACH(const Layer &layer_to_remove, layers_to_remove)
	{
		remove_layer(layer_to_remove);
	}
#endif

	// Get the input file to disconnect all connections that use it as input.
	input_file_impl->disconnect_output_connections();

	// Remove the input file object.
	d_input_files.erase(input_file_iter);
}


void
GPlatesAppLogic::ReconstructGraph::debug_reconstruct_graph_state()
{
	qDebug() << "\nRECONSTRUCT GRAPH:-";
	qDebug() << " INPUT FILES:-";
	for (input_file_ptr_map_type::const_iterator it = d_input_files.begin(); it != d_input_files.end(); ++it) {
		qDebug() << "   " << it->first.get_file().get_file_info().get_display_name(false);
	}

	qDebug() << " LAYERS:-";
	BOOST_FOREACH(layer_ptr_type l_ptr, d_layers) {
		const char *is_active = l_ptr->is_active()? "A" : " ";
		const LayerTask &layer_task = l_ptr->get_layer_task();
		qDebug() << "   " << is_active << "Type =" << layer_task.get_layer_type();
		qDebug() << "      CONNECTIONS:-";
		const ReconstructGraphImpl::LayerInputConnections &lics = l_ptr->get_input_connections();
		const ReconstructGraphImpl::LayerInputConnections::input_connection_map_type &licmap =
				lics.get_input_connection_map();
		for (ReconstructGraphImpl::LayerInputConnections::input_connection_map_type::const_iterator it = licmap.begin();
			  it != licmap.end(); ++it) {
			boost::optional<FeatureCollectionFileState::file_reference> fileref = it->second->get_input_data()->get_input_file();
			if (fileref) {
				qDebug() << "       " << it->first << "<-" << fileref->get_file().get_file_info().get_display_name(false);
			} else {
				qDebug() << "       " << it->first << "<- something dynamic";
			}
		}
	}

}



void
GPlatesAppLogic::ReconstructGraph::emit_layer_activation_changed(
		const Layer &layer,
		bool activation)
{
	emit layer_activation_changed(*this, layer, activation);
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_added_input_connection(
		GPlatesAppLogic::Layer layer,
		GPlatesAppLogic::Layer::InputConnection input_connection)
{
	emit layer_added_input_connection(*this, layer, input_connection);
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_about_to_remove_input_connection(
		GPlatesAppLogic::Layer layer,
		GPlatesAppLogic::Layer::InputConnection input_connection)
{
	emit layer_about_to_remove_input_connection(*this, layer, input_connection);
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_removed_input_connection(
		GPlatesAppLogic::Layer layer)
{
	emit layer_removed_input_connection(*this, layer);
}

