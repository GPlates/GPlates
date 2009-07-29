/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include "ResolvedTopologicalGeometry.h"
#include "ReconstructionGeometryVisitor.h"
#include "ConstReconstructionGeometryVisitor.h"
#include "WeakObserverVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/IntrusivePointerZeroRefCountException.h"

#include "utils/GeometryCreationUtils.h"


const GPlatesModel::ResolvedTopologicalGeometry::non_null_ptr_to_const_type
GPlatesModel::ResolvedTopologicalGeometry::get_non_null_pointer_to_const() const
{
	if (get_reference_count() == 0) {
		// How did this happen?  This should not have happened.
		//
		// Presumably, the programmer obtained the raw ResolvedTopologicalGeometry pointer
		// from inside a ResolvedTopologicalGeometry::non_null_ptr_type, and is invoking
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


const GPlatesModel::ResolvedTopologicalGeometry::non_null_ptr_type
GPlatesModel::ResolvedTopologicalGeometry::get_non_null_pointer()
{
	if (get_reference_count() == 0) {
		// How did this happen?  This should not have happened.
		//
		// Presumably, the programmer obtained the raw ResolvedTopologicalGeometry pointer
		// from inside a ResolvedTopologicalGeometry::non_null_ptr_type, and is invoking
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
GPlatesModel::ResolvedTopologicalGeometry::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return FeatureHandle::weak_ref();
	}
}


void
GPlatesModel::ResolvedTopologicalGeometry::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit_resolved_topological_geometry(this->get_non_null_pointer_to_const());
}


void
GPlatesModel::ResolvedTopologicalGeometry::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit_resolved_topological_geometry(this->get_non_null_pointer());
}


void
GPlatesModel::ResolvedTopologicalGeometry::accept_weak_observer_visitor(
		WeakObserverVisitor<FeatureHandle> &visitor)
{
	visitor.visit_resolved_topological_geometry(*this);
}


void
GPlatesModel::ResolvedTopologicalGeometry::SubSegment::create_sub_segment_geometry() const
{
	// Caller has ensured there is at least one vertex.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_sub_segment_num_vertices != 0,
			GPLATES_ASSERTION_SOURCE);

	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity geometry_validity;

	const GPlatesMaths::PolygonOnSphere::vertex_const_iterator sub_segment_vertex_iter_begin =
			d_resolved_topology_geometry_ptr->vertex_indexed_iterator(
					d_sub_segment_vertex_index);
	const GPlatesMaths::PolygonOnSphere::vertex_const_iterator sub_segment_vertex_iter_end =
			d_resolved_topology_geometry_ptr->vertex_indexed_iterator(
					d_sub_segment_vertex_index + d_sub_segment_num_vertices);
			
	// Try and create a PolylineOnSphere.
	d_geometry_ptr = GPlatesUtils::create_polyline_on_sphere(
			sub_segment_vertex_iter_begin, sub_segment_vertex_iter_end, geometry_validity);

	// If that fails then try to create a PointOnSphere.
	if (geometry_validity != GPlatesUtils::GeometryConstruction::VALID)
	{
		d_geometry_ptr = GPlatesUtils::create_point_on_sphere(
				sub_segment_vertex_iter_begin, sub_segment_vertex_iter_end, geometry_validity);

		// The caller has ensured there's at least one vertex so we should be able to
		// create a PointOnSphere.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				geometry_validity == GPlatesUtils::GeometryConstruction::VALID,
				GPLATES_ASSERTION_SOURCE);
	}
}
