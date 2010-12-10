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
			// (2) Flip the sign of the cross product magnitude relative to its alignment
			//     with the vector from the origin (sphere centre) to the point-on-sphere
			//     joining the two edges.
			if (dot(cross_product_normals, second_edge.start_point().position_vector()).dval() > 0)
			{
				return std::atan2(-cross_product_magnitude,  -dot_product_normals);
			}

			return std::atan2(cross_product_magnitude,  -dot_product_normals);
		}
	}
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_signed_area(
		const PolygonOnSphere &polygon)
{
	double sum_internal_angles = 0;
	unsigned int num_non_zero_edges = 0;

	// We need to deal with zero length edges - these are edges that have endpoints so
	// close together that a rotation axis (for the edge) could not be determined.
	// We'll deal with these edges by ignoring them - their contribution to the polygon
	// is so small anyway (at the limits of numerical precision).

	// Find the first non-zero-length edge.
	PolygonOnSphere::const_iterator edge_iter = polygon.begin();
	const PolygonOnSphere::const_iterator edge_end = polygon.end();
	for ( ; edge_iter != edge_end; ++edge_iter)
	{
		if (!edge_iter->is_zero_length())
		{
			++num_non_zero_edges;
			break;
		}
	}

	// If we were unable to find a non-zero-length edge then return zero area because
	// all the polygon points are so close together (within the limits of numerical precision).
	if (num_non_zero_edges == 0)
	{
		return 0;
	}

	// Keep track of the first non-zero-length edge so we can calculate the final internal angle.
	const GreatCircleArc &first_non_zero_length_edge = *edge_iter;

	// Calculate the internal angles between adjacent non-zero-length edges.
	const GreatCircleArc *previous_non_zero_length_edge = &first_non_zero_length_edge;
	for ( ++edge_iter; edge_iter != edge_end; ++edge_iter)
	{
		if (edge_iter->is_zero_length())
		{
			continue;
		}
		++num_non_zero_edges;

		const GreatCircleArc &current_non_zero_length_edge = *edge_iter;

		const double angle_between_adjacent_non_zero_length_edges =
				calculate_angle_between_adjacent_non_zero_length_edges(
						*previous_non_zero_length_edge, current_non_zero_length_edge);

		sum_internal_angles += angle_between_adjacent_non_zero_length_edges;

		// Update the previous edge to be the current edge.
		previous_non_zero_length_edge = &current_non_zero_length_edge;
	}

	// If there's less than three non-zero edges then the polygon looks like either
	// a point or a line, both of which have zero area.
	if (num_non_zero_edges < 3)
	{
		return 0;
	}

	// Handle the internal angle between the current non-zero-length edge and
	// the first non-zero-length edge.
	sum_internal_angles +=
			calculate_angle_between_adjacent_non_zero_length_edges(
					*previous_non_zero_length_edge, first_non_zero_length_edge);

	// The area of the polygon, on unit sphere, is:
	//   Area = Sum(internal angles) - (N-2) * PI
	// ...where N is number of vertices (or edges).
	double signed_area = sum_internal_angles - (num_non_zero_edges - 2) * PI;

	// If the area is greater than 2 * PI then the polygon is clockwise when viewed from above
	// the surface of the sphere.
	// Calculate the complementary area *inside* the polygon and make it negative to indicate
	// orientation.
	if (signed_area > 2 * PI)
	{
		signed_area = signed_area - 4*PI/*area of unit sphere*/;
	}

	return signed_area;
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
