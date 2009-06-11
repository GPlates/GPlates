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

#include "ReconstructContext.h"

#include "app-logic/Reconstruct.h"

#include "model/Model.h"


GPlatesViewOperations::ReconstructContext::ReconstructContext(
		GPlatesModel::ModelInterface &model) :
	d_model(model),
	d_current_reconstruction(model->create_empty_reconstruction(0.0, 0))
{
}


GPlatesViewOperations::ReconstructContext::ReconstructContext(
		GPlatesModel::ModelInterface &model,
		ReconstructHook::non_null_ptr_type reconstruct_hook) :
	d_model(model),
	d_current_reconstruction(model->create_empty_reconstruction(0.0, 0)),
	// We can do this because reconstruct_hook is non_null_ptr_type and hence
	// has a reference count greater than zero.
	d_reconstruct_hook(reconstruct_hook.get())
{
}


void
GPlatesViewOperations::ReconstructContext::set_reconstruct_hook(
		ReconstructHook::non_null_ptr_type reconstruct_hook)
{
	// We can do this because reconstruct_hook is non_null_ptr_type and hence
	// has a reference count greater than zero.
	d_reconstruct_hook = reconstruct_hook.get();
}


void
GPlatesViewOperations::ReconstructContext::reconstruct(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id)
{
	//
	// Call pre-reconstruction hook.
	//
	if (d_reconstruct_hook)
	{
		d_reconstruct_hook->pre_reconstruction_hook(
				d_model,
				reconstruction_time,
				reconstruction_anchored_plate_id);
	}

	// Get app logic to perform a reconstruction.
	std::pair<
		const GPlatesModel::Reconstruction::non_null_ptr_type,
		boost::shared_ptr<GPlatesFeatureVisitors::TopologyResolver> >
				reconstruct_result =
						GPlatesAppLogic::Reconstruct::create_reconstruction(
								reconstructable_features_collection,
								reconstruction_features_collection,
								reconstruction_time,
								reconstruction_anchored_plate_id);

	// Unpack the results of the reconstruction.
	d_current_reconstruction = reconstruct_result.first;
	GPlatesFeatureVisitors::TopologyResolver &topology_resolver =
			*reconstruct_result.second;

	//
	// Call post-reconstruction hook.
	//
	if (d_reconstruct_hook)
	{
		d_reconstruct_hook->post_reconstruction_hook(
				d_model,
				*d_current_reconstruction,
				reconstruction_time,
				reconstruction_anchored_plate_id,
				topology_resolver);
	}
}
