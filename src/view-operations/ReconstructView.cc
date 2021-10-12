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

#include "ReconstructView.h"

#include "app-logic/PlateVelocityWorkflow.h"

#include "view-operations/RenderReconstructionGeometries.h"


void
GPlatesViewOperations::ReconstructView::end_reconstruction(
		GPlatesModel::ModelInterface &/*model*/,
		GPlatesModel::Reconstruction &reconstruction,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection,
		GPlatesFeatureVisitors::TopologyResolver &topology_resolver)
{
	// Solve plate velocities.
	d_plate_velocity_workflow.solve_velocities(
			reconstruction,
			reconstruction_time,
			reconstruction_anchored_plate_id,
			reconstruction_features_collection,
			topology_resolver);

	// Render all reconstruction geometries as rendered geometries.
	GPlatesViewOperations::render_reconstruction_geometries(
			reconstruction,
			d_rendered_geom_collection,
			d_colour_table);
}
