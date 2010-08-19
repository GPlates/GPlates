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

#include <algorithm>
#include <cstddef> // For std::size_t
#include <iterator>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "ApplicationState.h"
#include "LayerTask.h"
#include "ReconstructGraph.h"
#include "ReconstructGraphImpl.h"
#include "ReconstructUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ReconstructGraph::ReconstructGraph(
		ApplicationState &application_state) :
	d_application_state(application_state)
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

	// If the new layer is a reconstruction tree layer then set it as the
	// default reconstruction tree layer.
	if (layer.get_output_definition() == Layer::OUTPUT_RECONSTRUCTION_TREE_DATA)
	{
		set_default_reconstruction_tree_layer(layer);
	}

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
	// This will be the last owning reference to the layer and so it will get destroyed
	// upon returning from this method.
	d_layers.remove(layer_impl);
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
				default_reconstruction_tree_layer.get_output_definition() ==
						Layer::OUTPUT_RECONSTRUCTION_TREE_DATA,
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
GPlatesAppLogic::ReconstructGraph::execute_layer_tasks(
		const double &reconstruction_time)
{
	// Create a Reconstruction to store the outputs of each layer that generates
	// reconstruction geometries.
	// The default reconstruction tree will be the empty tree unless we later
	// find a valid default reconstruction tree layer.
	Reconstruction::non_null_ptr_type reconstruction =
			Reconstruction::create(
					reconstruction_time,
					d_application_state.get_current_anchored_plate_id());

	// Get a shared reference to the default reconstruction tree layer if there is one.
	layer_ptr_type default_reconstruction_tree_layer;
	if (d_default_reconstruction_tree_layer.is_valid() &&
		// FIXME: Should probably handle this elsewhere so we don't have to check here...
		d_default_reconstruction_tree_layer.get_output_definition() ==
			Layer::OUTPUT_RECONSTRUCTION_TREE_DATA)
	{
		default_reconstruction_tree_layer =
				layer_ptr_type(d_default_reconstruction_tree_layer.get_impl());
	}

	// Traverse the dependency graph to determine the order in which layers should be executed
	// to that layers requiring input from other layers get executed later.
	const std::vector<layer_ptr_type> dependency_ordered_layers =
			get_layer_dependency_order(default_reconstruction_tree_layer);

	// Iterate over the layers and execute them in the correct order.
	BOOST_FOREACH(const layer_ptr_type &layer, dependency_ordered_layers)
	{
		// Pass a layer handle, that clients can use, when executing layer tasks.
		const Layer layer_handle(layer);

		// Execute the layer task.
		layer->execute(
				layer_handle,
				*reconstruction,
				d_application_state.get_current_anchored_plate_id());

		// If the layer just executed is the default reconstruction tree layer then
		// store its output in the Reconstruction object.
		// The Reconstruction object will get queried by subsequent layers for the default
		// reconstruction tree.
		// NOTE: There should be no danger of another layer querying the default reconstruction
		// tree before its generated due to the dependency order of execution of the layers.
		if (layer == default_reconstruction_tree_layer)
		{
			// Get the output of the default reconstruction tree layer.
			boost::optional<ReconstructionTree::non_null_ptr_to_const_type>
					default_reconstruction_tree =
							d_default_reconstruction_tree_layer.get_output_data<
									ReconstructionTree::non_null_ptr_to_const_type>();
			if (default_reconstruction_tree)
			{
				// Store it in the output Reconstruction.
				reconstruction->set_default_reconstruction_tree(default_reconstruction_tree.get());
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

	// Partition all layers into:
	// * topological layers and their dependency ancestors, and
	// * the rest.
	// Topological layers are treated separately because features in topological layers
	// can reference features in other layers even though the layers are not connected.
	// Dependency ancestors of topological layers are grouped with the topological layers because
	// they explicitly depend on the topological layers and hence must be executed after them.
	std::set<layer_ptr_type> topological_layers_and_ancestors;
	std::set<layer_ptr_type> other_layers;
	partition_topological_layers_and_their_dependency_ancestors(
			topological_layers_and_ancestors, other_layers);

	// The final sequence of dependency ordered layers to return to the caller.
	std::vector<layer_ptr_type> dependency_ordered_layers;
	dependency_ordered_layers.reserve(num_layers);

	// Keep track of all layers visited when traversing dependency graph below.
	std::set<layer_ptr_type> all_layers_visited;

	// If there's a default reconstruction tree layer then it should get executed first
	// since layer's can be implicitly connected to it (that is they can be connected to
	// it even though they have no explicit connections to it) - and since we pnly follow
	// explicit connections in the dependency graph we'll need to treat this as a special case.
	if (default_reconstruction_tree_layer)
	{
		// The default reconstruction tree layer will be in the non-topological layers.
		std::set<layer_ptr_type>::iterator default_recon_tree_layer_iter =
				other_layers.find(default_reconstruction_tree_layer);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				default_recon_tree_layer_iter != other_layers.end(),
				GPLATES_ASSERTION_SOURCE);

		// Remove from the other layers list and add it here instead.
		other_layers.erase(default_recon_tree_layer_iter);

		find_layer_dependency_order(
				default_reconstruction_tree_layer, dependency_ordered_layers, all_layers_visited);
	}

	// Next iterate iterate over the other layers to build a dependency ordering.
	BOOST_FOREACH(const layer_ptr_type &layer, other_layers)
	{
		find_layer_dependency_order(layer, dependency_ordered_layers, all_layers_visited);
	}

	// Next iterate iterate over the topological layers and their dependency ancestors
	// to build a dependency ordering.
	// This is done after the other layers since features in the topological layers
	// can implicitly reference features in the other layers.
	BOOST_FOREACH(const layer_ptr_type &layer, topological_layers_and_ancestors)
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
GPlatesAppLogic::ReconstructGraph::partition_topological_layers_and_their_dependency_ancestors(
		std::set<layer_ptr_type> &topological_layers_and_ancestors,
		std::set<layer_ptr_type> &other_layers) const
{
	// First search all layers for any topological layers.
	BOOST_FOREACH(const layer_ptr_type &layer, d_layers)
	{
		if (layer->get_layer_task()->is_topological_layer_task())
		{
			// Add the current layer to the list of topological layers and their dependency ancestors.
			topological_layers_and_ancestors.insert(layer);

			// Add any dependency ancestor layers.
			find_dependency_ancestors_of_layer(
					layer, topological_layers_and_ancestors);
		}
	}

	// Insert the other remaining layers into the caller's sequence.
	BOOST_FOREACH(const layer_ptr_type &layer, d_layers)
	{
		if (topological_layers_and_ancestors.find(layer) ==
			topological_layers_and_ancestors.end())
		{
			other_layers.insert(layer);
		}
	}
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

	// Get the input file to disconnect all connections that use it as input.
	input_file_impl->disconnect_output_connections();

	// Remove the input file object.
	d_input_files.erase(input_file_iter);

	// Iterate over all layers and remove any layer that now has no inputs on its main channel.
	layer_ptr_seq_type::iterator layers_iter = d_layers.begin();
	layer_ptr_seq_type::iterator layers_end = d_layers.end();
	for ( ; layers_iter != layers_end; )
	{
		Layer layer(*layers_iter);

		const QString main_input_channel = layer.get_main_input_feature_collection_channel();

		// Increment the layer list iterator in case we remove the layer.
		++layers_iter;

		if (layer.get_channel_inputs(main_input_channel).empty())
		{
			remove_layer(layer);
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

