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

#include <boost/optional.hpp>

#include "PolygonOrientation.h"

#include "AngularDistance.h"
#include "GnomonicProjection.h"
#include "MathsUtils.h"
#include "PolygonOnSphere.h"
#include "SphericalArea.h"
#include "Vector3D.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * If any 3D point (in polygon) is further than this angle from the polygon centroid
		 * then the gnomonic projection fails.
		 */
		const double MAXIMUM_PROJECTION_ANGLE_DEGREES = 45;


		/**
		 * Iterate through the vertices of the polygon ring and project onto the plane and
		 * calculate the signed area of the projected ring.
		 *
		 * Returns none if unable to project a point in the ring (due to angle between point and
		 * projection tangent point being too large).
		 */
		boost::optional<real_t>
		calculate_polygon_ring_projected_signed_area(
				const PolygonOnSphere::ring_vertex_const_iterator &ring_vertex_begin,
				const PolygonOnSphere::ring_vertex_const_iterator &ring_vertex_end,
				const GnomonicProjection &gnomonic_projection)
		{
			real_t ring_projected_signed_area = 0;

			QPointF last_proj_point;

			PolygonOnSphere::ring_vertex_const_iterator ring_vertex_iter = ring_vertex_begin;
			if (ring_vertex_iter != ring_vertex_end)
			{
				const PointOnSphere &vertex = *ring_vertex_iter;

				boost::optional<QPointF> point =
						gnomonic_projection.project_from_point_on_sphere(vertex);
				if (!point)
				{
					return boost::none;
				}

				last_proj_point = point.get();
				++ring_vertex_iter;
			}

			for ( ; ring_vertex_iter != ring_vertex_end; ++ring_vertex_iter)
			{
				const PointOnSphere &vertex = *ring_vertex_iter;

				boost::optional<QPointF> point =
						gnomonic_projection.project_from_point_on_sphere(vertex);
				if (!point)
				{
					return boost::none;
				}

				ring_projected_signed_area +=
						last_proj_point.x() * point->y() -
						last_proj_point.y() * point->x();

				last_proj_point = point.get();
			}

			return ring_projected_signed_area;
		}
	}
}


GPlatesMaths::PolygonOrientation::Orientation
GPlatesMaths::PolygonOrientation::calculate_polygon_orientation(
		const PolygonOnSphere &polygon)
{
	//PROFILE_BLOCK("poly orientation test");

	// Project the polygon onto a tangent plane using a gnomonic projection.
	//
	// We'll calculate signed areas of the projected 2D triangles since that's less expensive
	// than calculating signed areas of spherical triangles (but we'll calculate spherical triangles
	// if we have trouble projecting onto a tangent plane).
	//
	// Get the centroid of the polygon to use as our tangent point.
	const PointOnSphere tangent_point(polygon.get_boundary_centroid());
	const GnomonicProjection gnomonic_projection(
			tangent_point,
			// If any 3D point (in polygon) is further than this angle from the tangent point
			// then the projection fails and we'll calculate signed triangle areas without
			// projecting (ie, signed triangle areas directly on the spherical polygon)...
			AngularDistance::create_from_angle(convert_deg_to_rad(MAXIMUM_PROJECTION_ANGLE_DEGREES)));

	// Calculate projected signed area of exterior ring.
	const boost::optional<real_t> exterior_ring_projected_signed_area =
			calculate_polygon_ring_projected_signed_area(
					polygon.exterior_ring_vertex_begin(),
					polygon.exterior_ring_vertex_end(),
					gnomonic_projection);
	if (!exterior_ring_projected_signed_area)
	{
		// Get an accurate polygon signed area.
		//
		// NOTE: We have to be a little bit careful here in case PolygonOnSphere methods
		// like 'get_area()' use methods like the one we're currently in to calculate its
		// results - however the signed area calculation is safe - there won't be any infinite recursion.
		// If in doubt call 'SphericalArea::calculate_polygon_signed_area()' directly.
		return polygon.get_signed_area().is_precisely_less_than(0) ? CLOCKWISE : COUNTERCLOCKWISE;
	}

	real_t total_projected_signed_area = exterior_ring_projected_signed_area.get();

	// Calculate projected signed area of interior rings.
	const unsigned int num_interior_rings = polygon.number_of_interior_rings();
	for (unsigned int interior_ring_index = 0;
		interior_ring_index < num_interior_rings;
		++interior_ring_index)
	{
		const boost::optional<real_t> interior_ring_projected_signed_area =
				calculate_polygon_ring_projected_signed_area(
						polygon.interior_ring_vertex_begin(interior_ring_index),
						polygon.interior_ring_vertex_end(interior_ring_index),
						gnomonic_projection);
		if (!interior_ring_projected_signed_area)
		{
			// Get an accurate polygon signed area.
			//
			// NOTE: We have to be a little bit careful here in case PolygonOnSphere methods
			// like 'get_area()' use methods like the one we're currently in to calculate its
			// results - however the signed area calculation is safe - there won't be any infinite recursion.
			// If in doubt call 'SphericalArea::calculate_polygon_signed_area()' directly.
			return polygon.get_signed_area().is_precisely_less_than(0) ? CLOCKWISE : COUNTERCLOCKWISE;
		}

		// Force the interior ring areas to have the opposite sign of the exterior area.
		// This way the interior rings reduce the absolute area of the exterior ring because
		// they are holes in the polygon. We need to do this since we don't know the orientation
		// of the interior rings (ie, we never forced them to have the opposite orientation of
		// the exterior ring like some software does).
		// This is the same as what is done in 'SphericalArea::calculate_polygon_signed_area()'.
		if (exterior_ring_projected_signed_area->is_precisely_greater_than(0))
		{
			total_projected_signed_area -= abs(interior_ring_projected_signed_area.get());
		}
		else // exterior_ring_projected_signed_area is negative...
		{
			total_projected_signed_area += abs(interior_ring_projected_signed_area.get());
		}
	}

	// Note that the interior rings shouldn't normally affect the orientation because they have the
	// opposite sign (area) to the exterior ring and the interior rings should normally be fully
	// inside the exterior ring and hence should not have enough area to flip the signed area.
	// However if the rings intersect the exterior then it's possible (though unlikely) that they
	// will have enough area to flip the sign.

	return total_projected_signed_area.is_precisely_less_than(0) ? CLOCKWISE : COUNTERCLOCKWISE;
}


