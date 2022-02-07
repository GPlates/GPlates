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
	class PolygonOnSphere;

	namespace SphericalArea
	{
		/**
		 * Calculates the *signed* spherical area of a polygon-on-sphere.
		 *
		 * If the polygon is clockwise, when viewed from above the surface of the sphere,
		 * the returned value will be negative, otherwise it will be positive.
		 *
		 * The signed area assumes a unit radius sphere.
		 * To get the signed area on the Earth, multiply by the square of the Earth's radius.
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
		 * Calculates the *signed* spherical area of the spherical triangle bounded by the specified edges.
		 *
		 * Note that the edges must connect end-to-end in a loop - in other words the end point of
		 * @a first_edge must be the start point of @a second_edge, etc, ... the end point of
		 * @a third_edge must be the start point of @a first_edge.
		 *
		 * If any of the edges are zero length then the area returned will be zero.
		 *
		 * The area assumes a unit radius sphere.
		 * To get the area on the Earth, multiply by the square of the Earth's radius.
		 */
		real_t
		calculate_spherical_triangle_signed_area(
				const GreatCircleArc &first_edge,
				const GreatCircleArc &second_edge,
				const GreatCircleArc &third_edge);


		/**
		 * Same as @a calculate_spherical_triangle_signed_area but returns the
		 * absolute value of the area.
		 */
		inline
		real_t
		calculate_spherical_triangle_area(
				const GreatCircleArc &first_edge,
				const GreatCircleArc &second_edge,
				const GreatCircleArc &third_edge)
		{
			return abs(calculate_spherical_triangle_signed_area(first_edge, second_edge, third_edge));
		}
	}
}

#endif // GPLATES_MATHS_SPHERICALAREA_H
