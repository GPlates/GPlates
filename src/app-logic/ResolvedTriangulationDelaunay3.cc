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

#include <CGAL/Origin.h>

#include "ResolvedTriangulationDelaunay3.h"

#include "utils/Profile.h"


bool
GPlatesAppLogic::ResolvedTriangulation::Delaunay_3::calc_surface_neighbor_coordinates(
		delaunay_natural_neighbor_coordinates_3_type &natural_neighbor_coordinates,
		const delaunay_point_3_type &point_3) const
{
	// compute the normal for the test point
	const delaunay_vector_3_type test_normal( point_3 - CGAL::ORIGIN);

	// Compute surface neighbor coordinates for test point 
	CGAL::Triple< std::back_insert_iterator<delaunay_point_coordinate_vector_3_type>, delaunay_coord_3_type, bool>
			coordinate_result =
					CGAL::surface_neighbor_coordinates_3(
							*this,
							point_3,
							test_normal,
							std::back_inserter(natural_neighbor_coordinates.first));
	
	natural_neighbor_coordinates.second = coordinate_result.second/*norm*/;

	return coordinate_result.third/*in_triangulation*/;
}


void
GPlatesAppLogic::ResolvedTriangulation::Delaunay_3::gradient_3(
		const delaunay_point_3_type &point_3,
		delaunay_map_point_to_value_3_type &function_values,
		delaunay_map_point_to_vector_3_type &function_gradients) const
{
	// compute the normal for the test point
	delaunay_vector_3_type test_normal( point_3 - CGAL::ORIGIN);

	//delaunay_map_point_to_value_3_type function_values;
	//delaunay_map_point_to_vector_3_type function_gradients;

	//typedef CGAL::Voronoi_intersection_2_traits_3<delaunay_kernel_3_type> Traits;

#if 0
 	CGAL::sibson_gradient_fitting(
		*this,
		std::inserter( map_point_3_to_value_grad, map_point_3_to_value_grad.begin()),
		point_3_value_access_type(map_point_3_to_value_func),
		Traits
	);

  //Sibson interpolant: version without sqrt:
  std::pair<Coord_type, bool> res =
    CGAL::sibson_c1_interpolation_square
    (coords.begin(),
     coords.end(),norm,p,
     CGAL::Data_access<map_point_3_to_value_type>(function_values),
     CGAL::Data_access<map_point_3_to_vector_type>(function_gradients),
     Traits());
  if(res.second)
    std::cout << "   Tested interpolation on " << p
	      << " interpolation: " << res.first << " exact: "
	      << a + bx * p.x()+ by * p.y()+ c*(p.x()*p.x()+p.y()*p.y())
	      << std::endl;
  else
    std::cout << "C^1 Interpolation not successful." << std::endl
	      << " not all function_gradients are provided."  << std::endl
	      << " You may resort to linear interpolation." << std::endl;

  std::cout << "done" << std::endl;
  return 0;
#endif
}
