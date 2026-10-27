/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "Centroid.h"

#include "GreatCircleArc.h"
#include "SmallCircleBounds.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	namespace Centroid
	{
		namespace Implementation
		{
			/**
			 * Calculate the sum of centroids of a sequence of spherical triangles formed by
			 * @a GreatCircleArc objects and a polygon centroid using an approximate area weighting of the spherical triangles.
			 *
			 * The sequence is assumed to be closed off (ie, the sequence of arcs forms a closed polygon).
			 *
			 * Note: If the ring has negative area (meaning the ring orientation is clockwise which
			 * means the triangle-area-weighted centroid will be on the opposite side of the globe
			 * from the ring) then we negate it to bring it onto the same side.
			 */
			Vector3D
			calculate_sum_area_weighted_centroids_in_polygon_ring(
					const PointOnSphere &polygon_centroid,
					const PolygonOnSphere::ring_const_iterator &ring_begin,
					const PolygonOnSphere::ring_const_iterator &ring_end)
			{
				// Iterate through the edges and calculate the area and centroid of each
				// triangle formed by the edge and the polygon centroid.
				// Iterate through the points and calculate the sum of vertex positions.
				Vector3D area_weighted_centroid(0,0,0);
				real_t total_area = 0;

				PolygonOnSphere::ring_const_iterator ring_iter = ring_begin;
				for ( ; ring_iter != ring_end; ++ring_iter)
				{
					const GreatCircleArc &edge = *ring_iter;

					// Returns zero area if any triangles edges are zero length...
					const real_t triangle_area =
							SphericalArea::calculate_spherical_triangle_signed_area(polygon_centroid, edge);

					const Vector3D triangle_centroid_sum =
									Vector3D(polygon_centroid.position_vector()) +
									Vector3D(edge.start_point().position_vector()) +
									Vector3D(edge.end_point().position_vector());

					// If the sum of the triangles points is the zero vector then all three points
					// must have been equally spaced on a great circle - in this case the triangle
					// area will be 2*PI (area of a hemisphere) hence we can't say it's clockwise or
					// counter-clockwise so we'll just pick the rotation axis of the edge
					// (since it's orthogonal to the great circle). This is extremely unlikely anyway.
					const UnitVector3D triangle_centroid = (triangle_centroid_sum.magSqrd() > 0)
							? triangle_centroid_sum.get_normalisation()
							: edge.rotation_axis();

					// Note that 'triangle_area' can be negative which means the triangle centroid
					// is subtracted instead of added.
					area_weighted_centroid = area_weighted_centroid + triangle_area * triangle_centroid;
					total_area += triangle_area;
				}

				// A negative ring total area means the ring orientation is clockwise which
				// means the triangle-area-weighted centroid will be on the opposite side of the globe
				// from the ring, so we negate it to bring it onto the same side.
				if (total_area.is_precisely_less_than(0))
				{
					area_weighted_centroid = -area_weighted_centroid;
				}

				return area_weighted_centroid;
			}


			/**
			 * Calculate the area moment (the integral of position over area) of the region to the left
			 * of a polygon ring, weighted by winding number.
			 *
			 * By Stokes' theorem this is half the sum, over the ring's edges, of each edge's arc length
			 * times its unit normal (rotation axis). It is exact, and needs no apex, but it loses
			 * precision on tiny rings: the terms (unit vectors scaled by arc lengths) round at about
			 * epsilon each, while the moment is about the square of the ring's width, so its direction
			 * errs by about epsilon over the width squared (measured: about 3e-7 radians for a ring
			 * 1e-5 radians across).
			 */
			Vector3D
			calculate_area_moment_of_polygon_ring(
					const PolygonOnSphere::ring_const_iterator &ring_begin,
					const PolygonOnSphere::ring_const_iterator &ring_end)
			{
				Vector3D area_moment(0, 0, 0);

				for (PolygonOnSphere::ring_const_iterator ring_iter = ring_begin;
					ring_iter != ring_end;
					++ring_iter)
				{
					const GreatCircleArc &edge = *ring_iter;
					if (edge.is_zero_length())
					{
						continue;
					}

					// The arc length from its sine and cosine (accurate for short arcs, unlike the cosine).
					const UnitVector3D &start = edge.start_point().position_vector();
					const UnitVector3D &end = edge.end_point().position_vector();
					const double arc_length = std::atan2(
							cross(start, end).magnitude().dval(),
							dot(start, end).dval());

					area_moment = area_moment + (0.5 * arc_length) * Vector3D(edge.rotation_axis());
				}

				return area_moment;
			}
		}
	}
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Centroid::Implementation::get_normalised_centroid_or_placeholder_centroid(
		const Vector3D &centroid,
		const UnitVector3D &placeholder_centroid)
{
	// If the magnitude of the centroid is zero then we cannot get a centroid point.
	// This most likely happens when the vertices roughly form a great circle arc.
	if (centroid.magSqrd() <= 0.0)
	{
		// Just return the placeholder centroid - this is obviously not very good but it
		// alleviates the caller from having to check an error code or catch an exception.
		// Also it's extremely unlikely to happen.
		// And even when 'centroid.magSqrd()' is *very* close to zero
		// but passes the test then the returned centroid is essentially random.
		// TODO: Implement a more robust alternative for those clients that require
		// an accurate centroid all the time - for most uses in GPlates the worst
		// that happens is a small circle bounding some geometry (with centroid used
		// as small circle centre) becomes larger than it would normally be resulting
		// in less efficient intersection tests.
		return placeholder_centroid;
	}

	return centroid.get_normalisation();
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Centroid::calculate_points_centroid(
		const PolygonOnSphere &polygon,
		bool use_interior_rings)
{
	Vector3D summed_points_position = calculate_sum_points(
			polygon.exterior_ring_vertex_begin(),
			polygon.exterior_ring_vertex_end());

	if (use_interior_rings)
	{
		const unsigned int num_interior_rings = polygon.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0;
			interior_ring_index < num_interior_rings;
			++interior_ring_index)
		{
			summed_points_position = summed_points_position + calculate_sum_points(
					polygon.interior_ring_vertex_begin(interior_ring_index),
					polygon.interior_ring_vertex_end(interior_ring_index));
		}
	}

	return Implementation::get_normalised_centroid_or_placeholder_centroid(
			summed_points_position,
			polygon.first_exterior_ring_vertex().position_vector());
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Centroid::calculate_outline_centroid(
		const PolygonOnSphere &polygon,
		bool use_interior_rings)
{
	Vector3D arc_length_weighted_centroid = Implementation::calculate_sum_arc_length_weighted_centroids(
			polygon.exterior_ring_begin(),
			polygon.exterior_ring_end());

	if (use_interior_rings)
	{
		const unsigned int num_interior_rings = polygon.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0;
			interior_ring_index < num_interior_rings;
			++interior_ring_index)
		{
			arc_length_weighted_centroid = arc_length_weighted_centroid +
					Implementation::calculate_sum_arc_length_weighted_centroids(
							polygon.interior_ring_begin(interior_ring_index),
							polygon.interior_ring_end(interior_ring_index));
		}
	}

	// If the magnitude is zero then just return the first point of the exterior ring.
	return Implementation::get_normalised_centroid_or_placeholder_centroid(
			arc_length_weighted_centroid,
			polygon.first_exterior_ring_vertex().position_vector());
}


