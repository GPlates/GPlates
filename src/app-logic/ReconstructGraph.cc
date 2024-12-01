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
#include <set>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/bind/bind.hpp>

#include <QDebug>


#include "ApplicationState.h"
#include "LayerParams.h"
#include "LayerProxyUtils.h"
#include "LayerTask.h"
#include "LayerTaskRegistry.h"
#include "ReconstructGraph.h"
#include "ReconstructGraphImpl.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesAppLogic::ReconstructGraph::ReconstructGraph(
		ApplicationState &application_state) :
	d_application_state(application_state),
	d_layer_task_registry(application_state.get_layer_task_registry()),
	d_identity_rotation_reconstruction_layer_proxy(
			ReconstructionLayerProxy::create(1/*max_num_reconstruction_trees_in_cache*/)),
	d_add_or_remove_layers_group_nested_count(0)
{
}


void
GPlatesAppLogic::ReconstructGraph::add_files(
		const std::vector<FeatureCollectionFileState::file_reference> &files,
		boost::optional<AutoCreateLayerParams> auto_create_layers)
{
	std::vector<Layer::InputFile> input_files;

	// Add all the files to our graph first before we create any layers.
	BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file, files)
	{
		input_files.push_back(add_file_internal(file));
	}

	// Any auto-creation of layers is done after *all* files have been added to the graph.
	// This is in case any clients attempt to access any of the files when the auto-creation
	// of layers emits signals (that clients connect to).
	if (auto_create_layers)
	{
		AddOrRemoveLayersGroup add_layers_group(*this);
		add_layers_group.begin_add_or_remove_layers();

		BOOST_FOREACH(const Layer::InputFile &input_file, input_files)
		{
			auto_create_layers_for_new_input_file(input_file, auto_create_layers.get());
		}

		// Now that all new layers have been created we can make auto-connections.
		auto_connect_layers();

		add_layers_group.end_add_or_remove_layers();
	}
}


GPlatesAppLogic::Layer::InputFile
GPlatesAppLogic::ReconstructGraph::add_file(
		const FeatureCollectionFileState::file_reference &file,
		boost::optional<AutoCreateLayerParams> auto_create_layers)
{
	const Layer::InputFile input_file = add_file_internal(file);

	if (auto_create_layers)
	{
		auto_create_layers_for_new_input_file(input_file, auto_create_layers.get());

		// Now that all new layers have been created we can make auto-connections.
		auto_connect_layers();
	}

	return input_file;
}


GPlatesAppLogic::Layer::InputFile
GPlatesAppLogic::ReconstructGraph::add_file_internal(
		const FeatureCollectionFileState::file_reference &file)
{
	// Wrap a new Data object around the file.
	const boost::shared_ptr<ReconstructGraphImpl::Data> input_file_impl(
			new ReconstructGraphImpl::Data(file));

	// The input file entry stored in the map of input file references to input files.
	const InputFileInfo input_file_info(*this, input_file_impl);

	// Add to our internal mapping of file indices to InputFile's.
	std::pair<input_file_info_map_type::const_iterator, bool> insert_result =
			d_input_files.insert(std::make_pair(file, input_file_info));

	// The file shouldn't already exist in the map.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			insert_result.second,
			GPLATES_ASSERTION_SOURCE);

	// The input file to return to the caller as a weak reference.
	return Layer::InputFile(input_file_impl);
}


void
GPlatesAppLogic::ReconstructGraph::remove_file(
		const FeatureCollectionFileState::file_reference &file)
{
	// Search for the file that's about to be removed.
	const input_file_info_map_type::iterator input_file_iter = d_input_files.find(file);

	// We should be able to find the file in our internal map.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			input_file_iter != d_input_files.end(),
			GPLATES_ASSERTION_SOURCE);

	// The input file corresponding to the file about to be removed.
	const InputFileInfo &input_file_info = input_file_iter->second;

	const input_file_ptr_type input_file_ptr = input_file_info.get_input_file_ptr();

	// Destroy auto-created layers for the file about to be removed.
	auto_destroy_layers_for_input_file_about_to_be_removed(Layer::InputFile(input_file_ptr));

	// Get the input file to disconnect all connections that use it as input.
	input_file_ptr->disconnect_output_connections();

	// Remove the input file object.
	d_input_files.erase(input_file_iter);
}


