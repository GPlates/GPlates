/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "ReconstructedScalarCoverage.h"

#include "GeometryUtils.h"
#include "ReconstructionGeometryVisitor.h"
#include "TopologyReconstructedFeatureGeometry.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ReconstructedScalarCoverage::ReconstructedScalarCoverage(
		const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_domain_geometry,
		GPlatesModel::FeatureHandle::iterator range_property_iterator,
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type &scalar_coverage_time_span,
		boost::optional<ReconstructHandle::type> reconstruct_handle_) :
	ReconstructionGeometry(
			reconstructed_domain_geometry->get_reconstruction_time(),
			reconstruct_handle_),
	WeakObserverType(*reconstructed_domain_geometry->feature_handle_ptr()),
	d_domain_reconstructed_feature_geometry(reconstructed_domain_geometry),
	d_range_property_iterator(range_property_iterator),
	d_scalar_type(scalar_type),
	d_scalar_coverage_time_span(scalar_coverage_time_span)
{
}


void
GPlatesAppLogic::ReconstructedScalarCoverage::get_reconstructed_points(
		point_seq_type &points) const
{
	// If the domain geometry was reconstructed using topologies then delegate to it.
	//
	// Note that we could have just called our 'get_reconstructed_geometry()' and extracted points from
	// that, but it's a bit more efficient to get the points directly (when there's a large number of points).
	if (const TopologyReconstructedFeatureGeometry *topology_reconstructed_feature_geometry =
		dynamic_cast<const TopologyReconstructedFeatureGeometry *>(d_domain_reconstructed_feature_geometry.get()))
	{
		topology_reconstructed_feature_geometry->get_reconstructed_points(points);
		return;
	}
	// ...else is a regular RFG...

	// Use exterior points for now to match 'ScalarCoverageFeatureProperties::get_coverages()'.
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry = get_reconstructed_geometry();
	GeometryUtils::get_geometry_exterior_points(*geometry, points);
}


void
GPlatesAppLogic::ReconstructedScalarCoverage::get_reconstructed_point_scalar_values(
		point_scalar_value_seq_type &scalar_values) const
{
	const bool scalar_values_are_valid = d_scalar_coverage_time_span->get_scalar_values(
			get_reconstruction_time(),
			scalar_values);

	// We should not have been created if the scalar time span was not valid at the current reconstruction time.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			scalar_values_are_valid,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::ReconstructedScalarCoverage::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedScalarCoverage::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedScalarCoverage::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_reconstructed_scalar_coverage(*this);
}
