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
 
#ifndef GPLATES_MATHS_SPHERICALAREA_H
#define GPLATES_MATHS_SPHERICALAREA_H

#include "types.h"


namespace GPlatesMaths
{
	class GreatCircleArc;
	class PointOnSphere;
	class PolygonOnSphere;

	namespace SphericalArea
	{
		/**
		 * Calculates the *signed* spherical area of a polygon-on-sphere.
		 *
		 * If the polygon is clockwise, when viewed from above the surface of the sphere,
		 * the returned value will be negative, otherwise it will be positive.
		 *
		 * The interior rings reduce the absolute area of the exterior ring (regardless of their
		 * orientation) because they are holes in the polygon and are meant to cutout the internal area.
		 * So if the signed area of exterior ring is positive then any interior rings will reduce that, and
		 * if the signed area of exterior ring is negative then any interior rings will make that less negative.
		 * Note that the orientation of the interior rings can be arbitrary (ie, the interior orientations
		 * are not forced to have the opposite orientation to the exterior ring like some software does)
		 * and they will still correctly affect the signed area.
		 *
		 * The signed area assumes a unit radius sphere.
		 * To get the signed area on the Earth, multiply by the square of the Earth's radius
		 * (see GPlatesUtils::Earth).
		 */
		real_t
		calculate_polygon_signed_area(
				const PolygonOnSphere &polygon);


		/**
		 * Same as @a calculate_polygon_signed_area but returns the absolute value of the area.
		 *
		 * The area is guaranteed to be less than "2 * PI" (ie, the area of a hemisphere).
		 * You can think of a polygon-on-sphere as containing the smaller *inside* area or
		 * the larger *outside* area.
		 * Here we calculate the smaller *inside* area (less than "2 * PI").
		 * This effectively makes this function polygon-orientation agnostic - the
		 * polygon orientation is what determines whether the smaller or larger area
		 * actually gets calculated internally - and this function takes the smaller area.
		 */
		inline
		real_t
		calculate_polygon_area(
				const PolygonOnSphere &polygon)
		{
			return abs(calculate_polygon_signed_area(polygon));
		}


		/**
		 * Calculates the *signed* spherical area of the exterior ring of a polygon.
		 */
		real_t
		calculate_polygon_exterior_ring_signed_area(
				const PolygonOnSphere &polygon);

		/**
		 * Same as @a calculate_polygon_exterior_ring_signed_area but returns the absolute value of the area.
		 */
		inline
		real_t
		calculate_polygon_exterior_ring_area(
				const PolygonOnSphere &polygon)
		{
			return abs(calculate_polygon_exterior_ring_signed_area(polygon));
		}

		/**
		 * Calculates the *signed* spherical area of the interior ring at the specified interior ring
		 * index of a polygon.
		 */
		real_t
		calculate_polygon_interior_ring_signed_area(
				const PolygonOnSphere &polygon,
				unsigned int interior_ring_index);

		/**
		 * Same as @a calculate_polygon_interior_ring_signed_area but returns the absolute value of the area.
		 */
		inline
		real_t
		calculate_polygon_interior_ring_area(
				const PolygonOnSphere &polygon,
				unsigned int interior_ring_index)
		{
			return abs(calculate_polygon_interior_ring_signed_area(polygon, interior_ring_index));
		}


		/**
		 * Calculates the *signed* spherical area of the spherical triangle bounded by the specified point and edge.
		 *
		 * The direction of the edge (ie, its start to end point) determines the orientation of the
		 * spherical triangle and hence whether its signed area is negative or positive.
		 * In other words the direction is from @a point to the edge start point to the edge end point
		 * and back to @a point.
		 *
		 * The area assumes a unit radius sphere.
		 * To get the area on the Earth, multiply by the square of the Earth's radius (see GPlatesUtils::Earth).
		 */
		real_t
		calculate_spherical_triangle_signed_area(
				const PointOnSphere &point,
				const GreatCircleArc &edge);

		/**
		 * Calculates the *signed* spherical area of the spherical triangle bounded by the specified points.
		 *
		 * The orientation of the spherical triangle is from first point to second to third and back to
		 * the first point. This orientation determines whether the signed area is negative or positive.
		 *
		 * The area assumes a unit radius sphere.
		 * To get the area on the Earth, multiply by the square of the Earth's radius (see GPlatesUtils::Earth).
		 *
		 * Note: If you have an edge (a @a GreatCircleArc) and a point then it is more efficient to
		 * use the other overload of @a calculate_spherical_triangle_signed_area instead.
		 */
		real_t
		calculate_spherical_triangle_signed_area(
				const PointOnSphere &first_point,
				const PointOnSphere &second_point,
				const PointOnSphere &third_point);


		/**
		 * Same as @a calculate_spherical_triangle_signed_area but returns the absolute value of the area.
		 */
		inline
		real_t
		calculate_spherical_triangle_area(
				const PointOnSphere &point,
				const GreatCircleArc &edge)
		{
			return abs(calculate_spherical_triangle_signed_area(point, edge));
		}

		/**
		 * Same as @a calculate_spherical_triangle_signed_area but returns the absolute value of the area.
		 */
		inline
		real_t
		calculate_spherical_triangle_area(
				const PointOnSphere &first_point,
				const PointOnSphere &second_point,
				const PointOnSphere &third_point)
		{
			return abs(calculate_spherical_triangle_signed_area(first_point, second_point, third_point));
		}
	}
}

#endif // GPLATES_MATHS_SPHERICALAREA_H
