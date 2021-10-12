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
GPlatesAppLogic::CgalUtils::convert_point_to_cgal(
		const GPlatesMaths::PointOnSphere &point)
{
	// Create a 2D point from the point on sphere.
	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
	return cgal_point_2_type(llp.longitude(), llp.latitude());
}


GPlatesMaths::PointOnSphere
GPlatesAppLogic::CgalUtils::convert_point_from_cgal(
		const cgal_point_2_type &point)
{
	// Create a 3D point on sphere from a 2D point.
	const GPlatesMaths::LatLonPoint llp(point.y(), point.x());
	return GPlatesMaths::make_point_on_sphere(llp);
}


boost::optional<GPlatesAppLogic::CgalUtils::interpolate_triangulation_query_type>
GPlatesAppLogic::CgalUtils::query_interpolate_triangulation(
		const cgal_point_2_type &point,
		const cgal_delaunay_triangulation_type &triangulation)
{
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


double
GPlatesAppLogic::CgalUtils::interpolate_triangulation(
		const interpolate_triangulation_query_type &point_in_triangulation_query,
		const cgal_map_point_to_value_type &map_point_to_value)
{
	const cgal_point_coordinate_vector_type &coords = point_in_triangulation_query.first;
	const cgal_coord_type &norm = point_in_triangulation_query.second;

	// Interpolate the mapped value.
	const cgal_coord_type interpolated_value = CGAL::linear_interpolation(
			coords.begin(), coords.end(), 
			norm, 
			cgal_value_access_type(map_point_to_value));

	return double(interpolated_value);
}
