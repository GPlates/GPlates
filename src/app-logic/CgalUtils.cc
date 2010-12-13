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

#include "CgalUtils.h"

GPlatesAppLogic::CgalUtils::cgal_point_2_type
GPlatesAppLogic::CgalUtils::convert_point_to_cgal_2(
		const GPlatesMaths::PointOnSphere &point)
{
	// Create a 2D point from the point on sphere.
	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
	//qDebug() << "lon, lat=" << llp.longitude() << " , " << llp.latitude();
	return cgal_point_2_type(llp.longitude(), llp.latitude());
}


GPlatesMaths::PointOnSphere
GPlatesAppLogic::CgalUtils::convert_point_from_cgal_2(
		const cgal_point_2_type &point)
{
	// Create a 3D point on sphere from a 2D point.
	const GPlatesMaths::LatLonPoint llp(point.y(), point.x());
	return GPlatesMaths::make_point_on_sphere(llp);
}


GPlatesAppLogic::CgalUtils::cgal_point_3_type
GPlatesAppLogic::CgalUtils::convert_point_to_cgal_3(
		const GPlatesMaths::PointOnSphere &point)
{
	// Create a 3D point from the point on sphere.
	return cgal_point_3_type(
		point.position_vector().x().dval(), 
		point.position_vector().y().dval(), 
		point.position_vector().z().dval()
	);
}


GPlatesMaths::PointOnSphere
GPlatesAppLogic::CgalUtils::convert_point_from_cgal_3(
		const cgal_point_3_type &point)
{
	// Create a 3D point on sphere from a 3D CGAL point.
	const double x = point.x();
	const double y = point.y();
	const double z = point.z();
	GPlatesMaths::UnitVector3D v(x, y, z);
	return GPlatesMaths::PointOnSphere( v );
}


//
// 2D
//

boost::optional<GPlatesAppLogic::CgalUtils::interpolate_triangulation_query_type>
GPlatesAppLogic::CgalUtils::query_interpolate_triangulation_2(
		const cgal_point_2_type &point,
		const cgal_delaunay_triangulation_2_type &triangulation,
		const cgal_constrained_delaunay_triangulation_2_type &c_triangulation)
{
	// Build the natural_neighbor_coordinates_2 from the cgal_delaunay_triangulation_2_type
	cgal_point_coordinate_vector_type coords;
	CGAL::Triple< std::back_insert_iterator<cgal_point_coordinate_vector_type>, cgal_kernel_type::FT, bool>
			coordinate_result =
					CGAL::natural_neighbor_coordinates_2(
							triangulation,
							point,
							std::back_inserter(coords));

	# if 0
	// NOTE: 
	// cannot use cgal_constrained_delaunay_triangulation_2_type as the base triangulation
	// 2D + C
	cgal_point_coordinate_vector_type c_coords;
	CGAL::Triple< std::back_insert_iterator<cgal_point_coordinate_vector_type>, cgal_kernel_type::FT, bool>
			c_coordinate_result =
					CGAL::natural_neighbor_coordinates_2(
							c_triangulation,
							point,
							std::back_inserter(c_coords));
	#endif

	
	const bool in_network = coordinate_result.third;

	if (!in_network)
	{
		return boost::none;
	}

	const cgal_coord_type &norm = coordinate_result.second;

	return std::make_pair(coords, norm);
}

double
GPlatesAppLogic::CgalUtils::interpolate_triangulation_2(
		const interpolate_triangulation_query_type &point_in_triangulation_query,
		const cgal_map_point_2_to_value_type &map_point_2_to_value)
{
	const cgal_point_coordinate_vector_type &coords = point_in_triangulation_query.first;
	const cgal_coord_type &norm = point_in_triangulation_query.second;

	// Interpolate the mapped value.
	const cgal_coord_type interpolated_value = CGAL::linear_interpolation(
			coords.begin(), coords.end(), 
			norm, 
			cgal_point_2_value_access_type(map_point_2_to_value) );

	return double(interpolated_value);
}


