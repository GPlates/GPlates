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

#include "ResolvedTopologicalNetwork.h"

#include "ReconstructionGeometryVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/IntrusivePointerZeroRefCountException.h"
#include "global/NotYetImplementedException.h"

#include "model/WeakObserverVisitor.h"

#include "utils/GeometryCreationUtils.h"


const GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::ResolvedTopologicalNetwork::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return GPlatesModel::FeatureHandle::weak_ref();
	}
}


const GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type
GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry() const
{
#if 1
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
#else
	// Get geometry-on-sphere.
	const geometry_ptr_type geom_on_sphere = geometry();

	// We know the geometry is always a polygon-on-sphere since this class created it
	// so we can dynamic cast it.
	const resolved_topology_geometry_ptr_type::element_type *poly_on_sphere =
			dynamic_cast<const resolved_topology_geometry_ptr_type::element_type *>(
					geom_on_sphere.get());
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			poly_on_sphere, GPLATES_ASSERTION_SOURCE);

	// Increment reference count to same geometry and return smart pointer.
	return resolved_topology_geometry_ptr_type(
			poly_on_sphere,
			GPlatesUtils::NullIntrusivePointerHandler());
#endif
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(this->get_non_null_pointer_to_const());
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(this->get_non_null_pointer());
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_resolved_topological_network(*this);
}