GPlatesMaths::PolygonOrientation::Orientation
GPlatesMaths::PolygonOrientation::calculate_polygon_exterior_ring_orientation(
		const PolygonOnSphere &polygon)
{
	// Project the polygon ring onto a tangent plane using a gnomonic projection.
	//
	// We'll calculate signed areas of the projected 2D triangles since that's less expensive
	// than calculating signed areas of spherical triangles (but we'll calculate spherical triangles
	// if we have trouble projecting onto a tangent plane).
	//
	// Get the centroid of the polygon to use as our tangent point.
	const PointOnSphere tangent_point(polygon.get_boundary_centroid());
	const GnomonicProjection gnomonic_projection(
			tangent_point,
			// If any 3D point (in polygon) is further than this angle from the tangent point
			// then the projection fails and we'll calculate signed triangle areas without
			// projecting (ie, signed triangle areas directly on the spherical polygon)...
			AngularDistance::create_from_angle(convert_deg_to_rad(MAXIMUM_PROJECTION_ANGLE_DEGREES)));

	// Calculate projected signed area of polygon ring.
	boost::optional<real_t> ring_projected_signed_area =
			calculate_polygon_ring_projected_signed_area(
					polygon.exterior_ring_vertex_begin(),
					polygon.exterior_ring_vertex_end(),
					gnomonic_projection);

	if (!ring_projected_signed_area)
	{
		// Get an accurate polygon ring signed area.
		ring_projected_signed_area = SphericalArea::calculate_polygon_exterior_ring_signed_area(polygon);
	}

	return ring_projected_signed_area->is_precisely_less_than(0) ? CLOCKWISE : COUNTERCLOCKWISE;
}


GPlatesMaths::PolygonOrientation::Orientation
GPlatesMaths::PolygonOrientation::calculate_polygon_interior_ring_orientation(
		const PolygonOnSphere &polygon,
		unsigned int interior_ring_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			interior_ring_index < polygon.number_of_interior_rings(),
			GPLATES_ASSERTION_SOURCE);

	// Project the polygon ring onto a tangent plane using a gnomonic projection.
	//
	// We'll calculate signed areas of the projected 2D triangles since that's less expensive
	// than calculating signed areas of spherical triangles (but we'll calculate spherical triangles
	// if we have trouble projecting onto a tangent plane).
	//
	// Get the centroid of the polygon to use as our tangent point.
	const PointOnSphere tangent_point(polygon.get_boundary_centroid());
	const GnomonicProjection gnomonic_projection(
			tangent_point,
			// If any 3D point (in polygon) is further than this angle from the tangent point
			// then the projection fails and we'll calculate signed triangle areas without
			// projecting (ie, signed triangle areas directly on the spherical polygon)...
			AngularDistance::create_from_angle(convert_deg_to_rad(MAXIMUM_PROJECTION_ANGLE_DEGREES)));

	// Calculate projected signed area of polygon ring.
	boost::optional<real_t> ring_projected_signed_area =
			calculate_polygon_ring_projected_signed_area(
					polygon.interior_ring_vertex_begin(interior_ring_index),
					polygon.interior_ring_vertex_end(interior_ring_index),
					gnomonic_projection);

	if (!ring_projected_signed_area)
	{
		// Get an accurate polygon ring signed area.
		ring_projected_signed_area =
				SphericalArea::calculate_polygon_interior_ring_signed_area(polygon, interior_ring_index);
	}

	return ring_projected_signed_area->is_precisely_less_than(0) ? CLOCKWISE : COUNTERCLOCKWISE;
}
