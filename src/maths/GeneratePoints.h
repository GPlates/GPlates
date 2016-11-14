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
		/**
		 * Generate a uniform distribution of points inside the specified polygon.
		 *
		 * The uniform distribution is based on the Hierarchical Triangular Mesh.
		 */
		void
		create_uniform_points_in_polygon(
				std::vector<PointOnSphere> &points,
				const PolygonOnSphere &polygon,
				unsigned int point_density_level,
				const double &point_random_offset);
	}
}

#endif // GPLATES_MATHS_GENERATEPOINTS_H