GPlatesAppLogic::Layer::InputFile
GPlatesAppLogic::ReconstructGraph::get_input_file(
		const FeatureCollectionFileState::file_reference input_file)
{
	// Search for the input file.
	input_file_info_map_type::const_iterator input_file_iter = d_input_files.find(input_file);

	// We should have all currently loaded files covered.
	// If the specified file cannot be found then something is broken.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			input_file_iter != d_input_files.end(),
			GPLATES_ASSERTION_SOURCE);

	// The input file info corresponding to the file.
	const InputFileInfo &input_file_info = input_file_iter->second;

	const input_file_ptr_type input_file_ptr = input_file_info.get_input_file_ptr();

	// Return to caller as a weak reference.
	return Layer::InputFile(input_file_ptr);
}


GPlatesAppLogic::Layer
GPlatesAppLogic::ReconstructGraph::add_layer(
		const boost::shared_ptr<LayerTask> &layer_task)
{
	// Make sure each layer addition is part of an add layers group.
	AddOrRemoveLayersGroup add_layers_group(*this);
	add_layers_group.begin_add_or_remove_layers();

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
	Q_EMIT layer_added(*this, layer);

	// Emit signal each time the layer's task parameters are modified.
	QObject::connect(
			layer_task->get_layer_params().get(), SIGNAL(modified(GPlatesAppLogic::LayerParams &)),
			this, SLOT(handle_layer_params_changed(GPlatesAppLogic::LayerParams &)));

	// End the add layers group.
	add_layers_group.end_add_or_remove_layers();

	// Return the weak reference.
	return layer;
}


void
GPlatesAppLogic::ReconstructGraph::remove_layer(
		Layer layer)
{
	// Make sure each layer removal is part of a remove layers group.
	AddOrRemoveLayersGroup remove_layers_group(*this);
	remove_layers_group.begin_add_or_remove_layers();

	// Throw our own exception to track location of throw.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		layer.is_valid(),
		GPLATES_ASSERTION_SOURCE);

	// If the layer being removed is the current default reconstruction tree layer then
	// remove it as a default reconstruction tree layer.
	// Also handles case where layer being removed is a previous default reconstruction tree layer.
	handle_default_reconstruction_tree_layer_removal(layer);

	// Deactivate the layer which will emit a signal if the layer is currently active.
	layer.activate(false);

	// Let clients know the layer is about to be removed.
	Q_EMIT layer_about_to_be_removed(*this, layer);

	// Convert from weak_ptr.
	boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(layer.get_impl());

	// Remove the layer.
	d_layers.remove(layer_impl);

	// We have the last owning reference to the layer and so it will get destroyed here.
	layer_impl.reset();

	// Let clients know a layer has been removed.
	Q_EMIT layer_removed(*this);

	// End the remove layers group.
	remove_layers_group.end_add_or_remove_layers();
}


void
GPlatesAppLogic::ReconstructGraph::set_default_reconstruction_tree_layer(
		const Layer &new_default_reconstruction_tree_layer)
{
	// Make sure we've been passed a valid reconstruction tree layer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			new_default_reconstruction_tree_layer.get_type() == LayerTaskType::RECONSTRUCTION &&
					new_default_reconstruction_tree_layer.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	const Layer prev_default_reconstruction_tree_layer = get_default_reconstruction_tree_layer();

	// If the default reconstruction tree layer isn't changing then do nothing.
	if (new_default_reconstruction_tree_layer == prev_default_reconstruction_tree_layer)
	{
		return;
	}

	d_default_reconstruction_tree_layer_stack.push_back(new_default_reconstruction_tree_layer);

	// Let clients know of the new default reconstruction tree layer.
	Q_EMIT default_reconstruction_tree_layer_changed(
			*this,
			prev_default_reconstruction_tree_layer,
			new_default_reconstruction_tree_layer);
}


