
/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2020 The University of Sydney, Australia
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

#ifndef GPLATES_MATH_GEOMETRYCROSSING_H
#define GPLATES_MATH_GEOMETRYCROSSING_H

#include <vector>

#include "GeometryIntersect.h"


namespace GPlatesMaths
{
	namespace GeometryCrossing
	{
		/**
		 * Finds the intersections of two geometries that result in each geometry *crossing* the other.
		 *
		 * When there are no vertex intersections then @a intersection_graph is simply returned since
		 * all intersections are therefore segment-segment intersections which are definitely crossings.
		 * However when there are vertex intersections then this function determines if one geometry
		 * actually *crosses* the other geometry as opposed to:
		 * - Just touching it but not crossing it, or
		 * - Overlapping it (where both geometries follow the same path for a portion of their line sections)
		 *   but not actually crossing over at the start or end of the overlap portion
		 *   (in other words geometry1 might merge into geometry2 from the South and then overlap for a
		 *   while and then exit geometry2 towards the South, and so never cross from South to North).
		 *   In this case no crossing is recorded (at least not along the overlapping portion).
		 *
		 * Note that you should first use @a GeometryIntersect to perform the actual intersection detection.
		 */
		GeometryIntersect::Graph
		find_crossings(
				GeometryIntersect::Graph &intersection_graph);
	}
}

#endif // GPLATES_MATH_GEOMETRYCROSSING_H
