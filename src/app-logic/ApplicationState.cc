/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ApplicationState.h"

#include "AppLogicUtils.h"
#include "FeatureCollectionFileIO.h"
#include "Layer.h"
#include "LayerTaskRegistry.h"
#include "LayerTaskTypes.h"
#include "ReconstructGraph.h"
#include "ReconstructUtils.h"

#include "global/AssertionFailureException.h"
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
}


GPlatesAppLogic::ApplicationState::ApplicationState() :
	d_feature_collection_file_state(
			new FeatureCollectionFileState(d_model)),
	d_feature_collection_file_io(
			new FeatureCollectionFileIO(
					d_model, *d_feature_collection_file_state)),
	d_layer_task_registry(new LayerTaskRegistry()),
	d_reconstruct_graph(new ReconstructGraph(*this)),
	d_block_handle_file_state_file_activation_changed(false),
	d_reconstruction_time(0.0),
	d_anchored_plate_id(0),
	d_reconstruction(
			// Empty reconstruction
			Reconstruction::create(d_reconstruction_time, 0/*anchored_plate_id*/))
{
	// Register all layer task types with the layer task registry.
	LayerTaskTypes::register_layer_task_types(*d_layer_task_registry, *this);

	mediate_signal_slot_connections();
}


GPlatesAppLogic::ApplicationState::~ApplicationState()
{
	// Need destructor defined in ".cc" file because boost::scoped_ptr destructor
	// needs complete type.

	// Disconnect from the file state remove file signal because we delegate to ReconstructGraph
	// which is one of our data members and if we don't disconnect then it's possible
	// that we'll delegate to an already destroyed ReconstructGraph as our other
	// data member, FeatureCollectionFileState, is being destroyed.
	//
	// Also disconnect from the file activation signal as it can get emitted when a
	// file is removed.
	QObject::disconnect(
			&get_feature_collection_file_state(),
			SIGNAL(file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
			this,
			SLOT(handle_file_state_file_about_to_be_removed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference)));
	QObject::disconnect(
			&get_feature_collection_file_state(),
			SIGNAL(file_state_file_activation_changed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference,
					bool)),
			this,
			SLOT(handle_file_state_file_activation_changed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference,
					bool)));
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
		unsigned long new_anchor_plate_id)
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
		// and because they will automatically get removed/destroyed when all input files
		// on their main input channels have been unloaded.
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
	// Pass the signal onto the reconstruct graph first.
	// We do this rather than connect it to the signal directly so we can control
	// the order in which things happen.
	d_reconstruct_graph->handle_file_state_file_about_to_be_removed(file_state, file_about_to_be_removed);

	/*
	 * It's ugly cause it's gonna get removed soon.
	 *
	 * FIXME: This is temporary until file activation in
	 * ManageFeatureCollectionsDialog is removed and layer activation is provided
	 * in the layers GUI. We could keep both but it might be confusing for the user.
	 */
	{
		file_to_layers_mapping_type::iterator iter =
				d_file_to_layers_mapping.find(file_about_to_be_removed);
		if (iter != d_file_to_layers_mapping.end())
		{
			d_file_to_layers_mapping.erase(iter);
		}
	}

	// Currently we don't need to do anything else since the reconstruct graph will
	// remove any layers that have no input file connections on their main input channel.

	// An input file has been removed so reconstruct in case it was connected to a layer
	// which is probably going to always be the case unless the user deletes a layer without
	// unloading the file it uses.
	reconstruct();
}


void
GPlatesAppLogic::ApplicationState::handle_file_state_file_activation_changed(
		FeatureCollectionFileState &file_state,
		FeatureCollectionFileState::file_reference file,
		bool active)
{
	/*
	 * It's ugly cause it's gonna get removed soon.
	 *
	 * FIXME: This is temporary until file activation in
	 * ManageFeatureCollectionsDialog is removed and layer activation is provided
	 * in the layers GUI. We could keep both but it might be confusing for the user.
	 */
	{
		d_reconstruct_graph->get_input_file(file).activate(active);

		if (d_block_handle_file_state_file_activation_changed)
		{
			return;
		}

		if (active)
		{
			handle_setting_default_reconstruction_tree_layer(file);
		}

		// An input file has been removed so reconstruct in case it was connected to a layer
		// which is probably going to always be the case unless the user deletes a layer without
		// unloading the file it uses.
		reconstruct();
	}
}