GPlatesAppLogic::Layer
GPlatesAppLogic::ReconstructGraph::get_default_reconstruction_tree_layer() const
{
	if (d_default_reconstruction_tree_layer_stack.empty())
	{
		return Layer();
	}

	return d_default_reconstruction_tree_layer_stack.back();
}


GPlatesAppLogic::Reconstruction::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructGraph::update_layer_tasks(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plated_id)
{
	PROFILE_FUNC();

	// Determine a default reconstruction layer proxy.
	boost::optional<ReconstructionLayerProxy::non_null_ptr_type> default_reconstruction_layer_proxy;

	// If we have a default reconstruction tree layer that's active then use that.
	if (get_default_reconstruction_tree_layer().is_valid() &&
		get_default_reconstruction_tree_layer().is_active() &&
		// FIXME: Should probably handle this elsewhere so we don't have to check here...
		get_default_reconstruction_tree_layer().get_type() == LayerTaskType::RECONSTRUCTION)
	{
		// Get the output of the default reconstruction tree layer.
		default_reconstruction_layer_proxy =
				get_default_reconstruction_tree_layer().get_layer_output<ReconstructionLayerProxy>();
	}

	// Otherwise use the identity rotation reconstruction layer proxy.
	if (!default_reconstruction_layer_proxy)
	{
		// NOTE: The specified reconstruction layer proxy will only get used if there are no
		// reconstruction tree layers loaded.
		// Also by keeping the same instance over time we avoid layers continually updating themselves,
		// when it's unnecessary, because they think the default reconstruction layer is constantly
		// being switched.
		//
		// FIXME: Having to update the identity reconstruction layer proxy to prevent problems in other
		// area is just dodgy. This whole default reconstruction tree layer has to be reevaluated.
		d_identity_rotation_reconstruction_layer_proxy->set_current_reconstruction_time(reconstruction_time);
		d_identity_rotation_reconstruction_layer_proxy->set_current_anchor_plate_id(anchored_plated_id);

		default_reconstruction_layer_proxy = d_identity_rotation_reconstruction_layer_proxy;
	}

	// Create a Reconstruction to store the layer proxies of each *active* layer and
	// the default reconstruction layer proxy.
	Reconstruction::non_null_ptr_type reconstruction =
			Reconstruction::create(
					reconstruction_time,
					anchored_plated_id,
					default_reconstruction_layer_proxy.get());

	// Iterate over the layers and add the active ones to the Reconstruction object.
	// We do this loop first so we can then pass the Reconstruction to all layers as we update
	// them in the second loop - some layers like topology layers references other layers without
	// going through their input channels and hence need to know about all active layers.
	BOOST_FOREACH(const layer_ptr_type &layer, d_layers)
	{
		// If this layer is not active then we don't add the layer proxy to the current reconstruction.
		if (!layer->is_active())
		{
			continue;
		}

		// Add the layer output (proxy) to the reconstruction.
		reconstruction->add_active_layer_output(layer->get_layer_task().get_layer_proxy());
	}

	// Iterate over the layers again and update them.
	//
	// Note: The layers can be updated in any order.
	//       This is because the layers now operate in a pull model where a layer directly makes requests
	//       to its dependencies layers and so on. Whereas previously, layers operated in a push model
	//       that required dependency layers to produce output before the executing layers that
	//       depended on them thus requiring layers to be executed in dependency order.
	BOOST_FOREACH(const layer_ptr_type &layer, d_layers)
	{
		// If this layer is not active then we don't add the layer proxy to the current reconstruction.
		if (!layer->is_active())
		{
			continue;
		}

		// Update the layer's task.
		layer->get_layer_task().update(reconstruction);
	}

	return reconstruction;
}