// return the gradient vector at the point
GPlatesAppLogic::CgalUtils::cgal_vector_2_type
GPlatesAppLogic::CgalUtils::gradient_2(
		const cgal_point_2_type &test_point,
		const cgal_delaunay_triangulation_2_type &triangulation,
		const cgal_map_point_2_to_value_type &function_values)
{
	typedef CGAL::Interpolation_gradient_fitting_traits_2<cgal_kernel_type> Traits;

	// coordinate comp.
	cgal_point_coordinate_vector_type coords;

	CGAL::Triple< std::back_insert_iterator<cgal_point_coordinate_vector_type>, cgal_kernel_type::FT, bool>
		coordinate_result =
			CGAL::natural_neighbor_coordinates_2(
				triangulation,
				test_point,
				std::back_inserter(coords)
			);
	
	const bool in_network = coordinate_result.third;
	if (!in_network)
	{
		qDebug() << " !in_network ";
	}

	const cgal_coord_type &norm = coordinate_result.second;

	// Gradient Fitting
	cgal_vector_2_type gradient_vector;

	gradient_vector =
		CGAL::sibson_gradient_fitting(
			coords.begin(),
			coords.end(),
			norm,
			test_point,
			CGAL::Data_access<cgal_map_point_2_to_value_type>(function_values),
			Traits()
		);

// FIXME: this is just test code ; remove 
#if 0
// estimates the function gradients at all vertices of triangulation 
// that lie inside the convex hull using the coordinates computed 
// by the function natural_neighbor_coordinates_2.

	sibson_gradient_fitting_nn_2(
		triangulation,
		std::inserter( 
			function_gradients, 
			function_gradients.begin() ),
		CGAL::Data_access<cgal_map_point_2_to_value_type>(function_values),
		Traits()
	);
#endif

#if 0

	//Sibson interpolant: version without sqrt
	std::pair<cgal_coord_type, bool> res =
		CGAL::sibson_c1_interpolation_square(
			coords.begin(),
     		coords.end(),
			norm,
			test_point,
			CGAL::Data_access<cgal_map_point_2_to_value_type>(function_values),
			CGAL::Data_access<cgal_map_point_2_to_vector_type>(function_gradients),
			Traits());

if(res.second)
    std::cout << "   Tested interpolation on " << test_point
	      << " interpolation: " << res.first << " exact: "
	      << std::endl;
else
    std::cout << "C^1 Interpolation not successful." << std::endl
	      << " not all function_gradients are provided."  << std::endl
	      << " You may resort to linear interpolation." << std::endl;
	std::cout << "done" << std::endl;
#endif

	return gradient_vector;
}





//
// 3D
//

boost::optional<GPlatesAppLogic::CgalUtils::interpolate_triangulation_query_3_type>
GPlatesAppLogic::CgalUtils::query_interpolate_triangulation_3(
		const cgal_point_3_type &test_point,
		const cgal_triangulation_hierarchy_3_type &triangulation)
{
	// compute the normal for the test_point
	cgal_vector_3_type test_normal( test_point - CGAL::ORIGIN);

	// this vector gets filled by the call to surface_neighbor_coordinates_3
	cgal_point_coordinate_vector_3_type coords;

	//std::vector<cgal_point_3_type> points;

	// Compute surface neighbor coordinates for test point 
	CGAL::Triple< std::back_insert_iterator<cgal_point_coordinate_vector_3_type>, cgal_kernel_type::FT, bool> result =
		CGAL::surface_neighbor_coordinates_3(
				triangulation,
				test_point,
				test_normal,
				std::back_inserter(coords)
		);
	
	const bool in_network = result.third;
	if (!in_network)
	{
		return boost::none;
	}

	const cgal_coord_type &norm = result.second;
	return std::make_pair(coords, norm);
	
}


void
GPlatesAppLogic::CgalUtils::gradient_3(
		const cgal_point_3_type &test_point,
		const cgal_triangulation_hierarchy_3_type &triangulation,
		cgal_map_point_3_to_value_type &function_values, // function values
		cgal_map_point_3_to_vector_type &function_gradients)  // gradient values
{
	// compute the normal for the test_point
	cgal_vector_3_type test_normal( test_point - CGAL::ORIGIN);

	//cgal_map_point_3_to_value_type function_values;
	//cgal_map_point_3_to_vector_type function_gradients;

	typedef CGAL::Voronoi_intersection_2_traits_3<cgal_kernel_type> Traits;

#if 0
 	CGAL::sibson_gradient_fitting(
		triangulation,
		std::inserter( map_point_3_to_value_grad, map_point_3_to_value_grad.begin()),
		cgal_point_3_value_access_type(map_point_3_to_value_func),
		Traits
	);

  //Sibson interpolant: version without sqrt:
  std::pair<Coord_type, bool> res =
    CGAL::sibson_c1_interpolation_square
    (coords.begin(),
     coords.end(),norm,p,
     CGAL::Data_access<cgal_map_point_3_to_value_type>(function_values),
     CGAL::Data_access<cgal_map_point_3_to_vector_type>(function_gradients),
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



double
GPlatesAppLogic::CgalUtils::interpolate_triangulation_3(
		const interpolate_triangulation_query_3_type &point_in_triangulation_query_3,
		const cgal_map_point_3_to_value_type &map_point_3_to_value) // function values
{
	const cgal_point_coordinate_vector_3_type &coords = point_in_triangulation_query_3.first;
	const cgal_coord_type &norm = point_in_triangulation_query_3.second;


	// Interpolate the mapped value.
	const cgal_coord_type interpolated_value = CGAL::linear_interpolation(
			coords.begin(), 
			coords.end(), 
			norm, 
			cgal_point_3_value_access_type(map_point_3_to_value) );

	return double(interpolated_value);
}


//
// END 
//

