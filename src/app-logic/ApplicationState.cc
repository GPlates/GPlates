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

#include "AgeModelCollection.h"
#include "AppLogicUtils.h"
#include "FeatureCollectionFileIO.h"
#include "Layer.h"
#include "LayerProxyUtils.h"
#include "LayerTask.h"
#include "LayerTaskRegistry.h"
#include "LogModel.h"
#include "ReconstructGraph.h"
#include "ReconstructMethodRegistry.h"
#include "ReconstructUtils.h"
#include "TopologyInternalUtils.h"
#include "UserPreferences.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "model/NotificationGuard.h"

#include "opengl/VulkanException.h"
#include "opengl/VulkanHpp.h"

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
	d_feature_collection_file_format_registry(
			new GPlatesFileIO::FeatureCollectionFileFormat::Registry()),
	d_feature_collection_file_state(
			new FeatureCollectionFileState(d_model)),
	d_feature_collection_file_io(
			new FeatureCollectionFileIO(
					d_model,
					*d_feature_collection_file_format_registry,
					*d_feature_collection_file_state)),
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
	d_currently_reconstructing(false),
	d_currently_creating_reconstruction(false),
	d_suppress_auto_layer_creation(false),
	d_callback_feature_store(d_model->root()),
	d_age_model_collection(new AgeModelCollection())
{
	// Initialise the Vulkan graphics and compute library.
	initialise_vulkan();

	// Register default layer task types with the layer task registry.
	register_default_layer_task_types(*d_layer_task_registry, *this);

	mediate_signal_slot_connections();

	// Register a model callback so we can reconstruct whenever the feature store is modified.
	d_callback_feature_store.attach_callback(new FeatureStoreIsModified(*this));
}


