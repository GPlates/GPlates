/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 * Copyright (C) 2012 2013 California Institute of Technology
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

#include "ResolvedTriangulationConstrainedDelaunay2.h"

#include "utils/Profile.h"

// #define DEBUG


bool
GPlatesAppLogic::ResolvedTriangulation::ConstrainedDelaunay_2::is_point_in_mesh(
		const constrained_delaunay_point_2_type &point) const
{
	PROFILE_FUNC();

	Locate_type locate_type;
	int number;
	Face_handle found_face = locate(point, locate_type, number);

#ifdef DEBUG
qDebug() << "\nis_point_in_mesh()";
qDebug() << "is_point_in_mesh: point = (" << point.x() << "," << point.y() << ")";
qDebug() << "is_point_in_mesh: number = " << number;
qDebug() << "is_point_in_mesh: locate_type = " << locate_type;
 	switch (locate_type)
	{
		case VERTEX:
			// Check that the vertex is not the infinite vertex...
			if ( found_face->is_in_domain() ) {
				qDebug() << "is_point_in_mesh: VERTEX: in Mesh domain";
			} else {
				qDebug() << "is_point_in_mesh: VERTEX: not in Mesh domain";
			}
			break;
		case FACE:
			qDebug() << "is_point_in_mesh: FACE";
			if ( found_face->is_in_domain() ) {
				qDebug() << "is_point_in_mesh: FACE: in Mesh domain";
			} else {
				qDebug() << "is_point_in_mesh: FACE: not in Mesh domain";
			}
			break;
		case EDGE:
			qDebug() << "is_point_in_mesh: EDGE";
			if ( found_face->is_in_domain() ) {
				qDebug() << "is_point_in_mesh: EDGE: in Mesh domain";
			} else {
				qDebug() << "is_point_in_mesh: EDGE: not in Mesh domain";
			}
			break;
		case OUTSIDE_CONVEX_HULL:
			qDebug() << "is_point_in_mesh: OUTSIDE_CONVEX_HULL";
			break;
		case OUTSIDE_AFFINE_HULL:
			qDebug() << "is_point_in_mesh: OUTSIDE_AFFINE_HULL";
			break;
		default:
			qDebug() << "is_point_in_mesh: ???";
			break;
	}
#endif

 	switch (locate_type)
	{
		case VERTEX:
			// Check that the vertex is not the infinite vertex...
			if ( found_face->is_in_domain() )
			{
				return true;
			}
			break;

		case FACE:
			if ( found_face->is_in_domain() )
			{
				return true;
			}
			break;

		case EDGE:
			if ( found_face->is_in_domain() )
			{
				return true;
			} 
			break;

		case OUTSIDE_CONVEX_HULL:
		case OUTSIDE_AFFINE_HULL:
			break;

		default:
			break;
	}

	return false;
}


bool
GPlatesAppLogic::ResolvedTriangulation::ConstrainedDelaunay_2::calc_natural_neighbor_coordinates(
		constrained_delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
		const constrained_delaunay_point_2_type &point) const
{
#ifdef HAVE_GCAL_PATCH_CDT2_H
#ifdef DEBUG
qDebug() << "constrained_delaunay_natural_neighbor_coordinates_2_type: HAVE_GCAL_PATCH_CDT2_H";
#endif
	// NOTE: 
	// must use patched version of CGAL's Constrained_Delaunay_triangulation_2.h 
	// to use our constrained_delaunay_triangulation_2_type as the base triangulation
	//

	// Build the natural neighbor coordinates.
	CGAL::Triple< std::back_insert_iterator<constrained_delaunay_point_coordinate_vector_2_type>, constrained_delaunay_coord_2_type, bool>
			coordinate_result =
					CGAL::natural_neighbor_coordinates_2(
							*this,
							point,
							std::back_inserter(natural_neighbor_coordinates.first));

	natural_neighbor_coordinates.second = coordinate_result.second/*norm*/;

	return coordinate_result.third/*in_triangulation*/;
#else
	return false;
#endif
}
