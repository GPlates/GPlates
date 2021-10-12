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

#include "PolygonOrientation.h"

#include "PolygonOnSphere.h"
#include "Vector3D.h"


namespace
{
	/**
	 * Projects a unit vector point onto the plane whose normal is @a plane_normal and
	 * returns normalised version of projected point.
	 */
	GPlatesMaths::UnitVector3D
	get_orthonormal_vector(
			const GPlatesMaths::UnitVector3D &point,
			const GPlatesMaths::UnitVector3D &plane_normal)
	{
		// The projection of 'point' in the direction of 'plane_normal'.
		GPlatesMaths::Vector3D proj = dot(point, plane_normal) * plane_normal;

		// The projection of 'point' perpendicular to the direction of
		// 'plane_normal'.
		return (GPlatesMaths::Vector3D(point) - proj).get_normalisation();
	}
}


GPlatesMaths::PolygonOrientation::Orientation
GPlatesMaths::PolygonOrientation::calculate_polygon_orientation(
		const PolygonOnSphere &polygon)
{
	// Iterate through the polygon vertices and calculate the sum of vertex positions.
	GPlatesMaths::Vector3D summed_vertex_position(0,0,0);
	int num_vertices = 0;
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator vertex_iter = polygon.vertex_begin();
	const GPlatesMaths::PolygonOnSphere::vertex_const_iterator vertex_end = polygon.vertex_end();
	for ( ; vertex_iter != vertex_end; ++vertex_iter, ++num_vertices)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::Vector3D point(vertex.position_vector());
		summed_vertex_position = summed_vertex_position + point;
	}

	// If the magnitude of the summed vertex position is zero then all the points averaged
	// to zero and hence we cannot get a plane normal to project onto.
	// This most likely happens when the vertices roughly form a great circle arc and hence
	// then are two possible projection directions and hence you could assign the orientation
	// to be either clockwise or counter-clockwise.
	// If this happens we'll just choose one orientation arbitrarily.
	if (summed_vertex_position.magSqrd() <= 0.0)
	{
		// Return arbitrary orientation.
		return CLOCKWISE;
	}

	// Calculate a unit vector from the sum to use as our plane normal.
	const GPlatesMaths::UnitVector3D proj_plane_normal =
			summed_vertex_position.get_normalisation();

	// First try starting with the global x axis - if it's too close to the plane normal
	// then choose the global y axis.
	GPlatesMaths::UnitVector3D proj_plane_x_axis_test_point(0, 0, 1); // global x-axis
	if (dot(proj_plane_x_axis_test_point, proj_plane_normal) > 1 - 1e-2)
	{
		proj_plane_x_axis_test_point = GPlatesMaths::UnitVector3D(0, 1, 0); // global y-axis
	}
	const GPlatesMaths::UnitVector3D proj_plane_axis_x = get_orthonormal_vector(
			proj_plane_x_axis_test_point, proj_plane_normal);

	// Determine the y axis of the plane.
	const GPlatesMaths::UnitVector3D proj_plane_axis_y(
			cross(proj_plane_normal, proj_plane_axis_x));

	// Iterate through the vertices again and project onto the plane and
	// also calculate the signed area of the projected polygon.
	GPlatesMaths::real_t signed_poly_area = 0;
	GPlatesMaths::real_t last_proj_point_x = 0;
	GPlatesMaths::real_t last_proj_point_y = 0;
	vertex_iter = polygon.vertex_begin();
	if (vertex_iter != vertex_end)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::UnitVector3D &point = vertex.position_vector();

		last_proj_point_x = dot(proj_plane_axis_x, point);
		last_proj_point_y = dot(proj_plane_axis_y, point);
		++vertex_iter;
	}
	for ( ; vertex_iter != vertex_end; ++vertex_iter)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::UnitVector3D &point = vertex.position_vector();

		const GPlatesMaths::real_t proj_point_x = dot(proj_plane_axis_x, point);
		const GPlatesMaths::real_t proj_point_y = dot(proj_plane_axis_y, point);

		signed_poly_area +=
				last_proj_point_x * proj_point_y -
				last_proj_point_y * proj_point_x;

		last_proj_point_x = proj_point_x;
		last_proj_point_y = proj_point_y;
	}

	return (signed_poly_area < 0) ? CLOCKWISE : COUNTERCLOCKWISE;
}
