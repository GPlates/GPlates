/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
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

#include "ReconstructedMotionPath.h"

#include "ReconstructionGeometryVisitor.h"
#include "ResolvedVertexSourceInfo.h"

#include "model/WeakObserverVisitor.h"


void
GPlatesAppLogic::ReconstructedMotionPath::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedMotionPath::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedMotionPath::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_reconstructed_motion_path(*this);
}


GPlatesMaths::Vector3D
GPlatesAppLogic::ReconstructedMotionPath::reconstructed_seed_point_velocity(
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	const ResolvedVertexSourceInfo::non_null_ptr_to_const_type reconstructed_source_info =
			ResolvedVertexSourceInfo::create(get_non_null_pointer_to_const());

	// Calculate a velocity at seed point.
	return reconstructed_source_info->get_velocity_vector(
			reconstructed_seed_point(),
			get_reconstruction_time(),
			velocity_delta_time,
			velocity_delta_time_type,
			velocity_units,
			earth_radius_in_kms);
}
