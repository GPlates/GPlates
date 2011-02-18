/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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
#include <QThread>

#include "ApplicationState.h"

#include "AppLogicUtils.h"
#include "FeatureCollectionFileIO.h"
#include "Layer.h"
#include "LayerTask.h"
#include "LayerTaskRegistry.h"
#include "ReconstructGraph.h"
#include "ReconstructUtils.h"
#include "SessionManagement.h"
#include "UserPreferences.h"

#include "api/PythonExecutionThread.h"
#include "api/PythonInterpreterLocker.h"
#include "api/PythonRunner.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace
{
	bool
	has_reconstruction_time_changed(
			const double &old_reconstruction_time,
			const double &new_reconstruction_time)
	{
		// != does not work with doubles, so we must wrap them in Real.
		return GPlatesMaths::Real(old_reconstruction_time)
				!= GPlatesMaths::Real(new_reconstruction_time);
	}


	bool
	has_anchor_plate_id_changed(
			GPlatesModel::integer_plate_id_type old_anchor_plate_id,
			GPlatesModel::integer_plate_id_type new_anchor_plate_id)
	{
		return old_anchor_plate_id != new_anchor_plate_id;
	}


	/**
	 * If the layer is a velocity field calculator then look for any topology resolver layers
	 * and connect their outputs to the velocity layer input.
	 */
	void
	connect_input_channels_for_velocity_field_calculator_layer(
			GPlatesAppLogic::Layer& velocity_layer,
			const GPlatesAppLogic::ReconstructGraph &graph)
	{
		GPlatesAppLogic::ReconstructGraph::const_iterator it = graph.begin();
		GPlatesAppLogic::ReconstructGraph::const_iterator it_end = graph.end();
		for(; it != it_end; it++)
		{
			if (it->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER &&
			   it->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
			{
				continue;
			}
			std::vector<GPlatesAppLogic::Layer::input_channel_definition_type> input_channel_definitions = 
				velocity_layer.get_input_channel_definitions();
		
			QString velocity_input_channel_name;
			GPlatesAppLogic::Layer::LayerInputDataType velocity_input_type =
					GPlatesAppLogic::Layer::INPUT_FEATURE_COLLECTION_DATA;
			// It needs to be initialised otherwise g++ complains.
			
			BOOST_FOREACH(
					boost::tie(velocity_input_channel_name, velocity_input_type, boost::tuples::ignore),
					input_channel_definitions)
			{
				// FIXME: Find a better way to do this - there could be several input channels
				// that accept reconstructed geometry input - we really need the channel name which
				// only the layer task itself should know - this auto-connection should probably
				// be done by the layer task.
				if (velocity_input_type == GPlatesAppLogic::Layer::INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA)
				{
					velocity_layer.connect_input_to_layer_output(*it, velocity_input_channel_name);
					break;
				}
			}
		}
	}


	/**
	 * If the layer is a topology resolver then look for any velocity field calculator layers
	 * and connect the topology resolver output to the input of the velocity layers.
	 */
	void
	connect_input_channels_for_topology_resolver_layer(
			const GPlatesAppLogic::Layer& topology_layer,
			GPlatesAppLogic::ReconstructGraph &graph)
	{
		GPlatesAppLogic::ReconstructGraph::iterator it = graph.begin();
		GPlatesAppLogic::ReconstructGraph::iterator it_end = graph.end();
		for(; it != it_end; it++)
		{
			if (it->get_type() != GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR)
			{
				continue;
			}
			GPlatesAppLogic::Layer velocity_layer = *it;

			std::vector<GPlatesAppLogic::Layer::input_channel_definition_type> input_channel_definitions = 
				velocity_layer.get_input_channel_definitions();
		
			QString velocity_input_channel_name;
			GPlatesAppLogic::Layer::LayerInputDataType velocity_input_type =
					GPlatesAppLogic::Layer::INPUT_FEATURE_COLLECTION_DATA;
			// It needs to be initialised otherwise g++ complains.
			
			BOOST_FOREACH(
					boost::tie(velocity_input_channel_name, velocity_input_type, boost::tuples::ignore),
					input_channel_definitions)
			{
				// FIXME: Find a better way to do this - there could be several input channels
				// that accept reconstructed geometry input - we really need the channel name which
				// only the layer task itself should know - this auto-connection should probably
				// be done by the layer task.
				if (velocity_input_type == GPlatesAppLogic::Layer::INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA)
				{
					velocity_layer.connect_input_to_layer_output(topology_layer, velocity_input_channel_name);
					break;
				}
			}
		}
	}


	/**
	 * Returns true if @a layer_task_type matches any layers types in @a layer_list.
	 */
	bool
	is_the_layer_task_in_the_layer_list(
			GPlatesAppLogic::LayerTaskType::Type layer_task_type,
			const std::list<GPlatesAppLogic::Layer>& layer_list)
	{
		BOOST_FOREACH(GPlatesAppLogic::Layer layer, layer_list)
		{
			if (!layer.is_valid())
			{
				continue;
			}

			if (layer_task_type == layer.get_type())
			{
				return true;
			}
		}
		return false;
	}
}


GPlatesAppLogic::ApplicationState::ApplicationState() :
	d_feature_collection_file_state(
			new FeatureCollectionFileState(d_model)),
	d_feature_collection_file_io(
			new FeatureCollectionFileIO(
					d_model, *d_feature_collection_file_state)),
	d_session_management_ptr(new SessionManagement(*this)),
	d_user_preferences_ptr(new UserPreferences()),
	d_layer_task_registry(new LayerTaskRegistry()),
	d_reconstruct_graph(new ReconstructGraph(*this)),
	d_update_default_reconstruction_tree_layer(true),
	d_reconstruction_time(0.0),
	d_anchored_plate_id(0),
	d_reconstruction(
			// Empty reconstruction
			Reconstruction::create(d_reconstruction_time, 0/*anchored_plate_id*/)),
	d_python_runner(NULL),
	d_python_execution_thread(NULL)
{
	// Register default layer task types with the layer task registry.
	register_default_layer_task_types(*d_layer_task_registry);

	mediate_signal_slot_connections();

#if !defined(GPLATES_NO_PYTHON)
	using namespace boost::python;

	// Hold references to the main module and its namespace for easy access from
	// all parts of GPlates.
	GPlatesApi::PythonInterpreterLocker interpreter_locker;
	try
	{
		d_python_main_module = import("__main__");
		d_python_main_namespace = d_python_main_module.attr("__dict__");
	}
	catch (const error_already_set &)
	{
		PyErr_Print();
	}
#endif

	// These two must be set up after d_python_main_module and
	// d_python_main_namespace have been set.
	d_python_runner = new GPlatesApi::PythonRunner(*this, this);
	d_python_execution_thread = new GPlatesApi::PythonExecutionThread(*this, this);
	d_python_execution_thread->start(QThread::IdlePriority);
}


GPlatesAppLogic::ApplicationState::~ApplicationState()
{
	// Need destructor defined in ".cc" file because boost::scoped_ptr destructor
	// needs complete type.

	// Disconnect from the file state remove file signal because we delegate to ReconstructGraph
	// which is one of our data members and if we don't disconnect then it's possible
	// that we'll delegate to an already destroyed ReconstructGraph as our other
	// data member, FeatureCollectionFileState, is being destroyed.
	QObject::disconnect(
			&get_feature_collection_file_state(),
			SIGNAL(file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
			this,
			SLOT(handle_file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)));

	// Stop the Python execution thread.
	static const int WAIT_TIME = 1000 /* milliseconds */;
	d_python_execution_thread->quit_event_loop();
	d_python_execution_thread->wait(WAIT_TIME);
	d_python_execution_thread->terminate();
}


void
GPlatesAppLogic::ApplicationState::set_reconstruction_time(
		const double &new_reconstruction_time)
{
	if (!has_reconstruction_time_changed(d_reconstruction_time, new_reconstruction_time))
	{
		return;
	}

	d_reconstruction_time = new_reconstruction_time;
	reconstruct();

	emit reconstruction_time_changed(*this, d_reconstruction_time);
}


void
GPlatesAppLogic::ApplicationState::set_anchored_plate_id(
		GPlatesModel::integer_plate_id_type new_anchor_plate_id)
{
	if (!has_anchor_plate_id_changed(d_anchored_plate_id, new_anchor_plate_id))
	{
		return;
	}

	d_anchored_plate_id = new_anchor_plate_id;
	reconstruct();

	emit anchor_plate_id_changed(*this, d_anchored_plate_id);
}


void
GPlatesAppLogic::ApplicationState::reconstruct()
{
	// Get each layer to perform its reconstruction processing and
	// dump its output results into an aggregate Reconstruction object.
	d_reconstruction = d_reconstruct_graph->execute_layer_tasks(d_reconstruction_time);

	emit reconstructed(*this);
}


void
GPlatesAppLogic::ApplicationState::handle_file_state_files_added(
		FeatureCollectionFileState &file_state,
		const std::vector<FeatureCollectionFileState::file_reference> &new_files)
{
	// Pass the signal onto the reconstruct graph first.
	// We do this rather than connect it to the signal directly so we can control
	// the order in which things happen.
	// In this case we want the reconstruct graph to know about the new files first
	// so that we can then get new file objects from it.
	d_reconstruct_graph->handle_file_state_files_added(file_state, new_files);

	// Create new layers for the new files.
	BOOST_FOREACH(FeatureCollectionFileState::file_reference new_file, new_files)
	{
		// Create a new layer for the current file (or create multiple layers if the
		// feature collection contains features that can be processed by more than one layer type).
		// We ignore the created layers because they've been added to the reconstruct graph
		// and because they will automatically get removed/destroyed their associated
		// input files (the ones that triggered this auto-layer-creation) have been unloaded.
		create_layers(new_file);
	}

	// New layers have been added so we need to reconstruct.
	reconstruct();
}


void
GPlatesAppLogic::ApplicationState::handle_file_state_file_about_to_be_removed(
		FeatureCollectionFileState &file_state,
		FeatureCollectionFileState::file_reference file_about_to_be_removed)
{
	// Destroy auto-created layers for the file about to be removed.
	// NOTE: If the user explicitly created a layer then it will never get removed automatically -
	// the user must also explicitly destroy the layer - this is even the case when all files
	// connected to that layer are unloaded (the user still has to explicitly destroy the layer).
	const file_to_primary_layers_mapping_type::iterator remove_layers_iter =
			d_file_to_primary_layers_mapping.find(file_about_to_be_removed);
	if (remove_layers_iter != d_file_to_primary_layers_mapping.end())
	{
		const layer_seq_type &layers_to_remove = remove_layers_iter->second;

		// The current default reconstruction tree layer (if any).
		const Layer current_default_reconstruction_tree_layer =
				d_reconstruct_graph->get_default_reconstruction_tree_layer();

		// Remove all layers auto-created for the file about to be removed.
		BOOST_FOREACH(const Layer &layer_to_remove, layers_to_remove)
		{
			// FIXME: We need to do something about the ambiguity of changing the default
			// reconstruction tree layer under the nose of the user.
			// This shouldn't really be done here and it's dodgy because we're assuming no one
			// else is setting the default reconstruction tree layer.
#if 1
			//
			// Before removing the layer see if it's the current default reconstruction tree layer.
			if (layer_to_remove == current_default_reconstruction_tree_layer)
			{
				// If there are any recently set defaults.
				if (!d_default_reconstruction_tree_layer_stack.empty())
				{
					// The current default should match the top of the stack unless
					// someone else is setting the default (FIXME: this is quite likely).
					if (current_default_reconstruction_tree_layer ==
						d_default_reconstruction_tree_layer_stack.top())
					{
						// Remove the current default from the top of the stack.
						d_default_reconstruction_tree_layer_stack.pop();
					}

					// Remove any invalid layers from the stack (these are the result of
					// removed layers caused by unloaded rotation files).
					while (!d_default_reconstruction_tree_layer_stack.empty() &&
						!d_default_reconstruction_tree_layer_stack.top().is_valid())
					{
						d_default_reconstruction_tree_layer_stack.pop();
					}

					// Get the most recently set (and valid) default.
					if (!d_default_reconstruction_tree_layer_stack.empty())
					{
						const Layer new_default_reconstruction_tree_layer =
								d_default_reconstruction_tree_layer_stack.top();

						d_reconstruct_graph->set_default_reconstruction_tree_layer(
								new_default_reconstruction_tree_layer);
					}
				}
			}
#endif

			// Remove the auto-created layer.
			if (layer_to_remove.is_valid())
			{
				d_reconstruct_graph->remove_layer(layer_to_remove);
			}
		}

		// We don't need to track the auto-generated layers for the file being removed.
		d_file_to_primary_layers_mapping.erase(remove_layers_iter);
	}

	// Pass the signal onto the reconstruct graph first.
	// We do this rather than connect it to the signal directly so we can control
	// the order in which things happen.
	d_reconstruct_graph->handle_file_state_file_about_to_be_removed(file_state, file_about_to_be_removed);

	// An input file has been removed so reconstruct in case it was connected to a layer
	// which is probably going to always be the case unless the user deletes a layer without
	// unloading the file it uses.
	reconstruct();
}


const GPlatesAppLogic::Reconstruction &
GPlatesAppLogic::ApplicationState::get_current_reconstruction() const
{
	return *d_reconstruction;
}


GPlatesAppLogic::FeatureCollectionFileState &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_state()
{
	return *d_feature_collection_file_state;
}


GPlatesAppLogic::FeatureCollectionFileIO &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_io()
{
	return *d_feature_collection_file_io;
}


GPlatesAppLogic::SessionManagement &
GPlatesAppLogic::ApplicationState::get_session_management()
{
	return *d_session_management_ptr;
}


GPlatesAppLogic::UserPreferences &
GPlatesAppLogic::ApplicationState::get_user_preferences()
{
	return *d_user_preferences_ptr;
}


GPlatesAppLogic::LayerTaskRegistry &
GPlatesAppLogic::ApplicationState::get_layer_task_registry()
{
	return *d_layer_task_registry;
}


GPlatesAppLogic::ReconstructGraph &
GPlatesAppLogic::ApplicationState::get_reconstruct_graph()
{
	return *d_reconstruct_graph;
}


void
GPlatesAppLogic::ApplicationState::mediate_signal_slot_connections()
{
	//
	// Connect to FeatureCollectionFileState signals.
	//
	QObject::connect(
			&get_feature_collection_file_state(),
			SIGNAL(file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)),
			this,
			SLOT(handle_file_state_files_added(
					GPlatesAppLogic::FeatureCollectionFileState &,
					const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &)));
	QObject::connect(
			&get_feature_collection_file_state(),
			SIGNAL(file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
			this,
			SLOT(handle_file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)));

	//
	// Perform a new reconstruction whenever shapefile attributes are modified.
	//
	// FIXME: This should be handled by listening for model modification events on the
	// feature collections of currently loaded files (since remapping shapefile attributes
	// modifies the model).
	//
	QObject::connect(
			&get_feature_collection_file_io(),
			SIGNAL(remapped_shapefile_attributes(
					GPlatesAppLogic::FeatureCollectionFileIO &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
			this,
			SLOT(reconstruct()));

	//
	// Perform a new reconstruction whenever layers are modified.
	//
	QObject::connect(
			d_reconstruct_graph.get(),
			SIGNAL(layer_added_input_connection(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::Layer::InputConnection)),
			this,
			SLOT(reconstruct()));
	QObject::connect(
			d_reconstruct_graph.get(),
			SIGNAL(layer_removed_input_connection(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(reconstruct()));
	QObject::connect(
			d_reconstruct_graph.get(),
			SIGNAL(layer_activation_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					bool)),
			this,
			SLOT(reconstruct()));
	QObject::connect(
			d_reconstruct_graph.get(),
			SIGNAL(default_reconstruction_tree_layer_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(reconstruct()));
}


std::vector< boost::shared_ptr<GPlatesAppLogic::LayerTask> >
GPlatesAppLogic::ApplicationState::create_layer_tasks(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &input_feature_collection)
{
	// Look for layer task types that can process the feature collection.
	const std::vector<LayerTaskRegistry::LayerTaskType> layer_task_types =
			d_layer_task_registry->get_layer_task_types_that_can_process_feature_collection(
					input_feature_collection);

	// The sequence of layer tasks to return to the caller.
	std::vector< boost::shared_ptr<LayerTask> > layer_tasks;
	layer_tasks.reserve(layer_task_types.size());

	// Iterate over the compatible layer task types and create layer tasks.
	BOOST_FOREACH(LayerTaskRegistry::LayerTaskType layer_task_type, layer_task_types)
	{
		const boost::optional<boost::shared_ptr<GPlatesAppLogic::LayerTask> > layer_task =
				create_primary_layer_task(layer_task_type);
		if (layer_task)
		{
			// Add to sequence returned to the caller.
			layer_tasks.push_back(*layer_task);
		}
	}

	return layer_tasks;
}


boost::optional<boost::shared_ptr<GPlatesAppLogic::LayerTask> >
GPlatesAppLogic::ApplicationState::create_primary_layer_task(
		LayerTaskRegistry::LayerTaskType& layer_task_type) 
{
	// Ignore layer task types that are not primary.
	// Primary task types are the set of orthogonal task types that we can
	// create without user interaction. The other types can be selected specifically
	// by the user but will never be created automatically when a file is first loaded.
	if (!layer_task_type.is_primary_task_type())
	{
		return boost::none;
	}

	// Create the layer task.
	return layer_task_type.create_layer_task();
}

void
GPlatesAppLogic::ApplicationState::update_layers(
		const FeatureCollectionFileState::file_reference &file_ref)
{
	const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file_ref.get_file().get_feature_collection();

	// The file may have changed so find out what layer types can process it.
	// This may have changed since we last checked.
	const std::vector<LayerTaskRegistry::LayerTaskType> new_layer_task_types =
			d_layer_task_registry->get_layer_task_types_that_can_process_feature_collection(
					feature_collection);

	bool created_new_layers = false;

	BOOST_FOREACH(LayerTaskRegistry::LayerTaskType layer_task_type, new_layer_task_types)
	{
		// Ignore layer task types that are not primary.
		// Primary task types are the set of orthogonal task types that we can
		// create without user interaction. The other types can be selected specifically
		// by the user but will never be created automatically when a file is first loaded.
		if (!layer_task_type.is_primary_task_type())
		{
			continue;
		}

		// Get the layers auto-created from 'file_ref'.
		const layer_seq_type &layers_created_from_file =
				d_file_to_primary_layers_mapping[file_ref];

		// If a layer task of the current type hasn't yet been created for 'file_ref'
		// then create one.
		if (!is_the_layer_task_in_the_layer_list(
				layer_task_type.get_layer_type(),
				layers_created_from_file))
		{
			const boost::shared_ptr<LayerTask> layer_task = layer_task_type.create_layer_task();

			create_layer(file_ref, layer_task);

			created_new_layers = true;
		}
	}

	if (created_new_layers)
	{
		reconstruct();
	}
}

void
GPlatesAppLogic::ApplicationState::create_layer(
		const FeatureCollectionFileState::file_reference &input_file_ref,
		const boost::shared_ptr<LayerTask> &layer_task)
{
	// Create a new layer using the layer task.
	// This will emit a signal in ReconstructGraph to notify clients of a new layer.
	Layer new_layer = d_reconstruct_graph->add_layer(layer_task);

	//
	// Connect the feature collection to the input of the new layer.
	//

	// Get the main feature collection input channel for our layer.
	const QString main_input_feature_collection_channel =
			new_layer.get_main_input_feature_collection_channel();

	// Get an input file object from the reconstruct graph.
	const Layer::InputFile input_file = d_reconstruct_graph->get_input_file(input_file_ref);
	// Connect the input file to the main input channel.
	new_layer.connect_input_to_file(
			input_file,
			main_input_feature_collection_channel);

	// If the layer is a velocity field calculator then look for any topology resolver layers
	// and connect their outputs to the velocity layer input.
	if (new_layer.get_type() == GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR)
	{
		connect_input_channels_for_velocity_field_calculator_layer(
				new_layer, 
				*d_reconstruct_graph);
	}

	// If the layer is a topology resolver then look for any velocity field calculator layers
	// and connect the topology resolver output to the input of the velocity layers.
	if (new_layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER ||
		new_layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
	{
		connect_input_channels_for_topology_resolver_layer(
				new_layer, 
				*d_reconstruct_graph);
	}

	// FIXME: We need to do something about the ambiguity of changing the default
	// reconstruction tree layer under the nose of the user.
	// This shouldn't really be done here and it's dodgy because we're assuming no one
	// else is setting the default reconstruction tree layer.
	if (new_layer.get_type() == GPlatesAppLogic::LayerTaskType::RECONSTRUCTION)
	{
		if (d_update_default_reconstruction_tree_layer)
		{
			d_reconstruct_graph->set_default_reconstruction_tree_layer(new_layer);
		}

		// Keep track of the default reconstruction tree layers set.
		d_default_reconstruction_tree_layer_stack.push(new_layer);
	}

	// Keep track of the layers auto-created for each file loaded.
	d_file_to_primary_layers_mapping[input_file_ref].push_back(new_layer);
}

void
GPlatesAppLogic::ApplicationState::create_layers(
		const FeatureCollectionFileState::file_reference &input_file_ref)
{
	const GPlatesModel::FeatureCollectionHandle::weak_ref new_feature_collection =
			input_file_ref.get_file().get_feature_collection();

	// Create the layer tasks that can processes the feature collection in the input file.
	const std::vector< boost::shared_ptr<LayerTask> > layer_tasks =
			create_layer_tasks(new_feature_collection);

	BOOST_FOREACH(const boost::shared_ptr<LayerTask> &layer_task, layer_tasks)
	{
		create_layer(input_file_ref, layer_task);
	}
}

