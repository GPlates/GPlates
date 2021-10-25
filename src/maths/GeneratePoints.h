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

#ifndef GPLATES_MATHS_GENERATEPOINTS_H
#define GPLATES_MATHS_GENERATEPOINTS_H

#include <vector>

#include "PointOnSphere.h"
#include "PolygonOnSphere.h"


namespace GPlatesMaths
{
	namespace GeneratePoints
	{
		//
		// In the following functions...
		//
		// The uniform distribution is based on a subdivided Rhombic Triacontahedron.
		// Points at a @a point_density_level of zero are spaced roughly 40 degrees apart.
		// Each increment of @a point_density_level halves the spacing.
		//
		// If @a point_random_offset is specified then it must be in the range [0, 1] with 0
		// meaning no random offset, and 1 meaning full random offset whereby each point is randomly
		// offset within a circle of radius half the spacing between points.
		//


		/**
		 * Generate a uniform distribution of points across the entire globe.
		 */
		void
		create_global_uniform_points(
				std::vector<PointOnSphere> &points,
				unsigned int point_density_level,
				const double &point_random_offset);


		/**
		 * Generate a uniform distribution of points within a latitude/longitude extent.
		 *
		 * @a top and @a bottom must be in range [-90, 90].
		 * @a left and @a right must be in range [-360, 360].
		 */
		void
		create_uniform_points_in_lat_lon_extent(
				std::vector<PointOnSphere> &points,
				unsigned int point_density_level,
				const double &point_random_offset,
				const double &top,    // Max lat.
				const double &bottom, // Min lat.
				const double &left,   // Min lon.
				const double &right); // Max lon.


		/**
		 * Generate a uniform distribution of points inside the specified polygon.
		 */
		void
		create_uniform_points_in_polygon(
				std::vector<PointOnSphere> &points,
				unsigned int point_density_level,
				const double &point_random_offset,
				const PolygonOnSphere &polygon);
	}
}

#endif // GPLATES_MATHS_GENERATEPOINTS_H