void
GPlatesAppLogic::ReconstructGraph::modified_input_file(
		const Layer::InputFile &input_file)
{
	//
	// First iterate over the output connections of the modified input file to find all layer
	// types that are currently processing the input file.
	//

	// The current set of layer types that are processing the input file.
	// A layer processes an input file when that file is connected to the *main* input channel of the layer.
	std::set<LayerTaskType::Type> layer_types_processing_input_file;

	const ReconstructGraphImpl::Data::connection_seq_type &output_connections =
			input_file_ptr_type(input_file.get_impl())->get_output_connections();
	BOOST_FOREACH(
			const ReconstructGraphImpl::LayerInputConnection *output_connection,
			output_connections)
	{
		const layer_ptr_type layer_receiving_file_input(output_connection->get_layer_receiving_input());

		Layer layer(layer_receiving_file_input);

		const LayerInputChannelName::Type main_input_channel = layer.get_main_input_feature_collection_channel();

		const std::vector<Layer::InputConnection> input_connections = layer.get_channel_inputs(main_input_channel);

		// Iterate over the input connections on the main input channel.
		std::vector<Layer::InputConnection>::const_iterator input_connection_iter = input_connections.begin();
		std::vector<Layer::InputConnection>::const_iterator input_connection_end = input_connections.end();
		for ( ; input_connection_iter != input_connection_end; ++input_connection_iter)
		{
			// If the input connects to a file (ie, not the output of another layer) *and*
			// that file is the input file then we have found a layer that is processing the
			// input file so add the layer type to the list.
			boost::optional<Layer::InputFile> main_channel_input_file = input_connection_iter->get_input_file();
			if (main_channel_input_file &&
				main_channel_input_file.get() == input_file)
			{
				layer_types_processing_input_file.insert(layer.get_type());
				break;
			}
		}
	}

	//
	// The file has changed so find out all layer types that can process the file.
	// This may have changed since we last checked.
	//

	const std::vector<LayerTaskRegistry::LayerTaskType> new_layer_task_types =
			d_layer_task_registry.get_layer_task_types_to_auto_create_for_loaded_file(
					input_file.get_feature_collection());

	//
	// If there are any new layer types not covered by the previous layer types then
	// auto-create respective layers to process the input file.
	// An example is the user saving a topology feature in a feature collection that only
	// contains non-topology features - hence a topology layer will need to be created.
	//
	
	bool created_new_layers = false;
	BOOST_FOREACH(LayerTaskRegistry::LayerTaskType new_layer_task_type, new_layer_task_types)
	{
		// If a layer task of the current type doesn't yet exist then create a layer for it.
		if (layer_types_processing_input_file.find(new_layer_task_type.get_layer_type()) ==
			layer_types_processing_input_file.end())
		{
			const boost::shared_ptr<LayerTask> new_layer_task = new_layer_task_type.create_layer_task();

			auto_create_layer(
					input_file,
					new_layer_task,
					// We don't want to set a new default reconstruction tree layer if one gets
					// created because it might surprise the user (ie, they're not loading a rotation file).
					AutoCreateLayerParams(false/*update_default_reconstruction_tree_layer_*/));
			created_new_layers = true;
		}
	}

	if (created_new_layers)
	{
		// Now that all new layers have been created we can make auto-connections.
		auto_connect_layers();
	}
}


void
GPlatesAppLogic::ReconstructGraph::auto_create_layers_for_new_input_file(
		const Layer::InputFile &input_file,
		const AutoCreateLayerParams &auto_create_layer_params)
{
	//
	// Create a new layer for the input file (or create multiple layers if the feature collection
	// contains features that can be processed by more than one layer type).
	//

	const GPlatesModel::FeatureCollectionHandle::weak_ref input_feature_collection =
			input_file.get_file().get_file().get_feature_collection();

	// Look for layer task types that we should create to process the loaded feature collection.
	const std::vector<LayerTaskRegistry::LayerTaskType> layer_task_types =
			d_layer_task_registry.get_layer_task_types_to_auto_create_for_loaded_file(
					input_feature_collection);

	// Iterate over the compatible layer task types and create layers.
	BOOST_FOREACH(LayerTaskRegistry::LayerTaskType layer_task_type, layer_task_types)
	{
		const boost::optional<boost::shared_ptr<LayerTask> > layer_task =
				layer_task_type.create_layer_task();
		if (layer_task)
		{
			auto_create_layer(input_file, layer_task.get(), auto_create_layer_params);
		}
	}
}


