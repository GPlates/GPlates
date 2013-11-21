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
#include "LogModel.h"
#include "ReconstructGraph.h"
#include "ReconstructMethodRegistry.h"
#include "ReconstructUtils.h"
#include "Serialization.h"
#include "SessionManagement.h"
#include "UserPreferences.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"

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
	d_feature_collection_file_format_registry(
			new GPlatesFileIO::FeatureCollectionFileFormat::Registry()),
	d_feature_collection_file_io(
			new FeatureCollectionFileIO(
					d_model,
					*d_feature_collection_file_format_registry,
					*d_feature_collection_file_state)),
	d_serialization_ptr(new Serialization(*this)),
	d_session_management_ptr(new SessionManagement(*this)),
	d_user_preferences_ptr(new UserPreferences(NULL)),
	d_reconstruct_method_registry(new ReconstructMethodRegistry()),
	d_layer_task_registry(new LayerTaskRegistry()),
	d_log_model(new LogModel(NULL)),
	d_reconstruct_graph(new ReconstructGraph(*this)),
	d_update_default_reconstruction_tree_layer(true),
	d_reconstruction_time(0.0),
	d_anchored_plate_id(0),
	d_reconstruction(
			// Empty reconstruction
			Reconstruction::create(d_reconstruction_time, d_anchored_plate_id)),
	d_scoped_reconstruct_nesting_count(0),
	d_reconstruct_on_scope_exit(false),
	d_suppress_auto_layer_creation(false),
	d_callback_feature_store(d_model->root())
{
	// Register default reconstruct method types with the reconstruct method registry.
	register_default_reconstruct_method_types(*d_reconstruct_method_registry);

	// Register default layer task types with the layer task registry.
	register_default_layer_task_types(*d_layer_task_registry, *this);

	mediate_signal_slot_connections();

	// Register a model callback so we can reconstruct whenever the feature store is modified.
	d_callback_feature_store.attach_callback(new ReconstructWhenFeatureStoreIsModified(*this));
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

	Q_EMIT reconstruction_time_changed(*this, d_reconstruction_time);
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

	Q_EMIT anchor_plate_id_changed(*this, d_anchored_plate_id);
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
	d_reconstruction = d_reconstruct_graph->update_layer_tasks(d_reconstruction_time, d_anchored_plate_id);

	//PROFILE_BLOCK("ApplicationState::reconstruct: emit reconstructed");

	Q_EMIT reconstructed(*this);
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

	// If we're not suppressing auto-creation of layers then auto-create layers.
	// An example of suppression is loading a Session from SessionManagement which explicitly
	// restores the auto-created layers itself.
	boost::optional<ReconstructGraph::AutoCreateLayerParams> auto_create_layers;
	if (!d_suppress_auto_layer_creation)
	{
		auto_create_layers =
				ReconstructGraph::AutoCreateLayerParams(
						// Do we update the default reconstruction tree layer when loading a rotation file ?
						d_update_default_reconstruction_tree_layer);
	}

	// Add the new files to the reconstruct graph.
	// NOTE: We add them to the reconstruct graph as a group to avoid the problem of slots,
	// connected to its layer creation, attempting to access any of the loaded files and expecting
	// them to have been added to the reconstruct graph.
	d_reconstruct_graph->add_files(new_files, auto_create_layers);
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

	// Pass the signal onto the reconstruct graph first.
	// We do this rather than connect it to the signal directly so we can control
	// the order in which things happen.
	d_reconstruct_graph->remove_file(file_about_to_be_removed);
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

GPlatesFileIO::FeatureCollectionFileFormat::Registry &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_format_registry()
{
	return *d_feature_collection_file_format_registry;
}

GPlatesAppLogic::FeatureCollectionFileIO &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_io()
{
	return *d_feature_collection_file_io;
}


GPlatesAppLogic::Serialization &
GPlatesAppLogic::ApplicationState::get_serialization()
{
	return *d_serialization_ptr;
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


GPlatesAppLogic::LogModel &
GPlatesAppLogic::ApplicationState::get_log_model()
{
	return *d_log_model;
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
