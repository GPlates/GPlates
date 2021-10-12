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

#include "ReconstructTemplate.h"

#include "Reconstruct.h"

#include "model/Model.h"


GPlatesAppLogic::ReconstructTemplate::ReconstructTemplate(
		GPlatesModel::ModelInterface &model) :
	d_model(model)
{
}


GPlatesModel::Reconstruction::non_null_ptr_type
GPlatesAppLogic::ReconstructTemplate::reconstruct(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id)
{
	//
	// Call template method.
	//
	begin_reconstruction(
			d_model,
			reconstruction_time,
			reconstruction_anchored_plate_id);

	// Get app logic to perform a reconstruction.
	std::pair<
		const GPlatesModel::Reconstruction::non_null_ptr_type,
		boost::shared_ptr<GPlatesFeatureVisitors::TopologyResolver> >
				reconstruct_result =
						Reconstruct::create_reconstruction(
								reconstructable_features_collection,
								reconstruction_features_collection,
								reconstruction_time,
								reconstruction_anchored_plate_id);

	// Unpack the results of the reconstruction.
	GPlatesModel::Reconstruction::non_null_ptr_type reconstruction = reconstruct_result.first;
	GPlatesFeatureVisitors::TopologyResolver &topology_resolver =
			*reconstruct_result.second;

	//
	// Call template method.
	//
	end_reconstruction(
			d_model,
			*reconstruction,
			reconstruction_time,
			reconstruction_anchored_plate_id,
			topology_resolver);

	return reconstruction;
}