GPlatesAppLogic::Layer
GPlatesAppLogic::ReconstructGraph::auto_create_layer(
		const Layer::InputFile &input_file,
		const boost::shared_ptr<LayerTask> &layer_task,
		const AutoCreateLayerParams &auto_create_layer_params)
{
	// Create a new layer using the layer task.
	// This will emit a signal to notify clients of a new layer.
	Layer new_layer = add_layer(layer_task);

	// Mark the layer as having been auto-created.
	// This will cause the layer to be auto-destroyed when 'input_file' is unloaded.
	new_layer.set_auto_created(true);

	//
	// Connect the file to the input of the new layer.
	//

	// Get the main feature collection input channel for our layer.
	const LayerInputChannelName::Type main_input_feature_collection_channel =
			new_layer.get_main_input_feature_collection_channel();

	// Connect the input file to the main input channel of the new layer.
	//
	// FIXME: This actually gives velocity (visual) layers the name of the input file that caused
	// their auto-creation even though velocity layers no longer have input files (only input layers).
	// This is because the input file connection is still there - just unused and undisplayed
	// in the visual layer - but yet still used to determine the visual layer name.
	// It's somewhat flakey and likely to break in the future.
	new_layer.connect_input_to_file(
			input_file,
			main_input_feature_collection_channel);

	// Set the new default reconstruction tree if we're updating the default *and*
	// the new layer type is a reconstruction tree layer.
	if (auto_create_layer_params.update_default_reconstruction_tree_layer &&
		new_layer.get_type() == LayerTaskType::RECONSTRUCTION)
	{
		set_default_reconstruction_tree_layer(new_layer);
	}

	return new_layer;
}


