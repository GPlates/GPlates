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


namespace
{
#if !defined(__WINDOWS__)
	// The old sigaction for SIGSEGV before we called sigaction().
	struct sigaction old_action;

	// Holds the information necessary for setjmp/longjmp to work.
	jmp_buf buf;


	/**
 	* Handles the SIGSEGV signal.
 	*/
	void
	handler(int signum)
	{
		// Jump past the problem call to GDALOpen.
		/* non-zero value so that the GDALOpen call doesn't get called again */
		//qDebug() << "handler";
		longjmp( buf, 1);
	}
#endif
}


		void
		GPlatesAppLogic::CgalUtils::do_insert(
			cgal_point_2_type point,
			cgal_constrained_delaunay_triangulation_2_type &constrained_delaunay_triangulation_2,
			std::vector<cgal_constrained_vertex_handle> &vertex_handles)
		{
#if !defined(__WINDOWS__)
			// Set a handler for SIGSEGV in case the call to GDALOpen does a segfault.
			struct sigaction action;
			std::memset(&action, 0, sizeof(action)); // sizeof(struct sigaction) is platform dependent.
			action.sa_handler = &handler;
			sigaction(SIGSEGV, &action, &old_action);
	
			// Try and call 
			if (!setjmp( buf ))
			{
#endif
				// The first time setjmp() is called, it returns 0. If call segfaults,
	
				// we longjmp back to the if statement, but with a non-zero value.
				//qDebug() << " call insert ";
				vertex_handles.push_back(
					 constrained_delaunay_triangulation_2.insert( point ) );
#if !defined(__WINDOWS__)
			}
			//qDebug() << " after insert ";

			// Restore the old sigaction whether or not we segfaulted.
			sigaction(SIGSEGV, &old_action, NULL);
#endif
		}