void
GPlatesAppLogic::ApplicationState::handle_setting_default_reconstruction_tree_layer(
		FeatureCollectionFileState::file_reference file)
{
	d_block_handle_file_state_file_activation_changed = true;

	/*
	 * It's ugly cause this whole method is gonna get removed soon.
	 *
	 * FIXME: This is temporary until file activation in
	 * ManageFeatureCollectionsDialog is removed and layer activation is provided
	 * in the layers GUI. We could keep both but it might be confusing for the user.
	 */

	// If the file spawned a reconstruction tree layer then set the layer
	// as the default reconstruction tree layer.
	BOOST_FOREACH(const Layer &file_layer, d_file_to_layers_mapping[file])
	{
		if (file_layer.get_output_definition() == Layer::OUTPUT_RECONSTRUCTION_TREE_DATA)
		{
			d_reconstruct_graph->set_default_reconstruction_tree_layer(file_layer);

			// Search all other files (other than 'file') and if there are any reconstruction
			// tree layers then deactivate the file that spawned it - this is so GPlates
			// behaves like it used to.
			BOOST_FOREACH(file_to_layers_mapping_type::value_type &layers, d_file_to_layers_mapping)
			{
				if (layers.first == file)
				{
					continue;
				}

				BOOST_FOREACH(Layer &layer, layers.second)
				{
					if (layer.get_output_definition() == Layer::OUTPUT_RECONSTRUCTION_TREE_DATA)
					{
						FeatureCollectionFileState::file_reference other_file = layers.first;
						// This will emit the 'file_state_file_activation_changed' signal.
						other_file.set_file_active(false);
					}
				}
			}

			break;
		}
	}

	d_block_handle_file_state_file_activation_changed = false;
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
	QObject::connect(
			&get_feature_collection_file_state(),
			SIGNAL(file_state_file_activation_changed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference,
					bool)),
			this,
			SLOT(handle_file_state_file_activation_changed(
					GPlatesAppLogic::FeatureCollectionFileState &,
					GPlatesAppLogic::FeatureCollectionFileState::file_reference,
					bool)));

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
		// Ignore layer task types that are not primary.
		// Primary task types are the set of orthogonal task types that we can
		// create without user interaction. The other types can be selected specifically
		// by the user but will never be created automatically when a file is first loaded.
		if (!layer_task_type.is_primary_task_type())
		{
			continue;
		}

		// Create the layer task.
		const boost::shared_ptr<LayerTask> layer_task = layer_task_type.create_layer_task();

		// Add to sequence returned to the caller.
		layer_tasks.push_back(layer_task);
	}

	// There should be at least one primary layer task type that is a catch-all.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!layer_tasks.empty(),
			GPLATES_ASSERTION_SOURCE);

	return layer_tasks;
}


void
GPlatesAppLogic::ApplicationState::create_layers(
		const FeatureCollectionFileState::file_reference &input_file_ref)
{
	const GPlatesModel::FeatureCollectionHandle::weak_ref new_feature_collection =
			input_file_ref.get_file().get_feature_collection();

	// Get an input file object from the reconstruct graph.
	const Layer::InputFile input_file = d_reconstruct_graph->get_input_file(input_file_ref);

	// Create the layer tasks that can processes the feature collection in the input file.
	const std::vector< boost::shared_ptr<LayerTask> > layer_tasks =
			create_layer_tasks(new_feature_collection);

	BOOST_FOREACH(boost::shared_ptr<LayerTask> layer_task, layer_tasks)
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

		// Connect the input file to the main input channel.
		new_layer.connect_input_to_file(
				input_file,
				main_input_feature_collection_channel);

		/*
		 * It's ugly cause it's gonna get removed soon.
		 *
		 * FIXME: This is temporary until file activation in
		 * ManageFeatureCollectionsDialog is removed and layer activation is provided
		 * in the layers GUI. We could keep both but it might be confusing for the user.
		 */
		{
			d_file_to_layers_mapping[input_file_ref].push_back(new_layer);
		}
	}

	/*
	 * It's ugly cause it's gonna get removed soon.
	 *
	 * FIXME: This is temporary until file activation in
	 * ManageFeatureCollectionsDialog is removed and layer activation is provided
	 * in the layers GUI. We could keep both but it might be confusing for the user.
	 */
	{
		handle_setting_default_reconstruction_tree_layer(input_file_ref);
	}
}