void
GPlatesAppLogic::ReconstructGraph::auto_connect_layers()
{
	// Make any auto-connections to/from this layer.
	// NOTE: We don't really want to encourage the other connections so currently we only auto-connect
	// velocity layers to topology layers and we will probably try to find a way to avoid this
	// such as grouping layers to make it easier for the user to connect the twelve CitcomS
	// mesh cap files to topologies in one go rather than twelve goes.

	//
	// Do other layer-specific connections.
	// FIXME: Find a way to make it easier for the user to make connections so that these
	// auto-connections are not needed - auto-connections might end up making connections
	// that the user didn't want - eg, connecting *multiple* topologies to velocity layers.
	//

	// Iterate over all layers (receiving input).
	iterator layer_receiving_input_iter = begin();
	iterator layer_receiving_input_end = end();
	for ( ; layer_receiving_input_iter != layer_receiving_input_end; layer_receiving_input_iter++)
	{
		Layer layer_receiving_input = *layer_receiving_input_iter;
		const std::vector<LayerInputChannelType> input_channel_types = 
				layer_receiving_input.get_input_channel_types();

		BOOST_FOREACH(const LayerInputChannelType &input_channel_type, input_channel_types)
		{
			const boost::optional< std::vector<LayerInputChannelType::InputLayerType> > &
					input_channel_layer_types_opt = input_channel_type.get_input_layer_types();
			if (!input_channel_layer_types_opt)
			{
				continue;
			}
			const std::vector<LayerInputChannelType::InputLayerType> &input_channel_layer_types =
					input_channel_layer_types_opt.get();

			const unsigned int num_input_channel_layer_types = input_channel_layer_types.size();
			for (unsigned int input_channel_layer_index = 0;
				input_channel_layer_index < num_input_channel_layer_types;
				++input_channel_layer_index)
			{
				const LayerInputChannelType::InputLayerType &input_channel_layer_type =
						input_channel_layer_types[input_channel_layer_index];

				if (input_channel_layer_type.auto_connect == LayerInputChannelType::DONT_AUTO_CONNECT)
				{
					continue;
				}

				// Iterate over all layers (giving output).
				iterator layer_giving_output_iter = begin();
				iterator layer_giving_output_end = end();
				for ( ; layer_giving_output_iter != layer_giving_output_end; layer_giving_output_iter++)
				{
					// A layer shouldn't receive input from itself.
					if (layer_giving_output_iter == layer_receiving_input_iter)
					{
						continue;
					}
					const Layer layer_giving_output = *layer_giving_output_iter;

					// Skip if the type of the layer giving output does not match the current input type.
					if (input_channel_layer_type.layer_type != layer_giving_output.get_type())
					{
						continue;
					}

					// If can only connect to layers spawned from the same input file then check this.
					if (input_channel_layer_type.auto_connect == LayerInputChannelType::LOCAL_AUTO_CONNECT)
					{
						// FIXME: This relies on the receiving layer having the same name input file as the
						// giving layer. But the receiving layer might no longer need to connect to
						// input files (since might just connect to giving layer instead).
						// So this is somewhat flakey and likely to break in the future.

						// See if both layers (receiving and giving) are connected to the same input file.
						const std::vector<Layer::InputConnection>
								layer_receiving_input_file_connections =
										layer_receiving_input.get_channel_inputs(
												layer_receiving_input.get_main_input_feature_collection_channel());
						const std::vector<Layer::InputConnection>
								layer_giving_output_file_connections =
										layer_giving_output.get_channel_inputs(
												layer_giving_output.get_main_input_feature_collection_channel());

						// We're expecting only one input file connection.
						if (layer_receiving_input_file_connections.size() != 1 ||
							layer_giving_output_file_connections.size() != 1)
						{
							continue;
						}

						// Make sure the inputs connect to a file (rather than the output of another layer).
						boost::optional<Layer::InputFile> layer_receiving_input_file =
								layer_receiving_input_file_connections[0].get_input_file();
						boost::optional<Layer::InputFile> layer_giving_output_file =
								layer_giving_output_file_connections[0].get_input_file();
						if (!layer_receiving_input_file ||
							!layer_giving_output_file)
						{
							continue;
						}

						if (layer_receiving_input_file->get_file() != layer_giving_output_file->get_file())
						{
							continue;
						}
					}

					const std::vector<Layer::InputConnection> input_channel_connections =
							layer_receiving_input.get_channel_inputs(
									input_channel_type.get_input_channel_name());

					// See if the giving layer is already connected to the receiving layer on
					// the current input channel.
					std::vector<Layer::InputConnection>::const_iterator input_channel_connections_iter =
							input_channel_connections.begin();
					std::vector<Layer::InputConnection>::const_iterator input_channel_connections_end =
							input_channel_connections.end();
					for ( ;
						input_channel_connections_iter != input_channel_connections_end;
						++input_channel_connections_iter)
					{
						const Layer::InputConnection &input_channel_connection =
								*input_channel_connections_iter;

						if (layer_giving_output == input_channel_connection.get_input_layer())
						{
							// Already connected.
							break;
						}
					}

					// Connect the giving layer to the receiving layer (if not already connected).
					if (input_channel_connections_iter == input_channel_connections_end)
					{
						layer_receiving_input.connect_input_to_layer_output(
								layer_giving_output,
								input_channel_type.get_input_channel_name());
					}
				}
			}
		}
	}
}


