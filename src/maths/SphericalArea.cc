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
#include "PointInPolygon.h"
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
			//         the point-on-sphere joining the two edges (first edge end or second edge start).
			//
			// We do (1) because if both edges are parallel (pointing in same direction) then we
			// want an angle of +/-PI (not zero) and anti-parallel edges (pointing in different directions)
			// to have an angle of zero (not +/-PI).
			// We do (2) so that clockwise turning (following from first edge to second edge) gives a
			// negative angle (and counter-clockwise a positive angle).
			const double angle =
					(dot(cross_product_normals, second_edge.start_point().position_vector()).dval() < 0)
					? std::atan2(-cross_product_magnitude,  -dot_product_normals)
					: std::atan2(cross_product_magnitude,  -dot_product_normals);

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
			//   Area = Sum(internal angles) - PI;  for counter-clockwise triangle (sum angles positive)
			//   Area = Sum(internal angles) + PI;  for clockwise triangle (sum angles negative)
			const double signed_area = (sum_internal_angles > 0)
					? sum_internal_angles - PI
					: sum_internal_angles + PI;

			return signed_area;
		}
	}
}


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Calculates the *signed* spherical area of the interior of a polygon ring (see
		 * PointInPolygon::InteriorReference).
		 *
		 * A ring within the open hemisphere about the boundary centroid C (its interior is parity
		 * from -C) has the fan sum from apex C, as before the interior rule. Any other ring has the fan
		 * sum from the antipode of the polygon's outside point, less 4 pi times its winding offset
		 * (non-zero when the outside point is in the ring's interior): the same fan sum, and the same
		 * rounding, as decided whether the outside point is outside.
		 */
		real_t
		calculate_polygon_ring_signed_area(
				const PolygonOnSphere &polygon,
				bool is_ring_in_boundary_centroid_hemisphere,
				const PolygonOnSphere::ring_const_iterator &ring_begin,
				const PolygonOnSphere::ring_const_iterator &ring_end)
		{
			if (is_ring_in_boundary_centroid_hemisphere)
			{
				return SphericalArea::calculate_polygon_ring_fan_signed_area(
						PointOnSphere(polygon.get_boundary_centroid()),
						ring_begin,
						ring_end);
			}

			const real_t ring_fan_signed_area = SphericalArea::calculate_polygon_ring_fan_signed_area(
					PointOnSphere(-polygon.get_interior_reference().get_outside_point()),
					ring_begin,
					ring_end);

			return ring_fan_signed_area -
					4 * PI * PointInPolygon::get_ring_winding_offset(ring_fan_signed_area.dval());
		}
	}
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_signed_area(
		const PolygonOnSphere &polygon)
{
	const PointInPolygon::InteriorReference &interior_reference = polygon.get_interior_reference();

	const real_t exterior_ring_signed_area = calculate_polygon_ring_signed_area(
			polygon,
			interior_reference.is_exterior_ring_in_boundary_centroid_hemisphere(),
			polygon.exterior_ring_begin(),
			polygon.exterior_ring_end());

	real_t total_signed_area = exterior_ring_signed_area;

	// Calculate signed area of interior rings.
	const unsigned int num_interior_rings = polygon.number_of_interior_rings();
	for (unsigned int interior_ring_index = 0;
		interior_ring_index < num_interior_rings;
		++interior_ring_index)
	{
		const real_t interior_ring_signed_area = calculate_polygon_ring_signed_area(
				polygon,
				interior_reference.is_interior_ring_in_boundary_centroid_hemisphere(interior_ring_index),
				polygon.interior_ring_begin(interior_ring_index),
				polygon.interior_ring_end(interior_ring_index));

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
	return calculate_polygon_ring_signed_area(
			polygon,
			polygon.get_interior_reference().is_exterior_ring_in_boundary_centroid_hemisphere(),
			polygon.exterior_ring_begin(),
			polygon.exterior_ring_end());
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_interior_ring_signed_area(
		const PolygonOnSphere &polygon,
		unsigned int interior_ring_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			interior_ring_index < polygon.number_of_interior_rings(),
			GPLATES_ASSERTION_SOURCE);

	return calculate_polygon_ring_signed_area(
			polygon,
			polygon.get_interior_reference().is_interior_ring_in_boundary_centroid_hemisphere(
					interior_ring_index),
			polygon.interior_ring_begin(interior_ring_index),
			polygon.interior_ring_end(interior_ring_index));
}


GPlatesMaths::real_t
GPlatesMaths::SphericalArea::calculate_polygon_ring_fan_signed_area(
		const PointOnSphere &apex,
		const PolygonOnSphere::ring_const_iterator &ring_begin,
		const PolygonOnSphere::ring_const_iterator &ring_end)
{
	//
	// Form triangles using the apex and each edge of the ring and sum the signed area of these
	// spherical triangles.
	//
	// Form the edges of a spherical triangle that:
	// (1) starts at the apex,
	// (2) moves to the current ring edge start point,
	// (3) moves to the current ring edge end point,
	// (4) moves back to the apex,
	// thus forming a continuous loop.
	//

	real_t ring_signed_area = 0;

	PolygonOnSphere::ring_const_iterator ring_edges_iter = ring_begin;
	for ( ; ring_edges_iter != ring_end; ++ring_edges_iter)
	{
		const GreatCircleArc &edge = *ring_edges_iter;
		ring_signed_area += calculate_spherical_triangle_signed_area(apex, edge);
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
		// An angle of 1e-5 radians equates to a dot product (cosine) deviation of 5e-11 which is
		// larger than the 1e-12 epsilon used in dot product to determine if two points are antipodal.
		// So this should be enough to prevent the same thing happening again.
		const Rotation point_rotation =
				Rotation::create(
						// Note that instead of using the edge rotation axis to rotate in the plane of the edge,
						// we rotate perpendicularly off the edge. This ensures that the rotated point
						// does not now become antipodal to the other edge point. It also avoids a complicated
						// situation where a polygon around the equator ends up with an area of zero instead of
						// 2*PI (ie, half the globe) due to rotating in such a way that all the spherical triangles
						// making up that equator polygon end up with zero area. Moving *off* the edge great circle
						// avoids this issue.
						cross(edge.rotation_axis(), point.position_vector()).get_normalisation(),
						1e-5);
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
