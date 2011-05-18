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
#include "ReconstructMethodRegistry.h"
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
	 * Connect the output of one layer to the input of another.
	 */
	void
	connect_layer_input_to_layer_output(
			GPlatesAppLogic::Layer& layer_receiving_input,
			const GPlatesAppLogic::Layer& layer_giving_output)
	{
		const std::vector<GPlatesAppLogic::LayerInputChannelType> input_channel_types = 
				layer_receiving_input.get_input_channel_types();

		BOOST_FOREACH(
				const GPlatesAppLogic::LayerInputChannelType &input_channel_type,
				input_channel_types)
		{
			const boost::optional< std::vector<GPlatesAppLogic::LayerTaskType::Type> > &
					supported_input_data_types =
							input_channel_type.get_layer_input_data_types();
			// If the current layer type (of layer giving output) matches the supported input types...
			if (supported_input_data_types &&
				std::find(
					supported_input_data_types->begin(),
					supported_input_data_types->end(),
					layer_giving_output.get_type()) != supported_input_data_types->end())
			{
				layer_receiving_input.connect_input_to_layer_output(
						layer_giving_output,
						input_channel_type.get_input_channel_name());
				break;
			}
		}
	}


	/**
	 * The layer is a velocity field calculator layer so look for any topology resolver layers
	 * and connect their outputs to the velocity layer input.
	 */
	template <typename LayerForwardIter>
	void
	connect_velocity_field_calculator_layer_input_to_topology_resolver_layer_outputs(
			GPlatesAppLogic::Layer& velocity_layer,
			LayerForwardIter layers_begin,
			LayerForwardIter layers_end)
	{
		for (LayerForwardIter layer_iter = layers_begin ; layer_iter != layers_end; layer_iter++)
		{
			if (layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER &&
			   layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
			{
				continue;
			}
			GPlatesAppLogic::Layer topology_layer = *layer_iter;

			connect_layer_input_to_layer_output(velocity_layer, topology_layer);
		}
	}


	/**
	 * The layer is a topology resolver layer so look for any velocity field calculator layers
	 * and connect the topology resolver output to the input of the velocity layers.
	 */
	template <typename LayerForwardIter>
	void
	connect_topology_resolver_layer_output_to_velocity_field_calculator_layer_inputs(
			const GPlatesAppLogic::Layer& topology_layer,
			LayerForwardIter layers_begin,
			LayerForwardIter layers_end)
	{
		for (LayerForwardIter layer_iter = layers_begin ; layer_iter != layers_end; layer_iter++)
		{
			if (layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR)
			{
				continue;
			}
			GPlatesAppLogic::Layer velocity_layer = *layer_iter;

			connect_layer_input_to_layer_output(velocity_layer, topology_layer);
		}
	}


	/**
	 * The layer is a reconstruct layer so look for any topology layers
	 * and connect the reconstruct layer output to the input of the topology layers.
	 */
	template <typename LayerForwardIter>
	void
	connect_reconstruct_layer_output_to_topology_resolver_layer_inputs(
			const GPlatesAppLogic::Layer& reconstruct_layer,
			LayerForwardIter layers_begin,
			LayerForwardIter layers_end)
	{
		for (LayerForwardIter layer_iter = layers_begin ; layer_iter != layers_end; layer_iter++)
		{
			if (layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER &&
				layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
			{
				continue;
			}
			GPlatesAppLogic::Layer topology_layer = *layer_iter;

			connect_layer_input_to_layer_output(topology_layer, reconstruct_layer);
		}
	}


	/**
	 * The layer is a topology layer so look for any reconstruct layers
	 * and connect the topology layer input to the reconstruct layer outputs.
	 */
	template <typename LayerForwardIter>
	void
	connect_topology_resolver_layer_input_to_reconstruct_layer_outputs(
			GPlatesAppLogic::Layer& topology_layer,
			LayerForwardIter layers_begin,
			LayerForwardIter layers_end)
	{
		for (LayerForwardIter layer_iter = layers_begin ; layer_iter != layers_end; layer_iter++)
		{
			if (layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
			{
				continue;
			}
			GPlatesAppLogic::Layer reconstruct_layer = *layer_iter;

			connect_layer_input_to_layer_output(topology_layer, reconstruct_layer);
		}
	}


	/**
	 * If there are any new reconstruct layers or any new topology layers then connect them up.
	 */
	template <typename OldLayersForwardIter, typename NewLayersForwardIter>
	void
	connect_reconstruct_layer_outputs_to_topology_resolver_layer_inputs(
			OldLayersForwardIter old_layers_begin,
			OldLayersForwardIter old_layers_end,
			NewLayersForwardIter new_layers_begin,
			NewLayersForwardIter new_layers_end)
	{
		// If there's a *new* reconstruct layer then connect it to any *old* topology layer inputs.
		for (NewLayersForwardIter new_layer_iter = new_layers_begin;
			new_layer_iter != new_layers_end;
			new_layer_iter++)
		{
			if (new_layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
			{
				continue;
			}
			GPlatesAppLogic::Layer new_reconstruct_layer = *new_layer_iter;

			connect_reconstruct_layer_output_to_topology_resolver_layer_inputs(
					new_reconstruct_layer, old_layers_begin, old_layers_end);
		}

		// If there's a *new* topology layer then connect it to any *old* reconstruct layer outputs.
		for (NewLayersForwardIter new_layer_iter = new_layers_begin;
			new_layer_iter != new_layers_end;
			new_layer_iter++)
		{
			if (new_layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER &&
				new_layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
			{
				continue;
			}
			GPlatesAppLogic::Layer new_topology_layer = *new_layer_iter;

			connect_topology_resolver_layer_input_to_reconstruct_layer_outputs(
					new_topology_layer, old_layers_begin, old_layers_end);
		}

		// If there's a *new* reconstruct layer then connect it to any *new* topology layer inputs.
		// Note that this also takes care of any *new* topology layer needing connection to any
		// *new* reconstruct layer outputs.
		for (NewLayersForwardIter new_layer_iter = new_layers_begin;
			new_layer_iter != new_layers_end;
			new_layer_iter++)
		{
			if (new_layer_iter->get_type() != GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
			{
				continue;
			}
			GPlatesAppLogic::Layer new_reconstruct_layer = *new_layer_iter;

			connect_reconstruct_layer_output_to_topology_resolver_layer_inputs(
					new_reconstruct_layer, new_layers_begin, new_layers_end);
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
	d_reconstruct_method_registry(new ReconstructMethodRegistry()),
	d_layer_task_registry(new LayerTaskRegistry()),
	d_reconstruct_graph(new ReconstructGraph(*this)),
	d_update_default_reconstruction_tree_layer(true),
	d_reconstruction_time(0.0),
	d_anchored_plate_id(0),
	d_reconstruction(
			// Empty reconstruction
			Reconstruction::create(d_reconstruction_time)),
	d_scoped_reconstruct_nesting_count(0),
	d_reconstruct_on_scope_exit(false),
	d_python_runner(NULL),
	d_python_execution_thread(NULL)
{
	// Register default reconstruct method types with the reconstruct method registry.
	register_default_reconstruct_method_types(*d_reconstruct_method_registry);

	// Register default layer task types with the layer task registry.
	register_default_layer_task_types(*d_layer_task_registry, *this);

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
	// If the count is non-zero then we've been requested to block calls to 'reconstruct'.
	// NOTE: We also want to block emitting the 'reconstructed' signal because the visual
	// layers listen to this signal and update the visual rendering of the layers which, due to the
	// layer proxy system, results in the layer proxies being requested to do reconstructions.
	if (d_scoped_reconstruct_nesting_count > 0)
	{
		// Call 'reconstruct' later when all scopes have exited.
		d_reconstruct_on_scope_exit = true;
		return;
	}

	// Get each layer to update itself in response to any changes that caused this
	// 'reconstruct' method to get called.
	d_reconstruction = d_reconstruct_graph->update_layer_tasks(d_reconstruction_time);

	emit reconstructed(*this);
}


void
GPlatesAppLogic::ApplicationState::handle_file_state_files_added(
		FeatureCollectionFileState &file_state,
		const std::vector<FeatureCollectionFileState::file_reference> &new_files)
{
	// New layers are added in this method so block any calls to 'reconstruct' until we exit
	// this method at which point we call 'reconstruct'.
	// Blocking of calls to 'reconstruct' during this scope prevent multiple calls
	// 'reconstruct' caused by layer signals, which is unnecessary if we're going to call it anyway.
	ScopedReconstructGuard scoped_reconstruct_guard(*this, true/*reconstruct_on_scope_exit*/);

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
}


void
GPlatesAppLogic::ApplicationState::handle_file_state_file_about_to_be_removed(
		FeatureCollectionFileState &file_state,
		FeatureCollectionFileState::file_reference file_about_to_be_removed)
{
	// An input file will be removed in this method so call 'reconstruct' at the end in case
	// the input file is connected to a layer which is probably going to always be
	// the case unless the user deletes a layer without unloading the file it uses.
	// Blocking of calls to 'reconstruct' during this scope prevent multiple calls
	// 'reconstruct' caused by layer signals, which is unnecessary if we're going to call it anyway.
	ScopedReconstructGuard scoped_reconstruct_guard(*this, true/*reconstruct_on_scope_exit*/);

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


GPlatesAppLogic::ReconstructMethodRegistry &
GPlatesAppLogic::ApplicationState::get_reconstruct_method_registry()
{
	return *d_reconstruct_method_registry;
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


void
GPlatesAppLogic::ApplicationState::begin_reconstruct_on_scope_exit()
{
	++d_scoped_reconstruct_nesting_count;
}


void
GPlatesAppLogic::ApplicationState::end_reconstruct_on_scope_exit(
		bool reconstruct_on_scope_exit)
{
	--d_scoped_reconstruct_nesting_count;

	// If *any* ScopedReconstructGuard objects specify that @a reconstruct should be called
	// then it will get called when the last object goes out of scope.
	if (reconstruct_on_scope_exit)
	{
		d_reconstruct_on_scope_exit = true;
	}

	// If the count is zero then we've exited all scopes and can now call 'reconstruct' if
	// any ScopedReconstructGuard objects requested it.
	if (d_scoped_reconstruct_nesting_count == 0 && d_reconstruct_on_scope_exit)
	{
		d_reconstruct_on_scope_exit = false;

		reconstruct();
	}
}


std::vector< boost::shared_ptr<GPlatesAppLogic::LayerTask> >
GPlatesAppLogic::ApplicationState::create_layer_tasks(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &input_feature_collection)
{
	// Look for layer task types that we should create to process the loaded feature collection.
	const std::vector<LayerTaskRegistry::LayerTaskType> layer_task_types =
			d_layer_task_registry->get_layer_task_types_to_auto_create_for_loaded_file(
					input_feature_collection);

	// The sequence of layer tasks to return to the caller.
	std::vector< boost::shared_ptr<LayerTask> > layer_tasks;
	layer_tasks.reserve(layer_task_types.size());

	// Iterate over the compatible layer task types and create layer tasks.
	BOOST_FOREACH(LayerTaskRegistry::LayerTaskType layer_task_type, layer_task_types)
	{
		const boost::optional<boost::shared_ptr<GPlatesAppLogic::LayerTask> > layer_task =
				layer_task_type.create_layer_task();
		if (layer_task)
		{
			// Add to sequence returned to the caller.
			layer_tasks.push_back(*layer_task);
		}
	}

	return layer_tasks;
}


void
GPlatesAppLogic::ApplicationState::update_layers(
		const FeatureCollectionFileState::file_reference &file_ref)
{
	// If we need to call 'reconstruct' then do it through a scoped object - that way
	// it can get delayed if there's an enclosing scoped object (higher in the call chain).
	ScopedReconstructGuard scoped_reconstruct_guard(*this);

	const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file_ref.get_file().get_feature_collection();

	// The file may have changed so find out what layer types should be created to
	// process the file.
	// This may have changed since we last checked.
	const std::vector<LayerTaskRegistry::LayerTaskType> new_layer_task_types =
			d_layer_task_registry->get_layer_task_types_to_auto_create_for_loaded_file(
					feature_collection);

	// Copy the old layers for the current file - for later.
	const std::vector<Layer> old_layers(
			d_file_to_primary_layers_mapping[file_ref].begin(),
			d_file_to_primary_layers_mapping[file_ref].end());
	std::vector<Layer> new_layers;
	BOOST_FOREACH(LayerTaskRegistry::LayerTaskType layer_task_type, new_layer_task_types)
	{
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

			const Layer new_layer = create_layer(file_ref, layer_task);
			new_layers.push_back(new_layer);
		}
	}

	// If there is more than one layer created for the same input file then we might need
	// to connect the layers to each other depending on the types of layers.
	// Note that we only connect up a new layer to an old layer or vice versa (or new to new)
	// but never an old layer to an old layer as that should have already been done.
	connect_reconstruct_layer_outputs_to_topology_resolver_layer_inputs(
			old_layers.begin(), old_layers.end(),
			new_layers.begin(), new_layers.end());

	if (!new_layers.empty())
	{
		// We want to call 'reconstruct' (when all scopes exit).
		scoped_reconstruct_guard.call_reconstruct_on_scope_exit();
	}
}


GPlatesAppLogic::Layer
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
		connect_velocity_field_calculator_layer_input_to_topology_resolver_layer_outputs(
				new_layer,
				d_reconstruct_graph->begin(),
				d_reconstruct_graph->end());
	}

	// If the layer is a topology resolver then look for any velocity field calculator layers
	// and connect the topology resolver output to the input of the velocity layers.
	if (new_layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER ||
		new_layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
	{
		connect_topology_resolver_layer_output_to_velocity_field_calculator_layer_inputs(
				new_layer, 
				d_reconstruct_graph->begin(),
				d_reconstruct_graph->end());
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

	return new_layer;
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

	// Create a new layer for each layer task.
	std::vector<Layer> new_layers;
	BOOST_FOREACH(const boost::shared_ptr<LayerTask> &layer_task, layer_tasks)
	{
		const Layer new_layer = create_layer(input_file_ref, layer_task);
		new_layers.push_back(new_layer);
	}

	// If there is more than one layer created for the same input file then we might need
	// to connect the layers to each other depending on the types of layers.
	// NOTE: Since we have just created layers for this file for the first time the
	// total number of layers created so far is the same as the number of new layers just created.
	const std::vector<Layer> old_layers; // Empty
	connect_reconstruct_layer_outputs_to_topology_resolver_layer_inputs(
			old_layers.begin(), old_layers.end(),
			new_layers.begin(), new_layers.end());
}
