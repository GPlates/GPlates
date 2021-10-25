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

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/WeakObserverVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/GeometryCreationUtils.h"


namespace
{
	/**
	 * Gets a @a PolygonOnSphere from a @a GeometryOnSphere if it is one.
	 */
	class PolygonGeometryFinder :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		const boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &
		get_polygon_on_sphere() const
		{
			return d_polygon_on_sphere;
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_polygon_on_sphere = polygon_on_sphere;
		}

	private:
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_polygon_on_sphere;
	};

	/**
	 * Gets a @a PolylineOnSphere from a @a GeometryOnSphere if it is one.
	 */
	class PolylineGeometryFinder :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		const boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &
		get_polyline_on_sphere() const
		{
			return d_polyline_on_sphere;
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_polyline_on_sphere = polyline_on_sphere;
		}

	private:
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> d_polyline_on_sphere;
	};
}



const GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::ResolvedTopologicalGeometry::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return GPlatesModel::FeatureHandle::weak_ref();
	}
}


boost::optional<GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_boundary_ptr_type>
GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_boundary() const
{
	// Get the polygon geometry - if it's a polygon.
	PolygonGeometryFinder polygon_geom_finder;
	d_resolved_topology_geometry_ptr->accept_visitor(polygon_geom_finder);

	return polygon_geom_finder.get_polygon_on_sphere();
}


boost::optional<GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_line_ptr_type>
GPlatesAppLogic::ResolvedTopologicalGeometry::resolved_topology_line() const
{
	// Get the polyline geometry - if it's a polyline.
	PolylineGeometryFinder polyline_geom_finder;
	d_resolved_topology_geometry_ptr->accept_visitor(polyline_geom_finder);

	return polyline_geom_finder.get_polyline_on_sphere();
}


void
GPlatesAppLogic::ResolvedTopologicalGeometry::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(this->get_non_null_pointer_to_const());
}


void
GPlatesAppLogic::ResolvedTopologicalGeometry::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(this->get_non_null_pointer());
}


void
GPlatesAppLogic::ResolvedTopologicalGeometry::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_resolved_topological_geometry(*this);
}
