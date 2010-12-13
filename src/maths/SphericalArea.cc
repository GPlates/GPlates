/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <cmath>

#include "SphericalArea.h"

#include "GreatCircleArc.h"
#include "PolygonOnSphere.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Calculates the angle, in radians, between two adjacent great circle arcs.
		 *
		 * Note that both edges must *not* be zero-length (ie, they must each have a rotation axis).
		 *
		 * Note that @a second_edge must be after @a first_edge in the sequence of edges *and*
		 * only non-zero-length edges should be inbetween them.
		 */
		double
		calculate_angle_between_adjacent_non_zero_length_edges(
				const GreatCircleArc &first_edge,
				const GreatCircleArc &second_edge)
		{
			// The client has assured us the great circle arcs don't have zero length and
			// hence have a rotation axis.
			const UnitVector3D &first_edge_normal = first_edge.rotation_axis();
			const UnitVector3D &second_edge_normal = second_edge.rotation_axis();

			//
			// To obtain accurate results we combine the cosine and sine of the angle into
			// an arc-tangent - this gives us more accurate results than taking just
			// the arc-cosine because arguments (to arc-cosine) that are near zero can give
			// inaccurate results.
			//

			// The cosine of the angle is related to the dot product.
			const double dot_product_normals = dot(first_edge_normal, second_edge_normal).dval();

			// The sine of the angle is related to the magnitude of the cross product.
			const Vector3D cross_product_normals = cross(first_edge_normal, second_edge_normal);
			const double cross_product_magnitude = cross_product_normals.magnitude().dval();

			// We need to get the cosine and sine into the correct atan quadrant.
			// To do this we need to:
			// (1) Negate the dot product and,
			// (2) Flip the sign of the cross product magnitude if the cross product vector
			//     is pointing in the opposite direction to the vector
			//         from the origin (sphere centre) to
			//         the point-on-sphere joining the two edges.
			double angle =
					(dot(cross_product_normals, second_edge.start_point().position_vector()).dval() < 0)
					? std::atan2(-cross_product_magnitude,  -dot_product_normals)
					: std::atan2(cross_product_magnitude,  -dot_product_normals);

			// Convert range [-PI, PI] returned by atan2 to the range [0, 2PI].
			if (angle < 0)
			{
				angle += 2 * PI;
			}

			return angle;
		}
	}
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_signed_area(
		const PolygonOnSphere &polygon)
{
	//
	// Calculate a rough centroid of the polygon.
	//

	// Iterate through the points and calculate the sum of vertex positions.
	Vector3D summed_points_position(0,0,0);

	PolygonOnSphere::vertex_const_iterator polygon_points_iter = polygon.vertex_begin();
	const PolygonOnSphere::vertex_const_iterator polygon_points_end = polygon.vertex_end();
	for ( ; polygon_points_iter != polygon_points_end; ++polygon_points_iter)
	{
		const PointOnSphere &point = *polygon_points_iter;

		const Vector3D vertex(point.position_vector());
		summed_points_position = summed_points_position + vertex;
	}

	// If the magnitude of the summed vertex position is zero then all the points averaged
	// to zero and hence we cannot get a centroid point.
	// This most likely happens when the vertices roughly form a great circle arc.
	// If this happens then just pick one of the points on the polygon - it doesn't really
	// matter because we're just trying to shorten the lengths of the sides of the triangles
	// that fan out from one point (centroid) to each edge of the polygon.
	const PointOnSphere polygon_centroid =
			(summed_points_position.magSqrd() > 0.0)
			? PointOnSphere(summed_points_position.get_normalisation())
			: polygon.first_vertex();

	//
	// Form triangles using the polygon centroid and each edge of the polygon and
	// sum the signed area of these spherical triangles.
	//

	real_t total_signed_area = 0;

	PolygonOnSphere::const_iterator polygon_edges_iter = polygon.begin();
	const PolygonOnSphere::const_iterator polygon_edges_end = polygon.end();
	for ( ; polygon_edges_iter != polygon_edges_end; ++polygon_edges_iter)
	{
		// Form the edges of a spherical triangle that:
		// (1) starts at the polygon centroid,
		// (2) moves to the current polygon edge start point,
		// (3) moves to the current polygon edge end point,
		// (4) moves back to the polygon centroid,
		// thus forming a continuous loop.
		const GreatCircleArc &polygon_edge = *polygon_edges_iter;

		const GreatCircleArc centroid_to_edge_start =
				GreatCircleArc::create(polygon_centroid, polygon_edge.start_point());

		const GreatCircleArc edge_end_to_centroid =
				GreatCircleArc::create(polygon_edge.end_point(), polygon_centroid);

		total_signed_area +=
				calculate_spherical_triangle_signed_area(
						centroid_to_edge_start, polygon_edge, edge_end_to_centroid);
	}

	return total_signed_area;
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_spherical_triangle_signed_area(
		const GreatCircleArc &first_edge,
		const GreatCircleArc &second_edge,
		const GreatCircleArc &third_edge)
{
	// If any edge is zero length then the area is zero.
	if (first_edge.is_zero_length() ||
		second_edge.is_zero_length() ||
		third_edge.is_zero_length())
	{
		return 0;
	}

	// Calculate the sum of all the internal angles.
	const double sum_internal_angles =
			calculate_angle_between_adjacent_non_zero_length_edges(first_edge, second_edge) +
			calculate_angle_between_adjacent_non_zero_length_edges(second_edge, third_edge) +
			calculate_angle_between_adjacent_non_zero_length_edges(third_edge, first_edge);

	// The area of a spherical triangle, on unit sphere, is:
	//   Area = Sum(internal angles) - PI
	double signed_area = sum_internal_angles - PI;

	// If the area is greater than 2 * PI then the spherical triangle is clockwise when
	// viewed from above the surface of the sphere.
	// Calculate the complementary area *inside* the spherical triangle and
	// make it negative to indicate orientation.
	if (signed_area > 2 * PI)
	{
		signed_area = signed_area - 4*PI/*area of unit sphere*/;
	}

	return signed_area;
}