void
GPlatesAppLogic::ReconstructGraph::auto_destroy_layers_for_input_file_about_to_be_removed(
		const Layer::InputFile &input_file_about_to_be_removed)
{
	// Destroy layers that were auto-created from the specified file.
	// NOTE: If the user explicitly created a layer then it will never get removed automatically -
	// the user must also explicitly destroy the layer - this is even the case when all files
	// connected to that layer are unloaded (the user still has to explicitly destroy the layer).

	std::vector<Layer> layers_to_remove;

	// Iterate over the output connections of the input file that's about to be removed.
	const ReconstructGraphImpl::Data::connection_seq_type &output_connections =
			input_file_ptr_type(input_file_about_to_be_removed.get_impl())->get_output_connections();
	BOOST_FOREACH(
			const ReconstructGraphImpl::LayerInputConnection *output_connection,
			output_connections)
	{
		const layer_ptr_type layer_receiving_file_input(output_connection->get_layer_receiving_input());

		Layer layer(layer_receiving_file_input);

		// If the layer was not auto-created then we shouldn't auto-destroy it.
		if (!layer.get_auto_created())
		{
			continue;
		}

		const LayerInputChannelName::Type main_input_channel = layer.get_main_input_feature_collection_channel();

		const std::vector<Layer::InputConnection> input_connections = layer.get_channel_inputs(main_input_channel);
		// We only remove layers that currently have one input file on the main channel.
		if (input_connections.size() != 1)
		{
			continue;
		}

		// Make sure the input connects to a file rather than the output of another layer.
		boost::optional<Layer::InputFile> main_channel_input_file = input_connections[0].get_input_file();
		if (!main_channel_input_file)
		{
			continue;
		}

		// If the sole input file on the main channel matches the file about to be removed then
		// we can remove the layer.
		if (main_channel_input_file.get() == input_file_about_to_be_removed)
		{
			layers_to_remove.push_back(layer);
		}
	}

	// Remove any layers that need removing.
	// We do this last to avoid any issues iterating over layer connections above.
	BOOST_FOREACH(const Layer &layer_to_remove, layers_to_remove)
	{
		remove_layer(layer_to_remove);
	}
}


void
GPlatesAppLogic::ReconstructGraph::handle_default_reconstruction_tree_layer_removal(
		const Layer &layer_being_removed)
{
	// If the layer being removed is one of the current or previous default reconstruction tree
	// layers then remove it from the default reconstruction tree layer stack.
	const default_reconstruction_tree_layer_stack_type::iterator default_recon_tree_iter =
			std::find(
					d_default_reconstruction_tree_layer_stack.begin(),
					d_default_reconstruction_tree_layer_stack.end(),
					layer_being_removed);
	if (default_recon_tree_iter == d_default_reconstruction_tree_layer_stack.end())
	{
		return;
	}
	// If we get here then the layer being removed is either the current or a previous default
	// reconstruction tree layer.

	// If the layer was a previous default then simply remove it from the stack of default layers.
	if (layer_being_removed != get_default_reconstruction_tree_layer())
	{
		// Remove all occurrences in the stack - the same layer may have been the default
		// reconstruction tree layer more than once.
		d_default_reconstruction_tree_layer_stack.erase(
				std::remove(
						d_default_reconstruction_tree_layer_stack.begin(),
						d_default_reconstruction_tree_layer_stack.end(),
						layer_being_removed),
				d_default_reconstruction_tree_layer_stack.end());
		return;
	}
	// If we get here then the layer being removed is the current default reconstruction tree layer.

	// The current default reconstruction tree layer.
	const Layer prev_default_reconstruction_tree_layer = layer_being_removed;

	// Remove all occurrences in the stack - the same layer may have been the default
	// reconstruction tree layer more than once.
	d_default_reconstruction_tree_layer_stack.erase(
			std::remove(
					d_default_reconstruction_tree_layer_stack.begin(),
					d_default_reconstruction_tree_layer_stack.end(),
					layer_being_removed),
			d_default_reconstruction_tree_layer_stack.end());

	// Get the new default reconstruction tree layer if there is one.
	Layer new_default_reconstruction_tree_layer;
	if (!d_default_reconstruction_tree_layer_stack.empty())
	{
		new_default_reconstruction_tree_layer = d_default_reconstruction_tree_layer_stack.back();

		// Make sure the previous default reconstruction tree layer is valid.
		// It should be if we removed any layers from this stack when those layers were removed.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				new_default_reconstruction_tree_layer.is_valid(),
				GPLATES_ASSERTION_SOURCE);
	}

	// Let clients know of the new default reconstruction tree layer even if there are no
	// default reconstruction trees left.
	Q_EMIT default_reconstruction_tree_layer_changed(
			*this,
			prev_default_reconstruction_tree_layer,
			new_default_reconstruction_tree_layer);
}