GPlatesAppLogic::ApplicationState::~ApplicationState()
{
	// Need destructor defined in ".cc" file because boost::scoped_ptr destructor
	// needs complete type.

	// Disconnect from the file state signals.
	//
	// One reason is we need to disconnect from the file state 'remove file' signal because
	// we delegate to ReconstructGraph which is one of our data members and if we don't disconnect
	// then it's possible that we'll delegate to an already destroyed ReconstructGraph as our other
	// data member, FeatureCollectionFileState, is being destroyed.
	//
	// Another reason is we need to disconnect from the file state 'state changed' signal because
	// it resets 'd_current_topological_sections' which is also destroyed by the time this happens and
	// (on some systems such as Ubuntu 18.04+) this results in attempting to free the same memory twice.
	QObject::disconnect(&get_feature_collection_file_state(), 0, this, 0);
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

	PROFILE_FUNC();

	// Ensure that our model notification event does not cause 'reconstruct()' to be called again
	// (re-entered) since this can create an infinite cycle.
	ScopedBooleanGuard scoped_reentrant_guard(*this, &ApplicationState::d_currently_reconstructing);

	// We want to merge model events across this scope to avoid any interference during the
	// reconstruction process. This currently can happen when visiting features with a *non-const*
	// feature visitor which currently sets visited cloned properties on the feature if they differ
	// from the original ones (and this can happen when a property's instance ID is updated even
	// if the property is not actually modified). This model event in turn can cause the
	// reconstruction tree layer to invalidate its reconstruction tree cache while its still being
	// used thus resulting in a crash. This will no longer be a problem once the new model
	// (for 'pygplates') is complete. In the meantime model notification guards and using the hacked
	// feature visitor FeatureVisitorThatGuaranteesNotToModify (eg, on ReconstructionGraphPopulator)
	// are the best alternatives.
	//
	// Note: We also do this *after* the above scoped re-entrant guard so that any model events do
	// not cause a recursive reconstruction.
	GPlatesModel::NotificationGuard model_notification_guard(*d_model.access_model());

	{
		// Used to detect when clients call us during this scope.
		ScopedBooleanGuard scoped_creating_reconstruction_guard(*this, &ApplicationState::d_currently_creating_reconstruction);

		// Get each layer to update itself in response to any changes that caused this 'reconstruct' method to get called.
		d_reconstruction = d_reconstruct_graph->update_layer_tasks(d_reconstruction_time, d_anchored_plate_id);
	}

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


void
GPlatesAppLogic::ApplicationState::handle_file_state_changed(
		FeatureCollectionFileState &file_state)
{
	// The set of referenced topological sections only changes when a file is loaded/removed/modified,
	// so it makes sense to only recalculate when that happens rather than every time the view is rendered.
	//
	// Reset the cache so the next time they are requested they'll have to be recalculated.
	d_current_topological_sections = boost::none;
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

const GPlatesAppLogic::FeatureCollectionFileState &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_state() const
{
	return *d_feature_collection_file_state;
}


GPlatesFileIO::FeatureCollectionFileFormat::Registry &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_format_registry()
{
	return *d_feature_collection_file_format_registry;
}

const GPlatesFileIO::FeatureCollectionFileFormat::Registry &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_format_registry() const
{
	return *d_feature_collection_file_format_registry;
}


GPlatesAppLogic::FeatureCollectionFileIO &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_io()
{
	return *d_feature_collection_file_io;
}

const GPlatesAppLogic::FeatureCollectionFileIO &
GPlatesAppLogic::ApplicationState::get_feature_collection_file_io() const
{
	return *d_feature_collection_file_io;
}


GPlatesAppLogic::UserPreferences &
GPlatesAppLogic::ApplicationState::get_user_preferences()
{
	return *d_user_preferences_ptr;
}

const GPlatesAppLogic::UserPreferences &
GPlatesAppLogic::ApplicationState::get_user_preferences() const
{
	return *d_user_preferences_ptr;
}


GPlatesAppLogic::ReconstructMethodRegistry &
GPlatesAppLogic::ApplicationState::get_reconstruct_method_registry()
{
	return *d_reconstruct_method_registry;
}

const GPlatesAppLogic::ReconstructMethodRegistry &
GPlatesAppLogic::ApplicationState::get_reconstruct_method_registry() const
{
	return *d_reconstruct_method_registry;
}


GPlatesAppLogic::LayerTaskRegistry &
GPlatesAppLogic::ApplicationState::get_layer_task_registry()
{
	return *d_layer_task_registry;
}

const GPlatesAppLogic::LayerTaskRegistry &
GPlatesAppLogic::ApplicationState::get_layer_task_registry() const
{
	return *d_layer_task_registry;
}


GPlatesAppLogic::LogModel &
GPlatesAppLogic::ApplicationState::get_log_model()
{
	return *d_log_model;
}

const GPlatesAppLogic::LogModel &
GPlatesAppLogic::ApplicationState::get_log_model() const
{
	return *d_log_model;
}


GPlatesAppLogic::ReconstructGraph &
GPlatesAppLogic::ApplicationState::get_reconstruct_graph()
{
	return *d_reconstruct_graph;
}

const GPlatesAppLogic::ReconstructGraph &
GPlatesAppLogic::ApplicationState::get_reconstruct_graph() const
{
	return *d_reconstruct_graph;
}


const std::set<GPlatesModel::FeatureId> &
GPlatesAppLogic::ApplicationState::get_current_topological_sections() const
{
	if (!d_current_topological_sections)
	{
		// Find the feature IDs of topological sections referenced for *all* times by all topologies
		// (topological geometry and network) in all loaded files.
		//
		// The set of referenced topological sections only changes when a file is loaded/removed/modified,
		// so it makes sense to only recalculate when that happens rather than every time the view is rendered.
		// It's not hugely expensive to calculate (tens of milliseconds) but enough that it makes sense not
		// to calculate it at every scene render.
		find_current_topological_sections();
	}

	return d_current_topological_sections.get();
}


void
GPlatesAppLogic::ApplicationState::find_current_topological_sections() const
{
	PROFILE_FUNC();

	// Start with an empty set of topological sections.
	d_current_topological_sections = std::set<GPlatesModel::FeatureId>();

	//
	// Iterate through the loaded files searching for topological features and
	// the topological sections they reference.
	//
	// We could have used LayerProxyUtils::find_dependent_topological_sections() instead, and it is
	// less expensive since the topological layers already keep track of dependent topological sections,
	// but it is difficult to get the signaling right. In other words, sometimes a caller calls
	// 'get_current_topological_sections()' just after a file is loaded but before the reconstruct
	// graph has been updated, and hence the topology layers are not yet up-to-date.
	// So instead we just search through all loaded files whenever a file is added/removed/modified.
	// It's slower but at least this function isn't called for each reconstruction or globe/map view redraw.
	//

	// From the file state, obtain the list of all currently loaded files.
	const std::vector<FeatureCollectionFileState::file_reference> loaded_files =
			d_feature_collection_file_state->get_loaded_files();

	// Iterate over the loaded files and find topological sections referenced in any of them.
	BOOST_FOREACH(FeatureCollectionFileState::file_reference file_ref, loaded_files)
	{
		// Insert referenced topological section feature IDs (for *all* reconstruction times).
		TopologyInternalUtils::find_topological_sections_referenced(
				d_current_topological_sections.get(),
				file_ref.get_file().get_feature_collection());
	}
}


void
GPlatesAppLogic::ApplicationState::initialise_vulkan()
{
	// Prefer the VK_LAYER_KHRONOS_validation layer, with fallback to the
	// deprecated VK_LAYER_LUNARG_standard_validation layer.
	//
	// VK_LAYER_KHRONOS_validation now incorporates (as of the 1.1.106 SDK):
	// 
	//   VK_LAYER_GOOGLE_threading
	//   VK_LAYER_LUNARG_parameter_validation
	//   VK_LAYER_LUNARG_object_tracker
	//   VK_LAYER_LUNARG_core_validation
	//   VK_LAYER_GOOGLE_unique_objects
	//
	if (d_vulkan_instance.supportedLayers().contains("VK_LAYER_KHRONOS_validation"))
	{
		d_vulkan_instance.setLayers(QByteArrayList() << "VK_LAYER_KHRONOS_validation");
	}
	else if (d_vulkan_instance.supportedLayers().contains("VK_LAYER_LUNARG_standard_validation"))
	{
		d_vulkan_instance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
	}

	// Initialise Vulkan.
	if (!d_vulkan_instance.create())
	{
		throw GPlatesOpenGL::VulkanException(
				GPLATES_EXCEPTION_SOURCE,
				"Failed to create Vulkan instance.");
	}
	qDebug() << "Vulkan enabled validation layers:" << d_vulkan_instance.layers();

	//
	// Initialise the Vulkan-Hpp C++ interface (around the Vulkan C interface).
	//
	// From here onward we'll be able to use the Vulkan-Hpp C++ interface.
	//
	GPlatesOpenGL::VulkanHpp::initialise(
			// Function pointer to vkGetInstanceProcAddr...
			reinterpret_cast<PFN_vkGetInstanceProcAddr>(d_vulkan_instance.getInstanceProcAddr("vkGetInstanceProcAddr")),
			d_vulkan_instance.vkInstance());
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
			SIGNAL(file_state_changed(
					GPlatesAppLogic::FeatureCollectionFileState &)),
			this,
			SLOT(handle_file_state_changed(
					GPlatesAppLogic::FeatureCollectionFileState &)));

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
			SIGNAL(layer_params_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::LayerParams &)),
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
GPlatesAppLogic::ApplicationState::feature_store_modified()
{
	// The set of referenced topological sections only changes when a file is loaded/removed/modified,
	// so it makes sense to only recalculate when that happens rather than every time the view is rendered.
	//
	// Reset the cache so the next time they are requested they'll have to be recalculated.
	d_current_topological_sections = boost::none;

	// Perform a reconstruction every time the model (feature store) is modified,
	// unless we are already inside a reconstruction (avoid infinite cycle).
	if (!d_currently_reconstructing)
	{
		// Clients should put model notification guards in the appropriate places to
		// avoid excessive reconstructions.
		reconstruct();
	}
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