GPlatesAppLogic::CgalUtils::cgal_point_3_type
GPlatesAppLogic::CgalUtils::convert_point_on_sphere_to_cgal_3(
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
GPlatesAppLogic::CgalUtils::convert_cgal_3_to_point_on_sphere(
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
// Lambert Equal Area Projection
//
// http://mathworld.wolfram.com/LambertAzimuthalEqual-AreaProjection.html
//

GPlatesAppLogic::CgalUtils::cgal_point_2_type
GPlatesAppLogic::CgalUtils::project_lat_lon_point_to_azimuthal_equal_area(
		const GPlatesMaths::LatLonPoint &point,
		const GPlatesMaths::LatLonPoint &proj_center)
{
	using namespace std;
	double phi_0 = proj_center.latitude()  * GPlatesMaths::PI / 180.0; // center of projection lat
	double lam_0 = proj_center.longitude() * GPlatesMaths::PI / 180.0; // center of projection lon

	double phi	= point.latitude()  * GPlatesMaths::PI / 180.0; // Node latitude
	double lam 	= point.longitude() * GPlatesMaths::PI / 180.0; // Node longitude

	double k = sqrt( 2 / ( 1 + sin(phi_0) * sin(phi) + cos(phi_0) * cos(phi) * cos(lam-lam_0) ) );

	double x = k * cos(phi) * sin(lam-lam_0);
	double y = k * (cos(phi_0) * sin(phi) - sin(phi_0) * cos(phi) * cos(lam-lam_0) );

#ifdef DEBUG
qDebug() << "project_llp_to_azimuthal_equal_area: proj_center = " << proj_center;
qDebug() << "project_llp_to_azimuthal_equal_area: point = " << point;
qDebug() << "project_llp_to_azimuthal_equal_area: phi,lam = (" << phi << "," << lam << ")";
qDebug() << "project_llp_to_azimuthal_equal_area: x,y = (" << x << "," << y << ")";
#endif
	return cgal_point_2_type(x,y);
}


GPlatesAppLogic::CgalUtils::cgal_point_2_type
GPlatesAppLogic::CgalUtils::project_point_on_sphere_to_azimuthal_equal_area(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesMaths::LatLonPoint &proj_center)
{
	return project_lat_lon_point_to_azimuthal_equal_area(
			GPlatesMaths::make_lat_lon_point(point),
			proj_center);
}


GPlatesMaths::LatLonPoint
GPlatesAppLogic::CgalUtils::project_azimuthal_equal_area_to_lat_lon_point(
		const GPlatesAppLogic::CgalUtils::cgal_point_2_type &point,
		const GPlatesMaths::LatLonPoint &proj_center)
{
	using namespace std;
	double phi_0 = proj_center.latitude()  * GPlatesMaths::PI / 180.0; // center of projection lat
	double lam_0 = proj_center.longitude() * GPlatesMaths::PI / 180.0; // center of projection lon

	double x = point.x();
	double y = point.y();

	double rho = sqrt( x*x + y*y);
	double a = rho / 2.0;
	double c = 2*asin( a );

#ifdef DEBUG
qDebug() << "project_azimuthal_equal_area_to_llp: rho = " << rho;
qDebug() << "project_azimuthal_equal_area_to_llp: a = " << a;
qDebug() << "project_azimuthal_equal_area_to_llp: c = " << c;
#endif

	double phi = asin( cos(c)*sin(phi_0) + y*sin(c)*cos(phi_0)/rho ); // Latitude in Radians
	double lam; // Longitude in radians

	if (phi_0 == GPlatesMaths::PI / 2)
	{
		lam = lam_0 + atan2(x,-y); 
	}
	else if (phi_0 == -GPlatesMaths::PI / 2)
	{
		lam = lam_0 + atan2(x,y); 
	}
	else
	{
		lam = lam_0 + atan2( x*sin(c) , rho*cos(phi_0)*cos(c) - y*sin(phi_0)*sin(c) ); 
	}

	double lat = phi * 180.0 / GPlatesMaths::PI;
	double lon = lam * 180.0 / GPlatesMaths::PI;

#ifdef DEBUG
qDebug() << "project_azimuthal_equal_area_to_llp: proj_center = " << proj_center;
qDebug() << "project_azimuthal_equal_area_to_llp: x,y = (" << x << "," << y << ")";
qDebug() << "project_azimuthal_equal_area_to_llp: phi,lam = (" << phi << "," << lam << ")";
qDebug() << "project_azimuthal_equal_area_to_llp: lat,lon = (" << lat << "," << lon << ")";
#endif

	GPlatesMaths::LatLonPoint llp = GPlatesMaths::LatLonPoint(lat,lon);
	return llp;
}


GPlatesMaths::PointOnSphere
GPlatesAppLogic::CgalUtils::project_azimuthal_equal_area_to_point_on_sphere(
		const GPlatesAppLogic::CgalUtils::cgal_point_2_type &point,
		const GPlatesMaths::LatLonPoint &proj_center)
{
	return GPlatesMaths::make_point_on_sphere(
			project_azimuthal_equal_area_to_lat_lon_point(point, proj_center));
}


//GPlatesAppLogic::CgalUtils::cgal_cdt_2_locate_type
bool
GPlatesAppLogic::CgalUtils::is_point_in_mesh(
	const cgal_point_2_type &point,
	const cgal_delaunay_triangulation_2_type &dt2,
	const cgal_constrained_delaunay_triangulation_2_type &cdt2)
{

	GPlatesAppLogic::CgalUtils::cgal_cdt_2_locate_type locate_type;

	GPlatesAppLogic::CgalUtils::cgal_cdt_2_face_handle start_face = cgal_cdt_2_face_handle();
	GPlatesAppLogic::CgalUtils::cgal_cdt_2_face_handle found_face;
	int number;

	found_face = cdt2.locate(point, locate_type, number, start_face);

	//qDebug() << "locate_point: number = " << number;
	//qDebug() << "locate_point: locate_type = " << locate_type;

 	switch (locate_type)
	{
		case cgal_constrained_delaunay_triangulation_2_type::VERTEX:
			//qDebug() << "locate_point: VERTEX";
			break;
		case cgal_constrained_delaunay_triangulation_2_type::FACE:
			if ( found_face->is_in_domain() )
			{
				//qDebug() << "locate_point: FACE: in Mesh domain";
				return true;
			} else {
				//qDebug() << "locate_point: FACE: not in Mesh domain";
			}
			break;
		case cgal_constrained_delaunay_triangulation_2_type::EDGE:
			//qDebug() << "locate_point: EDGE";
			break;
		case cgal_constrained_delaunay_triangulation_2_type::OUTSIDE_CONVEX_HULL:
			//qDebug() << "locate_point: OUTSIDE_CONVEX_HULL";
			break;
		case cgal_constrained_delaunay_triangulation_2_type::OUTSIDE_AFFINE_HULL:
			//qDebug() << "locate_point: OUTSIDE_AFFINE_HULL";
			break;
		default:
			qDebug() << "???";
	} 
	
	//return cgal_constrained_delaunay_triangulation_2_type::VERTEX;
	return false;
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

	const bool in_network = coordinate_result.third;

	if (!in_network)
	{
		return boost::none;
	}

	const cgal_coord_type &norm = coordinate_result.second;

	return std::make_pair(coords, norm);
}

//
// 2D + C
//
boost::optional<GPlatesAppLogic::CgalUtils::interpolate_triangulation_query_type>
GPlatesAppLogic::CgalUtils::query_interpolate_constrained_triangulation_2(
		const cgal_point_2_type &point,
		const cgal_delaunay_triangulation_2_type &triangulation,
		const cgal_constrained_delaunay_triangulation_2_type &c_triangulation)
{
#ifdef HAVE_GCAL_PATCH_CDT2_H
qDebug() << "HAVE_GCAL_PATCH_CDT2_H";
	// NOTE: 
	// must use patched version of CGAL's Constrained_Delaunay_triangulation_2.h 
	// to use our cgal_constrained_delaunay_triangulation_2_type as the base triangulation
	//

	// Build the natural_neighbor_coordinates_2 from the cgal_delaunay_triangulation_2_type
	cgal_point_coordinate_vector_type c_coords;

	CGAL::Triple< std::back_insert_iterator<cgal_point_coordinate_vector_type>, cgal_kernel_type::FT, bool>
			c_coordinate_result =
					CGAL::natural_neighbor_coordinates_2(
							c_triangulation,
							point,
							std::back_inserter(c_coords));
	
	const bool in_network = c_coordinate_result.third;

	if (!in_network)
	{
		return boost::none;
	}

	const cgal_coord_type &norm = c_coordinate_result.second;

	return std::make_pair(c_coords, norm);
#else
	return boost::none;
#endif
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
		const cgal_constrained_delaunay_triangulation_2_type &c_triangulation,
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
		//qDebug() << " !in_network ";
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

