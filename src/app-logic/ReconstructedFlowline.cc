/* $Id: ReconstructedVirtualGeomagneticPole.cc 8651 2010-06-06 18:15:55Z jcannon $ */
 
/**
 * \file 
 * $Revision: 8651 $
 * $Date: 2010-06-06 20:15:55 +0200 (sø, 06 jun 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "ReconstructedFlowline.h"

#include "FlowlineUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionGeometryVisitor.h"


void
GPlatesAppLogic::ReconstructedFlowline::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedFlowline::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedFlowline::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_reconstructed_flowline(*this);
}


GPlatesMaths::Vector3D
GPlatesAppLogic::ReconstructedFlowline::reconstructed_seed_point_velocity(
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	// The seed point moves by half-stage rotations from the flowline's spreading start time and then
	// by the left plate.
	// So use the flowline's times, rather than a generic half-stage rotation.
	const GPlatesMaths::FiniteRotation velocity_stage_rotation =
			FlowlineUtils::calculate_seed_point_stage_rotation(
					get_reconstruction_time(),
					d_flowline_times,
					d_spreading_start_time,
					d_left_plate_id,
					d_right_plate_id,
					get_reconstruction_tree_creator(),
					velocity_delta_time,
					velocity_delta_time_type);

	// Calculate a velocity at seed point.
	return PlateVelocityUtils::calculate_velocity_vector(
			reconstructed_seed_point(),
			velocity_stage_rotation,
			velocity_delta_time,
			velocity_units,
			earth_radius_in_kms);
}