void
GPlatesAppLogic::ReconstructGraph::handle_layer_params_changed(
		LayerParams &layer_params)
{
	// Find the layer that owns the layer params.
	BOOST_FOREACH(const layer_ptr_type &layer, d_layers)
	{
		if (layer->get_layer_params().get() == &layer_params)
		{
			emit_layer_params_changed(Layer(layer), layer_params);

			return;
		}
	}

	// Shouldn't really be able to get here.
	//
	// However we won't treat it as an error in case a layer's params get modified
	// just as (or after) the layer is getting removed for some reason.
}


void
GPlatesAppLogic::ReconstructGraph::debug_reconstruct_graph_state()
{
	qDebug() << "\nRECONSTRUCT GRAPH:-";
	qDebug() << " INPUT FILES:-";
	for (input_file_info_map_type::const_iterator it = d_input_files.begin(); it != d_input_files.end(); ++it) {
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
GPlatesAppLogic::ReconstructGraph::emit_begin_add_or_remove_layers()
{
	if (d_add_or_remove_layers_group_nested_count == 0)
	{
		Q_EMIT begin_add_or_remove_layers();
	}

	++d_add_or_remove_layers_group_nested_count;
}


void
GPlatesAppLogic::ReconstructGraph::emit_end_add_or_remove_layers()
{
	--d_add_or_remove_layers_group_nested_count;

	if (d_add_or_remove_layers_group_nested_count == 0)
	{
		Q_EMIT end_add_or_remove_layers();
	}
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_activation_changed(
		const Layer &layer,
		bool activation)
{
	Q_EMIT layer_activation_changed(*this, layer, activation);
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_params_changed(
		const Layer &layer,
		LayerParams &layer_params)
{
	Q_EMIT layer_params_changed(*this, layer, layer_params);
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_added_input_connection(
		Layer layer,
		Layer::InputConnection input_connection)
{
	Q_EMIT layer_added_input_connection(*this, layer, input_connection);
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_about_to_remove_input_connection(
		Layer layer,
		Layer::InputConnection input_connection)
{
	Q_EMIT layer_about_to_remove_input_connection(*this, layer, input_connection);
}


void
GPlatesAppLogic::ReconstructGraph::emit_layer_removed_input_connection(
		Layer layer)
{
	Q_EMIT layer_removed_input_connection(*this, layer);
}


GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup::AddOrRemoveLayersGroup(
		ReconstructGraph &reconstruct_graph) :
	d_reconstruct_graph(reconstruct_graph),
	d_inside_group(false)
{
}


GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup::~AddOrRemoveLayersGroup()
{
	if (d_inside_group)
	{
		// Since this is a destructor we cannot let any exceptions escape.
		// If one is thrown we just have to lump it and continue on.
		try
		{
			end_add_or_remove_layers();
		}
		catch (...)
		{
		}
	}
}


void
GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup::begin_add_or_remove_layers()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_inside_group,
			GPLATES_ASSERTION_SOURCE);

	d_reconstruct_graph.emit_begin_add_or_remove_layers();

	d_inside_group = true;
}


void
GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup::end_add_or_remove_layers()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_inside_group,
			GPLATES_ASSERTION_SOURCE);

	d_reconstruct_graph.emit_end_add_or_remove_layers();

	d_inside_group = false;
}
