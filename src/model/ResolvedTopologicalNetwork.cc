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


#include "ConstReconstructionGeometryVisitor.h"
#include "ReconstructionGeometryVisitor.h"
#include "WeakObserverVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/IntrusivePointerZeroRefCountException.h"

#include "utils/GeometryCreationUtils.h"


const GPlatesModel::ResolvedTopologicalNetwork::non_null_ptr_to_const_type
GPlatesModel::ResolvedTopologicalNetwork::get_non_null_pointer_to_const() const
{
	if (get_reference_count() == 0) {
		// How did this happen?  This should not have happened.
		//
		// Presumably, the programmer obtained the raw ResolvedTopologicalNetwork pointer
		// from inside a ResolvedTopologicalNetwork::non_null_ptr_type, and is invoking
		// this member function upon the instance indicated by the raw pointer, after all
		// ref-counting pointers have expired and the instance has actually been deleted.
		//
		// Regardless of how this happened, this is an error.
		throw GPlatesGlobal::IntrusivePointerZeroRefCountException(GPLATES_EXCEPTION_SOURCE,
				this);
	} else {
		// This instance is already managed by intrusive-pointers, so we can simply return
		// another intrusive-pointer to this instance.
		return non_null_ptr_to_const_type(
				this,
				GPlatesUtils::NullIntrusivePointerHandler());
	}
}


const GPlatesModel::ResolvedTopologicalNetwork::non_null_ptr_type
GPlatesModel::ResolvedTopologicalNetwork::get_non_null_pointer()
{
	if (get_reference_count() == 0) {
		// How did this happen?  This should not have happened.
		//
		// Presumably, the programmer obtained the raw ResolvedTopologicalNetwork pointer
		// from inside a ResolvedTopologicalNetwork::non_null_ptr_type, and is invoking
		// this member function upon the instance indicated by the raw pointer, after all
		// ref-counting pointers have expired and the instance has actually been deleted.
		//
		// Regardless of how this happened, this is an error.
		throw GPlatesGlobal::IntrusivePointerZeroRefCountException(GPLATES_EXCEPTION_SOURCE,
				this);
	} else {
		// This instance is already managed by intrusive-pointers, so we can simply return
		// another intrusive-pointer to this instance.
		return non_null_ptr_type(
				this,
				GPlatesUtils::NullIntrusivePointerHandler());
	}
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::ResolvedTopologicalNetwork::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return FeatureHandle::weak_ref();
	}
}


const GPlatesModel::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type
GPlatesModel::ResolvedTopologicalNetwork::resolved_topology_geometry() const
{
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
}


void
GPlatesModel::ResolvedTopologicalNetwork::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit_resolved_topological_network(this->get_non_null_pointer_to_const());
}


void
GPlatesModel::ResolvedTopologicalNetwork::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit_resolved_topological_network(this->get_non_null_pointer());
}


void
GPlatesModel::ResolvedTopologicalNetwork::accept_weak_observer_visitor(
		WeakObserverVisitor<FeatureHandle> &visitor)
{
	visitor.visit_resolved_topological_network(*this);
}
