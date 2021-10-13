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

#include <QDebug>

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


const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type>
GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometries_from_triangulation_2() const
{
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type> ret;

	// 
	// 2D 
	//

	// Iterate over the individual faces of the constrained triangulation and create a
	// ResolvedTopologicalNetwork for each one.
	CgalUtils::cgal_finite_faces_2_iterator finite_faces_2_iter = 
		d_delaunay_triangulation_2->finite_faces_begin();
	CgalUtils::cgal_finite_faces_2_iterator finite_faces_2_end = 
		d_delaunay_triangulation_2->finite_faces_end();

	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter)
	{
			std::vector<GPlatesMaths::PointOnSphere> network_triangle_points;
			network_triangle_points.reserve(3);

			for (int index = 0; index != 3 ; ++index)
			{
				const CgalUtils::cgal_point_2_type cgal_triangle_point =
						finite_faces_2_iter->vertex( index )->point();
				const float lon = cgal_triangle_point.x();
				const float lat = cgal_triangle_point.y();
	
				// convert coordinates
				const GPlatesMaths::LatLonPoint triangle_point_lat_lon(lat, lon);
				const GPlatesMaths::PointOnSphere triangle_point =
						GPlatesMaths::make_point_on_sphere(triangle_point_lat_lon);
				network_triangle_points.push_back(triangle_point);
			}
		
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch(...)
			{
				std::cout << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2\n"; 
			}
	}

	return ret;
}

const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type>
GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometries() const
{
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type> ret;

	// 
	// 2D + Constraints
	//

	// Iterate over the individual faces of the constrained triangulation and create a
	// ResolvedTopologicalNetwork for each one.
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_iter = 
		d_constrained_delaunay_triangulation_2->finite_faces_begin();
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_end = 
		d_constrained_delaunay_triangulation_2->finite_faces_end();

	for ( ; constrained_finite_faces_2_iter != constrained_finite_faces_2_end; ++constrained_finite_faces_2_iter)
	{
		// Only draw those triangles in the interior of the meshed region
		// This excludes areas with seed points
		// and regions outside the envelope of the bounded area 
		if ( constrained_finite_faces_2_iter->is_in_domain() )
		{
//#if 0
			std::vector<GPlatesMaths::PointOnSphere> network_triangle_points;
			network_triangle_points.reserve(3);

			for (int index = 0; index != 3 ; ++index)
			{
				const CgalUtils::cgal_point_2_type cgal_triangle_point =
						constrained_finite_faces_2_iter->vertex( index )->point();
				const float lon = cgal_triangle_point.x();
				const float lat = cgal_triangle_point.y();
	
				// convert coordinates
				const GPlatesMaths::LatLonPoint triangle_point_lat_lon(lat, lon);
				const GPlatesMaths::PointOnSphere triangle_point =
						GPlatesMaths::make_point_on_sphere(triangle_point_lat_lon);
				network_triangle_points.push_back(triangle_point);
			}
		
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch(...)
			{
				std::cout << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2\n"; 
			}
//#endif 
			// continue;
		} 
		// end of exclude seed points 
		else
		{
			std::vector<GPlatesMaths::PointOnSphere> network_triangle_points;
			network_triangle_points.reserve(3);

			for (int index = 0; index != 3 ; ++index)
			{
				const CgalUtils::cgal_point_2_type cgal_triangle_point =
						constrained_finite_faces_2_iter->vertex( index )->point();
				const float lon = cgal_triangle_point.x();
				const float lat = cgal_triangle_point.y();
	
				// convert coordinates
				const GPlatesMaths::LatLonPoint triangle_point_lat_lon(lat, lon);
				const GPlatesMaths::PointOnSphere triangle_point =
						GPlatesMaths::make_point_on_sphere(triangle_point_lat_lon);
				network_triangle_points.push_back(triangle_point);
			}
		
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch(...)
			{
				std::cout << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2\n"; 
			}
		} 
	}

	return ret;
}

const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type>
GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometries_mesh() const
{
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type> ret;

	// 
	// 2D + Constraints
	//

	// Iterate over the individual faces of the constrained triangulation and create a
	// ResolvedTopologicalNetwork for each one.
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_iter = 
		d_constrained_delaunay_triangulation_2->finite_faces_begin();
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_end = 
		d_constrained_delaunay_triangulation_2->finite_faces_end();

	for ( ; constrained_finite_faces_2_iter != constrained_finite_faces_2_end; ++constrained_finite_faces_2_iter)
	{

		// Only draw those triangles in the interior of the meshed region
		// This excludes areas with seed points
		// and regions outside the envelope of the bounded area 
		if ( constrained_finite_faces_2_iter->is_in_domain() )
		{
			std::vector<GPlatesMaths::PointOnSphere> network_triangle_points;
			network_triangle_points.reserve(3);

			for (int index = 0; index != 3 ; ++index)
			{
				const CgalUtils::cgal_point_2_type cgal_triangle_point =
						constrained_finite_faces_2_iter->vertex( index )->point();
				const float lon = cgal_triangle_point.x();
				const float lat = cgal_triangle_point.y();
	
				// convert coordinates
				const GPlatesMaths::LatLonPoint triangle_point_lat_lon(lat, lon);
				const GPlatesMaths::PointOnSphere triangle_point =
						GPlatesMaths::make_point_on_sphere(triangle_point_lat_lon);
				network_triangle_points.push_back(triangle_point);
			}
		
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch(...)
			{
				std::cout << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2\n"; 
			}
		} 
		// end of exclude seed points 
		else
		{
			continue;

#if 0
			std::vector<GPlatesMaths::PointOnSphere> network_triangle_points;
			network_triangle_points.reserve(3);

			for (int index = 0; index != 3 ; ++index)
			{
				const CgalUtils::cgal_point_2_type cgal_triangle_point =
						constrained_finite_faces_2_iter->vertex( index )->point();
				const float lon = cgal_triangle_point.x();
				const float lat = cgal_triangle_point.y();
	
				// convert coordinates
				const GPlatesMaths::LatLonPoint triangle_point_lat_lon(lat, lon);
				const GPlatesMaths::PointOnSphere triangle_point =
						GPlatesMaths::make_point_on_sphere(triangle_point_lat_lon);
				network_triangle_points.push_back(triangle_point);
			}
		
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch(...)
			{
				std::cout << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2\n"; 
			}
#endif
		} 
	}

	return ret;
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
