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

#include "ApplicationState.h"

#include "AppLogicUtils.h"
#include "FeatureCollectionFileIO.h"
#include "FeatureCollectionFileState.h"
#include "LayerTaskRegistry.h"
#include "LayerTaskTypes.h"
#include "ReconstructionActivationStrategy.h"
#include "ReconstructUtils.h"


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


	void
	get_feature_collections_from_file_info_collection(
			const GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range &active_files,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &features_collection)
	{
		GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator iter = active_files.begin;
		GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator end = active_files.end;
		for ( ; iter != end; ++iter)
		{
			features_collection.push_back(iter->get_feature_collection());
		}
	}


	void
	get_active_feature_collections(
			GPlatesAppLogic::FeatureCollectionFileState &file_state,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
					reconstructable_features_collection,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
					reconstruction_features_collection)
	{
		// Get the active reconstructable feature collections from the application state.
		get_feature_collections_from_file_info_collection(
				file_state.get_active_reconstructable_files(),
				reconstructable_features_collection);

		// Get the active reconstruction feature collections from the application state.
		get_feature_collections_from_file_info_collection(
				file_state.get_active_reconstruction_files(),
				reconstruction_features_collection);
	}
}


GPlatesAppLogic::ApplicationState::ApplicationState() :
	d_reconstruction_time(0.0),
	d_anchored_plate_id(0),
	d_reconstruction_tree(
			// Empty reconstruction tree
			ReconstructUtils::create_reconstruction_tree(
					d_reconstruction_time, d_anchored_plate_id)),
	d_reconstruction(
			// Empty reconstruction
			ReconstructUtils::create_reconstruction(d_reconstruction_tree)),
	d_layer_task_registry(new LayerTaskRegistry()),
	d_reconstruction_activation_strategy(
			new ReconstructionActivationStrategy()),
	d_feature_collection_file_state(
			new FeatureCollectionFileState()),
	d_feature_collection_file_io(
			new FeatureCollectionFileIO(
					d_model, *d_feature_collection_file_state)),
	d_reconstruction_hook(NULL)
{
	// We have a strategy for activating reconstruction files.
	d_feature_collection_file_state->set_reconstruction_activation_strategy(
			d_reconstruction_activation_strategy.get());

	// Register all layer task types with the layer task registry.
	LayerTaskTypes::register_layer_task_types(*d_layer_task_registry, *this);

	mediate_signal_slot_connections();
}


GPlatesAppLogic::ApplicationState::~ApplicationState()
{
	// boost::scoped_ptr destructor needs complete type
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
GPlatesAppLogic::ApplicationState::set_reconstruction_hook(
		ReconstructHook *reconstruction_hook)
{
	d_reconstruction_hook = reconstruction_hook;
}


void
GPlatesAppLogic::ApplicationState::reconstruct()
{
	//
	// Call the client's callback before the reconstruction.
	//
	if (d_reconstruction_hook)
	{
		d_reconstruction_hook->begin_reconstruction(
				get_model_interface(),
				get_current_reconstruction_time(),
				get_current_anchored_plate_id());
	}


	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
		reconstructable_features_collection,
		reconstruction_features_collection;

	get_active_feature_collections(
			get_feature_collection_file_state(),
			reconstructable_features_collection,
			reconstruction_features_collection);

	// Create a reconstruction tree at the reconstruction time.
	d_reconstruction_tree = ReconstructUtils::create_reconstruction_tree(
			get_current_reconstruction_time(),
			get_current_anchored_plate_id(),
			reconstruction_features_collection);

	// Perform the actual reconstruction using the reconstruction tree.
	d_reconstruction = ReconstructUtils::create_reconstruction(
			d_reconstruction_tree,
			reconstructable_features_collection);

	//
	// Call the client's callback after the reconstruction.
	//
	if (d_reconstruction_hook)
	{
		d_reconstruction_hook->end_reconstruction(
				get_model_interface(),
				*d_reconstruction,
				get_current_reconstruction_time(),
				get_current_anchored_plate_id(),
				reconstructable_features_collection,
				reconstruction_features_collection);
	}

	emit reconstructed(*this);
}


GPlatesModel::Reconstruction &
GPlatesAppLogic::ApplicationState::get_current_reconstruction()
{
	return *d_reconstruction;
}


GPlatesModel::Reconstruction::non_null_ptr_type
GPlatesAppLogic::ApplicationState::get_current_reconstruction_non_null_ptr()
{
	return d_reconstruction;
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


void
GPlatesAppLogic::ApplicationState::mediate_signal_slot_connections()
{
	//
	// Perform a new reconstruction whenever FeatureCollectionFileState is modified.
	//
	QObject::connect(
			&get_feature_collection_file_state(),
			SIGNAL(file_state_changed(
					GPlatesAppLogic::FeatureCollectionFileState &)),
			this,
			SLOT(reconstruct()));

	//
	// Perform a new reconstruction whenever shapefile attributes are modified.
	//
	QObject::connect(
			&get_feature_collection_file_io(),
			SIGNAL(remapped_shapefile_attributes(
					GPlatesAppLogic::FeatureCollectionFileIO &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			this,
			SLOT(reconstruct()));
}