GPlatesMaths::UnitVector3D
GPlatesMaths::Centroid::calculate_interior_centroid(
		const PolygonOnSphere &polygon,
		bool use_interior_rings)
{
	// A polygon wider than this (the radius of its outline's bounding small circle) uses its exact
	// area moment. A smaller one sums the triangles of a fan from its boundary centroid, each
	// weighted by its area at its vertices' (normalised) mean. That approximation is good for small
	// triangles but not for large ones (about 0.3 degrees off for a polygon 1 radian across), while
	// the exact moment loses precision on tiny polygons. At this size both agree to about 1e-10.
	static const double MINIMUM_RADIUS_COSINE_FOR_FAN = std::cos(0.01);
	if (polygon.get_bounding_small_circle().get_angular_extent().get_cosine().dval() <=
		MINIMUM_RADIUS_COSINE_FOR_FAN)
	{
		// Each ring's moment is its interior's (with the sign of the ring's signed area), and the
		// interior rings are holes, as for the area (see SphericalArea::calculate_polygon_signed_area).
		Vector3D area_moment =
				Implementation::calculate_area_moment_of_polygon_ring(
						polygon.exterior_ring_begin(),
						polygon.exterior_ring_end());
		if (SphericalArea::calculate_polygon_exterior_ring_signed_area(polygon).is_precisely_less_than(0))
		{
			area_moment = -area_moment;
		}

		if (use_interior_rings)
		{
			const unsigned int num_interior_rings = polygon.number_of_interior_rings();
			for (unsigned int interior_ring_index = 0;
				interior_ring_index < num_interior_rings;
				++interior_ring_index)
			{
				Vector3D interior_ring_area_moment =
						Implementation::calculate_area_moment_of_polygon_ring(
								polygon.interior_ring_begin(interior_ring_index),
								polygon.interior_ring_end(interior_ring_index));
				if (SphericalArea::calculate_polygon_interior_ring_signed_area(polygon, interior_ring_index)
						.is_precisely_less_than(0))
				{
					interior_ring_area_moment = -interior_ring_area_moment;
				}

				area_moment = area_moment - interior_ring_area_moment;
			}
		}

		return Implementation::get_normalised_centroid_or_placeholder_centroid(
				area_moment,
				polygon.first_exterior_ring_vertex().position_vector());
	}

	// Calculate a rough centroid of the polygon.
	const PointOnSphere polygon_centroid(polygon.get_boundary_centroid());

	Vector3D area_weighted_centroid =
			Implementation::calculate_sum_area_weighted_centroids_in_polygon_ring(
					polygon_centroid,
					polygon.exterior_ring_begin(),
					polygon.exterior_ring_end());

	if (use_interior_rings)
	{
		const unsigned int num_interior_rings = polygon.number_of_interior_rings();
		for (unsigned int interior_ring_index = 0;
			interior_ring_index < num_interior_rings;
			++interior_ring_index)
		{
			// Force the interior ring centroids to have the opposite effect of the exterior centroid
			// by substracting them. This is because the interior rings are holes in the polygon.
			area_weighted_centroid = area_weighted_centroid -
					Implementation::calculate_sum_area_weighted_centroids_in_polygon_ring(
							polygon_centroid,
							polygon.interior_ring_begin(interior_ring_index),
							polygon.interior_ring_end(interior_ring_index));
		}
	}

	if (area_weighted_centroid.magSqrd() <= 0)
	{
		// The area weighted triangle centroids summed to the zero vector.
		// Just return the first point of the exterior ring - this is obviously not very good but
		// it alleviates the caller from having to check an error code or catch an exception.
		// Also it's extremely unlikely to happen.
		// And even when 'area_weighted_centroid.magSqrd()' is *very* close to zero
		// but passes the test then the returned centroid is essentially random.
		// TODO: Implement a more robust alternative for those clients that require
		// an accurate centroid all the time - for most uses in GPlates the worst
		// that happens is a small circle bounding some geometry (with centroid used
		// as small circle centre) becomes larger than it would normally be resulting
		// in less efficient intersection tests.
		return polygon.first_exterior_ring_vertex().position_vector();
	}

	return area_weighted_centroid.get_normalisation();
}
