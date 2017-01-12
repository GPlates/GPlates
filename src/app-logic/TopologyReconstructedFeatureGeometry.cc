/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "TopologyReconstructedFeatureGeometry.h"

#include "ReconstructionGeometryVisitor.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::TopologyReconstructedFeatureGeometry::geometry_ptr_type
GPlatesAppLogic::TopologyReconstructedFeatureGeometry::reconstructed_geometry() const
{
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> reconstructed_geom =
			d_topology_reconstruct_geometry_time_span->get_geometry(get_reconstruction_time());

	// We should not have been created if the geometry time span was not valid at the current reconstruction time.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			reconstructed_geom,
			GPLATES_ASSERTION_SOURCE);

	return reconstructed_geom.get();
}


void
GPlatesAppLogic::TopologyReconstructedFeatureGeometry::get_geometry_data(
		boost::optional<point_seq_type &> reconstructed_points,
		boost::optional<point_deformation_strain_rate_seq_type &> strain_rates,
		boost::optional<point_deformation_total_strain_seq_type &> strains) const
{
	const bool geometry_data_is_valid = d_topology_reconstruct_geometry_time_span->get_geometry_data(
			get_reconstruction_time(),
			reconstructed_points,
			strain_rates,
			strains);

	// We should not have been created if the geometry time span was not valid at the current reconstruction time.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			geometry_data_is_valid,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::TopologyReconstructedFeatureGeometry::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::TopologyReconstructedFeatureGeometry::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::TopologyReconstructedFeatureGeometry::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_topology_reconstructed_feature_geometry(*this);
}
