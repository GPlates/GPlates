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

#include "ResolvedTriangulationDelaunay2.h"

#include "ResolvedTriangulationUtils.h"

#include "utils/Profile.h"


bool
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::calc_natural_neighbor_coordinates(
		delaunay_natural_neighbor_coordinates_2_type &natural_neighbor_coordinates,
		const delaunay_point_2_type &point,
		Face_handle start_face_hint) const
{
	// Build the natural neighbor coordinates.
	CGAL::Triple< std::back_insert_iterator<delaunay_point_coordinate_vector_2_type>, delaunay_coord_2_type, bool>
			coordinate_result =
					CGAL::natural_neighbor_coordinates_2(
							*this,
							point,
							std::back_inserter(natural_neighbor_coordinates.first),
							start_face_hint);

	natural_neighbor_coordinates.second = coordinate_result.second/*norm*/;

	if (!coordinate_result.third/*in_triangulation*/)
	{
		return false;
	}

	if (natural_neighbor_coordinates.second > 0)
	{
		// Normalisation factor greater than zero - everything's fine.
		return true/*in_triangulation*/;
	}

	//
	// It appears that CGAL can trigger assertions under certain situations (at certain query points).
	// This is most likely due to us not using exact arithmetic in our delaunay triangulation
	// (currently we use 'CGAL::Exact_predicates_inexact_constructions_kernel').
	// The assertion seems to manifest as a normalisation factor of zero - so we checked for zero above.
	//
	// We get around this by converting barycentric coordinates to natural neighbour coordinates.
	//

	delaunay_coord_2_type barycentric_coord_vertex_1;
	delaunay_coord_2_type barycentric_coord_vertex_2;
	delaunay_coord_2_type barycentric_coord_vertex_3;
	const boost::optional<Face_handle> face = calc_barycentric_coordinates(
			barycentric_coord_vertex_1,
			barycentric_coord_vertex_2,
			barycentric_coord_vertex_3,
			point,
			start_face_hint);
	if (!face)
	{
		return false/*in_triangulation*/;
	}

	natural_neighbor_coordinates.first.clear();
	natural_neighbor_coordinates.first.push_back(
			std::make_pair(face.get()->vertex(0)->point(), barycentric_coord_vertex_1));
	natural_neighbor_coordinates.first.push_back(
			std::make_pair(face.get()->vertex(1)->point(), barycentric_coord_vertex_2));
	natural_neighbor_coordinates.first.push_back(
			std::make_pair(face.get()->vertex(2)->point(), barycentric_coord_vertex_3));

	natural_neighbor_coordinates.second = 1.0;

	return true/*in_triangulation*/;
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::calc_barycentric_coordinates(
		delaunay_coord_2_type &barycentric_coord_vertex_1,
		delaunay_coord_2_type &barycentric_coord_vertex_2,
		delaunay_coord_2_type &barycentric_coord_vertex_3,
		const delaunay_point_2_type &point,
		Face_handle start_face_hint) const
{
	// Locate the (finite) face that the point is inside (if any).
	const boost::optional<Face_handle> face_opt = get_face_containing_point(point, start_face_hint);
	if (!face_opt)
	{
		return boost::none;
	}
	const Face_handle face = face_opt.get();

	delaunay_coord_2_type barycentric_norm;
	ResolvedTriangulation::get_barycentric_coords_2(
			point,
			face->vertex(0)->point(),
			face->vertex(1)->point(),
			face->vertex(2)->point(),
			barycentric_norm,
			barycentric_coord_vertex_1,
			barycentric_coord_vertex_2,
			barycentric_coord_vertex_3);

	return face;
}


boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::get_face_containing_point(
		const delaunay_point_2_type &point,
		Face_handle start_face_hint) const
{
	// Locate the (finite) face that the point is inside (if any).
	Locate_type locate_type;
	int li;
	const Face_handle face = locate(point, locate_type, li, start_face_hint);

	if (locate_type != FACE &&
		locate_type != EDGE &&
		locate_type != VERTEX)
	{
		// Point was not inside the convex hull (delaunay triangulation).
		return boost::none;
	}

	return face;
}


GPlatesAppLogic::ResolvedTriangulation::delaunay_vector_2_type
GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::gradient_2(
		const delaunay_point_2_type &point,
		const delaunay_map_point_to_value_2_type &function_values) const
{
	typedef CGAL::Interpolation_gradient_fitting_traits_2<delaunay_kernel_2_type> Traits;

	// coordinate comp.
	delaunay_point_coordinate_vector_2_type coords;

	CGAL::Triple< std::back_insert_iterator<delaunay_point_coordinate_vector_2_type>, delaunay_coord_2_type, bool>
		coordinate_result =
			CGAL::natural_neighbor_coordinates_2(
				*this,
				point,
				std::back_inserter(coords)
			);

	const delaunay_coord_2_type &norm = coordinate_result.second;

	// Gradient Fitting
	delaunay_vector_2_type gradient_vector;

	gradient_vector =
		CGAL::sibson_gradient_fitting(
			coords.begin(),
			coords.end(),
			norm,
			point,
			CGAL::Data_access<delaunay_map_point_to_value_2_type>(function_values),
			Traits()
		);

// FIXME: test code ; remove when no longer needed
#if 0
// estimates the function gradients at all vertices of triangulation 
// that lie inside the convex hull using the coordinates computed 
// by the function natural_neighbor_coordinates_2.

	sibson_gradient_fitting_nn_2(
		*this,
		std::inserter( 
			function_gradients, 
			function_gradients.begin() ),
		CGAL::Data_access<map_point_2_to_value_type>(function_values),
		Traits()
	);
#endif

#if 0

	//Sibson interpolant: version without sqrt
	std::pair<coord_type, bool> res =
		CGAL::sibson_c1_interpolation_square(
			coords.begin(),
     		coords.end(),
			norm,
			point,
			CGAL::Data_access<map_point_2_to_value_type>(function_values),
			CGAL::Data_access<map_point_2_to_vector_2_type>(function_gradients),
			Traits());

if(res.second)
    std::cout << "   Tested interpolation on " << point
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
