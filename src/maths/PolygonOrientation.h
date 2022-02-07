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

#ifndef GPLATES_MATHS_POLYGONORIENTATION_H
#define GPLATES_MATHS_POLYGONORIENTATION_H


namespace GPlatesMaths
{
	class PointOnSphere;
	class PolygonOnSphere;

	namespace PolygonOrientation
	{
		//! Orientation of a polygon.
		enum Orientation
		{
			CLOCKWISE,
			COUNTERCLOCKWISE
		};


		/**
		 * Calculates the orientation of the vertices of @a polygon
		 * when viewed from above the globe (looking down on the globe's surface).
		 *
		 * A polygon that is larger than a half-sphere is really a smaller polygon
		 * on the other side of the globe (and the viewpoint, for orientation considerations,
		 * will be looking down from the other side of the globe).
		 *
		 * The orientation of the polygon's exterior ring determines the orientation.
		 * However if there are interior rings and they are not fully contained inside the exterior ring
		 * (ie, they intersect the exterior) then it's possible (though unlikely) that the combined area
		 * of the interior rings will be enough (ie, greater than exterior ring area) to flip the orientation.
		 * Note that the orientation of the interior rings can be arbitrary (ie, the interior orientations
		 * are not forced to have the opposite orientation to the exterior ring like some software does)
		 * and they will still correctly affect the orientation.
		 *
		 * The polygon orientation is determined from the sign of the polygon's signed area.
		 * Internally a cheaper signed area calculation is attempted based on projecting the polygon
		 * onto a 2D tangent plane (and calculating the 2D signed area) - if that fails (due to
		 * inability to project polygon onto tangent plane because too big) then the more expensive
		 * spherical signed area is calculated.
		 */
		Orientation
		calculate_polygon_orientation(
				const PolygonOnSphere &polygon);


		/**
		 * Calculates the orientation of the exterior ring of the specified polygon
		 * when viewed from above the globe (looking down on the globe's surface).
		 */
		Orientation
		calculate_polygon_exterior_ring_orientation(
				const PolygonOnSphere &polygon);

		/**
		 * Calculates the orientation of the interior ring at the specified interior ring index of the
		 * specified polygon when viewed from above the globe (looking down on the globe's surface).
		 */
		Orientation
		calculate_polygon_interior_ring_orientation(
				const PolygonOnSphere &polygon,
				unsigned int interior_ring_index);
	}
}

#endif // GPLATES_MATHS_POLYGONORIENTATION_H
