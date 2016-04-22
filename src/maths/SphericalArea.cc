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
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "Rotation.h"
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


		/**
		 * Calculates the *signed* spherical area of the spherical triangle bounded by the specified edges.
		 *
		 * Note that the edges must connect end-to-end in a loop - in other words the end point of
		 * @a first_edge must be the start point of @a second_edge, etc, ... the end point of
		 * @a third_edge must be the start point of @a first_edge.
		 *
		 * If any of the edges are zero length then the area returned will be zero.
		 */
		real_t
		calculate_spherical_triangle_signed_area(
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
	}
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_signed_area(
		const PolygonOnSphere &polygon)
{
	// Calculate a rough centroid of the polygon.
	const PointOnSphere polygon_centroid(polygon.get_boundary_centroid());

	//
	// Form triangles using the polygon centroid and each edge of the polygon (including exterior
	// and interior rings) and sum the signed area of these spherical triangles (with the signed area
	// of interior rings forced to have opposite sign than the exterior ring as detailed below).
	//
	// Form the edges of a spherical triangle that:
	// (1) starts at the polygon centroid,
	// (2) moves to the current polygon edge start point,
	// (3) moves to the current polygon edge end point,
	// (4) moves back to the polygon centroid,
	// thus forming a continuous loop.
	//

	real_t exterior_ring_signed_area = 0;

	// Calculate signed area of exterior ring.
	PolygonOnSphere::ring_const_iterator exterior_ring_edges_iter = polygon.exterior_ring_begin();
	const PolygonOnSphere::ring_const_iterator exterior_ring_edges_end = polygon.exterior_ring_end();
	for ( ; exterior_ring_edges_iter != exterior_ring_edges_end; ++exterior_ring_edges_iter)
	{
		const GreatCircleArc &edge = *exterior_ring_edges_iter;
		exterior_ring_signed_area += calculate_spherical_triangle_signed_area(polygon_centroid, edge);
	}

	real_t total_signed_area = exterior_ring_signed_area;

	// Calculate signed area of interior rings.
	const unsigned int num_interior_rings = polygon.number_of_interior_rings();
	for (unsigned int interior_ring_index = 0;
		interior_ring_index < num_interior_rings;
		++interior_ring_index)
	{
		real_t interior_ring_signed_area = 0;

		PolygonOnSphere::ring_const_iterator interior_ring_edges_iter = polygon.interior_ring_begin(interior_ring_index);
		const PolygonOnSphere::ring_const_iterator interior_ring_edges_end = polygon.interior_ring_end(interior_ring_index);
		for ( ; interior_ring_edges_iter != interior_ring_edges_end; ++interior_ring_edges_iter)
		{
			const GreatCircleArc &edge = *interior_ring_edges_iter;
			interior_ring_signed_area += calculate_spherical_triangle_signed_area(polygon_centroid, edge);
		}

		// Force the interior ring areas to have the opposite sign of the exterior area.
		// This way the interior rings reduce the absolute area of the exterior ring because
		// they are holes in the polygon. We need to do this since we don't know the orientation
		// of the interior rings (ie, we never forced them to have the opposite orientation of
		// the exterior ring like some software does).
		if (exterior_ring_signed_area.is_precisely_greater_than(0))
		{
			total_signed_area -= abs(interior_ring_signed_area);
		}
		else // exterior_ring_signed_area is negative...
		{
			total_signed_area += abs(interior_ring_signed_area);
		}
	}

	return total_signed_area;
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_exterior_ring_signed_area(
		const PolygonOnSphere &polygon)
{
	// Calculate a rough centroid of the polygon.
	const PointOnSphere polygon_centroid(polygon.get_boundary_centroid());

	//
	// Form triangles using the polygon centroid and each edge of the polygon exterior
	// and sum the signed area of these spherical triangles.
	//
	// Form the edges of a spherical triangle that:
	// (1) starts at the polygon centroid,
	// (2) moves to the current polygon edge start point,
	// (3) moves to the current polygon edge end point,
	// (4) moves back to the polygon centroid,
	// thus forming a continuous loop.
	//

	real_t ring_signed_area = 0;

	// Calculate signed area of exterior ring.
	PolygonOnSphere::ring_const_iterator ring_edges_iter = polygon.exterior_ring_begin();
	const PolygonOnSphere::ring_const_iterator ring_edges_end = polygon.exterior_ring_end();
	for ( ; ring_edges_iter != ring_edges_end; ++ring_edges_iter)
	{
		const GreatCircleArc &edge = *ring_edges_iter;
		ring_signed_area += calculate_spherical_triangle_signed_area(polygon_centroid, edge);
	}

	return ring_signed_area;
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_interior_ring_signed_area(
		const PolygonOnSphere &polygon,
		unsigned int interior_ring_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			interior_ring_index < polygon.number_of_interior_rings(),
			GPLATES_ASSERTION_SOURCE);

	// Calculate a rough centroid of the polygon.
	const PointOnSphere polygon_centroid(polygon.get_boundary_centroid());

	//
	// Form triangles using the polygon centroid and each edge of the polygon interior
	// and sum the signed area of these spherical triangles.
	//
	// Form the edges of a spherical triangle that:
	// (1) starts at the polygon centroid,
	// (2) moves to the current polygon edge start point,
	// (3) moves to the current polygon edge end point,
	// (4) moves back to the polygon centroid,
	// thus forming a continuous loop.
	//

	real_t ring_signed_area = 0;

	// Calculate signed area of interior ring.
	PolygonOnSphere::ring_const_iterator ring_edges_iter = polygon.interior_ring_begin(interior_ring_index);
	const PolygonOnSphere::ring_const_iterator ring_edges_end = polygon.interior_ring_end(interior_ring_index);
	for ( ; ring_edges_iter != ring_edges_end; ++ring_edges_iter)
	{
		const GreatCircleArc &edge = *ring_edges_iter;
		ring_signed_area += calculate_spherical_triangle_signed_area(polygon_centroid, edge);
	}

	return ring_signed_area;
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_spherical_triangle_signed_area(
		const PointOnSphere &point,
		const GreatCircleArc &edge)
{
	const GreatCircleArc::ConstructionParameterValidity point_to_edge_start_validity =
			GreatCircleArc::evaluate_construction_parameter_validity(
					point,
					edge.start_point());
	const GreatCircleArc::ConstructionParameterValidity edge_end_to_point_validity =
			GreatCircleArc::evaluate_construction_parameter_validity(
					edge.end_point(),
					point);

	// Detect and handle case where an arc end point is antipodal with respect to the point.
	if (point_to_edge_start_validity != GreatCircleArc::VALID ||
		edge_end_to_point_validity != GreatCircleArc::VALID)
	{
		// If edge is zero length then both edge end points are antipodal to the
		// point, but the triangle area will be zero.
		if (edge.is_zero_length())
		{
			return 0;
		}

		// Rotate the point slightly so that's it's no longer antipodal.
		// This will introduce a small error to the spherical triangle area though.
		// An angle of 1e-4 radians equates to a dot product (cosine) deviation of 5e-9 which is
		// less than the 1e-12 epsilon used in dot product to determine if two points are antipodal.
		// So this should be enough to prevent the same thing happening again.
		// Note that it's still possible that the *other* edge end point is now antipodal
		// to the rotated point - to avoid this we rotate the point *away* from the
		// antipodal edge such that it cannot lie on the antipodal arc.
		const Rotation point_rotation =
				Rotation::create(
						edge.rotation_axis(),
						(point_to_edge_start_validity == GreatCircleArc::VALID) ? 1e-4 : -1e-4);
		const PointOnSphere rotated_point(point_rotation * point);

		const GreatCircleArc point_to_edge_start = GreatCircleArc::create(rotated_point, edge.start_point());

		const GreatCircleArc edge_end_to_point = GreatCircleArc::create(edge.end_point(), rotated_point);

		// Returns zero area if any triangles edges are zero length...
		return GPlatesMaths::calculate_spherical_triangle_signed_area(
				point_to_edge_start,
				edge,
				edge_end_to_point);
	}

	const GreatCircleArc point_to_edge_start =
			GreatCircleArc::create(
					point,
					edge.start_point(),
					// No need to check construction parameter validity - we've already done so...
					false/*check_validity*/);

	const GreatCircleArc edge_end_to_point =
			GreatCircleArc::create(
					edge.end_point(),
					point,
					// No need to check construction parameter validity - we've already done so...
					false/*check_validity*/);

	// Returns zero area if any triangles edges are zero length...
	return GPlatesMaths::calculate_spherical_triangle_signed_area(
			point_to_edge_start,
			edge,
			edge_end_to_point);
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_spherical_triangle_signed_area(
				const PointOnSphere &first_point,
				const PointOnSphere &second_point,
				const PointOnSphere &third_point)
{
	//
	// Look for a two points that are not antipodal (so we can create a great circle arc).
	//

	const GreatCircleArc::ConstructionParameterValidity first_to_second_validity =
			GreatCircleArc::evaluate_construction_parameter_validity(first_point, second_point);
	if (first_to_second_validity == GreatCircleArc::VALID)
	{
		return calculate_spherical_triangle_signed_area(
				third_point,
				GreatCircleArc::create(
						first_point,
						second_point,
						// No need to check construction parameter validity - we've already done so...
						false/*check_validity*/));
	}

	const GreatCircleArc::ConstructionParameterValidity second_to_third_validity =
			GreatCircleArc::evaluate_construction_parameter_validity(second_point, third_point);
	if (second_to_third_validity == GreatCircleArc::VALID)
	{
		return calculate_spherical_triangle_signed_area(
				first_point,
				GreatCircleArc::create(
						second_point,
						third_point,
						// No need to check construction parameter validity - we've already done so...
						false/*check_validity*/));
	}

	const GreatCircleArc::ConstructionParameterValidity third_to_first_validity =
			GreatCircleArc::evaluate_construction_parameter_validity(third_point, first_point);
	if (third_to_first_validity == GreatCircleArc::VALID)
	{
		return calculate_spherical_triangle_signed_area(
				second_point,
				GreatCircleArc::create(
						third_point,
						first_point,
						// No need to check construction parameter validity - we've already done so...
						false/*check_validity*/));
	}

	// If all three points are antipodal then it means two points are coincident and a third point
	// is antipodal to the first two points.
	return 0;
}
