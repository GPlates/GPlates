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
GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometries_from_triangulation_2(bool clip_to_mesh) const
{
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type> ret;

	// 
	// 2D 
	//

	// Compute the centroid of the boundary polygon and get lat lon for projection
	const GPlatesMaths::LatLonPoint proj_center =
			GPlatesMaths::make_lat_lon_point(
					GPlatesMaths::PointOnSphere(
							d_boundary_polygon_ptr->get_centroid()));

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

		std::vector<CgalUtils::cgal_point_2_type> centroid_points_2;
		centroid_points_2.reserve(3);

		// save for example on bary centric calcs:
		// std::vector<std::pair<CgalUtils::cgal_point_2_type, CgalUtils::cgal_coord_type> > bary_points_2;
		// bary_points_2.reserve(3);

		for (int index = 0; index != 3 ; ++index)
		{
			const CgalUtils::cgal_point_2_type cgal_triangle_point =
					finite_faces_2_iter->vertex( index )->point();

			centroid_points_2.push_back( cgal_triangle_point );
			// bary_points_2.push_back( std::make_pair(cgal_triangle_point, 1.0) ); 

			// convert coordinates
			network_triangle_points.push_back(
					CgalUtils::project_azimuthal_equal_area_to_point_on_sphere(
							cgal_triangle_point, proj_center));
		}

		if ( clip_to_mesh )
		{ // only create face polygon if its centroid is in mesh area

			// compute centroid of face
			const CgalUtils::cgal_point_2_type c2 = 
				CGAL::centroid(centroid_points_2.begin(), centroid_points_2.end(),CGAL::Dimension_tag<0>());
			//std::cout << c2 << std::endl;

			#if 0
			// compute barry center with equal weighting 
			const CgalUtils::cgal_point_2_type bc2 = 
				CGAL::barycenter(bary_points_2.begin(), bary_points_2.end());
			std::cout << bc2 << std::endl;
			#endif

			bool in_mesh = true;
			in_mesh = CgalUtils::is_point_in_mesh(
				c2,
				get_delaunay_triangulation_2(), 
				get_constrained_delaunay_triangulation_2() ); 

			if (in_mesh) 
			{ 
				try 
				{
					// create a PolygonOnSphere
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
						GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
					ret.push_back(resolved_topology_network_triangle);
				}
				catch (std::exception &exc)
				{
					qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: " << exc.what();
				}
				catch(...)
				{
					qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: Unknown error";
				}
			}
		}
		else
		{ 
			// create all triangles
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch (std::exception &exc)
			{
				qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: " << exc.what();
			}
			catch(...)
			{
				qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: Unknown error";
			}
		}
	}

	return ret;
}

const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type>
GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometries_from_constrained(bool clip_to_mesh) const
{
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type> ret;

	// 
	// 2D + Constraints
	//

	// Compute the centroid of the boundary polygon and get lat lon for projection
	const GPlatesMaths::LatLonPoint proj_center =
			GPlatesMaths::make_lat_lon_point(
					GPlatesMaths::PointOnSphere(
							d_boundary_polygon_ptr->get_centroid()));

	// Iterate over the individual faces of the constrained triangulation and create a
	// ResolvedTopologicalNetwork for each one.
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_iter = 
		d_constrained_delaunay_triangulation_2->finite_faces_begin();
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_end = 
		d_constrained_delaunay_triangulation_2->finite_faces_end();

	for ( ; constrained_finite_faces_2_iter != constrained_finite_faces_2_end; ++constrained_finite_faces_2_iter)
	{
		std::vector<GPlatesMaths::PointOnSphere> network_triangle_points;
		std::vector<CgalUtils::cgal_point_2_type> points_2;
		network_triangle_points.reserve(3);

		for (int index = 0; index != 3 ; ++index)
		{
			const CgalUtils::cgal_point_2_type cgal_triangle_point =
					constrained_finite_faces_2_iter->vertex( index )->point();

			points_2.push_back( cgal_triangle_point );

			// convert coordinates
			network_triangle_points.push_back(
					CgalUtils::project_azimuthal_equal_area_to_point_on_sphere(
							cgal_triangle_point, proj_center));
		}
		
		if ( clip_to_mesh )
		{
			// compute centoid of face
			const CgalUtils::cgal_point_2_type c2 = 
				CGAL::centroid(points_2.begin(), points_2.end(),CGAL::Dimension_tag<0>());

			// test if centroid of face is in mesh zone
			bool in_mesh = true;
			in_mesh = CgalUtils::is_point_in_mesh(
				c2,
				get_delaunay_triangulation_2(), // d_delaunay_triangulation_2,
				get_constrained_delaunay_triangulation_2() ); // d_constrained_delaunay_triangulation_2

			if (in_mesh) 
			{ 
				try 
				{
					// create a PolygonOnSphere
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
						GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
					ret.push_back(resolved_topology_network_triangle);
				}
				catch (std::exception &exc)
				{
					qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: " << exc.what();
				}
				catch(...)
				{
					qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: Unknown error";
				}
			}
		}
		else
		{ 
			// create all face polygons
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch (std::exception &exc)
			{
				qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: " << exc.what();
			}
			catch(...)
			{
				qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: Unknown error";
			}
		} 
	}
	return ret;
}

const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type>
GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometries_from_mesh() const
{
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::resolved_topology_geometry_ptr_type> ret;

	// 
	// 2D + C + Mesh 
	//

	// Compute the centroid of the boundary polygon and get lat lon for projection
	const GPlatesMaths::LatLonPoint proj_center =
			GPlatesMaths::make_lat_lon_point(
					GPlatesMaths::PointOnSphere(
							d_boundary_polygon_ptr->get_centroid()));

	// Iterate over the individual faces of the constrained triangulation and create a
	// ResolvedTopologicalNetwork for each one.
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_iter = 
		d_constrained_delaunay_triangulation_2->finite_faces_begin();
	CgalUtils::cgal_constrained_finite_faces_2_iterator constrained_finite_faces_2_end = 
		d_constrained_delaunay_triangulation_2->finite_faces_end();

	for ( ; constrained_finite_faces_2_iter != constrained_finite_faces_2_end; ++constrained_finite_faces_2_iter)
	{
		// Only draw those triangles in the interior of the meshed region
		// This excludes areas with seed points (also called micro blocks) 
		// and excludes regions outside the envelope of the bounded area 
		if ( constrained_finite_faces_2_iter->is_in_domain() )
		{
			std::vector<GPlatesMaths::PointOnSphere> network_triangle_points;
			network_triangle_points.reserve(3);

			for (int index = 0; index != 3 ; ++index)
			{
				const CgalUtils::cgal_point_2_type cgal_triangle_point =
						constrained_finite_faces_2_iter->vertex( index )->point();
	
				// convert coordinates
				network_triangle_points.push_back(
						CgalUtils::project_azimuthal_equal_area_to_point_on_sphere(
								cgal_triangle_point, proj_center));
			}
		
			try 
			{
				// create a PolygonOnSphere
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_network_triangle = 
					GPlatesMaths::PolygonOnSphere::create_on_heap(network_triangle_points);
				ret.push_back(resolved_topology_network_triangle);
			}
			catch (std::exception &exc)
			{
				qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: " << exc.what();
			}
			catch(...)
			{
				qWarning() << "Cannot create polygon on sphere from d_constrained_delaunay_triangulation_2: Unknown error";
			}
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
