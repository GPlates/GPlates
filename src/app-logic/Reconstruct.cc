/* $Id$ */

/**
 * \file Reconstructs feature geometry(s) from present day to the past.
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

#include "Reconstruct.h"

#include "AppLogicUtils.h"
#include "FeatureCollectionFileState.h"
#include "ReconstructUtils.h"

#include "feature-visitors/TopologyResolver.h"


namespace
{
	bool
	has_reconstruction_time_changed(
			double old_reconstruction_time,
			double new_reconstruction_time)
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
	get_active_feature_collections_from_application_state(
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


GPlatesAppLogic::Reconstruct::Reconstruct(
		GPlatesModel::ModelInterface &model,
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		double reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		Hook *reconstruction_hook) :
	d_model(model),
	d_file_state(file_state),
	d_reconstruction_time(reconstruction_time),
	d_anchored_plate_id(anchored_plate_id),
	d_reconstruction(
			ReconstructUtils::create_empty_reconstruction(
					reconstruction_time, anchored_plate_id)),
	d_reconstruction_hook(reconstruction_hook)
{
}


void
GPlatesAppLogic::Reconstruct::reconstruct()
{
	// Reconstruct before we tell everyone that we've reconstructed!
	reconstruct_application_state();

	emit reconstructed(*this, false/*reconstruction_time_changed*/, false/*anchor_plate_id_changed*/);
}


void
GPlatesAppLogic::Reconstruct::reconstruct_to_time(
		double new_reconstruction_time)
{
	// See if the reconstruction time has changed.
	const bool reconstruction_time_changed = has_reconstruction_time_changed(
			d_reconstruction_time, new_reconstruction_time);

	d_reconstruction_time = new_reconstruction_time;

	// Reconstruct before we tell everyone that we've reconstructed!
	reconstruct_application_state();

	emit reconstructed(*this, reconstruction_time_changed, false/*anchor_plate_id_changed*/);
}


void
GPlatesAppLogic::Reconstruct::reconstruct_with_anchor(
		unsigned long new_anchor_plate_id)
{
	const bool anchor_plate_id_changed = has_anchor_plate_id_changed(
			d_anchored_plate_id, new_anchor_plate_id);

	d_anchored_plate_id = new_anchor_plate_id;

	// Reconstruct before we tell everyone that we've reconstructed!
	reconstruct_application_state();

	emit reconstructed(*this, false/*reconstruction_time_changed*/, anchor_plate_id_changed);
}


void
GPlatesAppLogic::Reconstruct::reconstruct_to_time_with_anchor(
		double new_reconstruction_time,
		unsigned long new_anchor_plate_id)
{
	// See if the reconstruction time has changed.
	const bool reconstruction_time_changed = has_reconstruction_time_changed(
			d_reconstruction_time, new_reconstruction_time);

	d_reconstruction_time = new_reconstruction_time;

	const bool anchor_plate_id_changed = has_anchor_plate_id_changed(
			d_anchored_plate_id, new_anchor_plate_id);

	d_anchored_plate_id = new_anchor_plate_id;

	// Reconstruct before we tell everyone that we've reconstructed!
	reconstruct_application_state();

	emit reconstructed(*this, reconstruction_time_changed, anchor_plate_id_changed);
}


void
GPlatesAppLogic::Reconstruct::reconstruct_application_state()
{
	//
	// Call the client's callback before the reconstruction.
	//
	if (d_reconstruction_hook)
	{
		d_reconstruction_hook->begin_reconstruction(
				d_model,
				d_reconstruction_time,
				d_anchored_plate_id);
	}


	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
		reconstructable_features_collection,
		reconstruction_features_collection;

	get_active_feature_collections_from_application_state(
			d_file_state,
			reconstructable_features_collection,
			reconstruction_features_collection);

	// Perform the actual reconstruction.
	std::pair<
		const GPlatesModel::Reconstruction::non_null_ptr_type,
		boost::shared_ptr<GPlatesFeatureVisitors::TopologyResolver> >
				reconstruct_result =
						ReconstructUtils::create_reconstruction(
								reconstructable_features_collection,
								reconstruction_features_collection,
								d_reconstruction_time,
								d_anchored_plate_id);

	// Unpack the results of the reconstruction.
	d_reconstruction = reconstruct_result.first;
	GPlatesFeatureVisitors::TopologyResolver &topology_resolver =
			*reconstruct_result.second;

	//
	// Call the client's callback after the reconstruction.
	//
	if (d_reconstruction_hook)
	{
		d_reconstruction_hook->end_reconstruction(
				d_model,
				*d_reconstruction,
				d_reconstruction_time,
				d_anchored_plate_id,
				reconstructable_features_collection,
				reconstruction_features_collection,
				topology_resolver);
	}
}
