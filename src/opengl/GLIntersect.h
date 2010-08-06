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
 
#ifndef GPLATES_OPENGL_GLINTERSECT_H
#define GPLATES_OPENGL_GLINTERSECT_H

#include <boost/optional.hpp>

#include "maths/types.h"


namespace GPlatesOpenGL
{
	/**
	 * Contains intersection routines used for view frustum culling and pixel/texel
	 * projections from screen-space to world-space (used for level-of-detail selection).
	 *
	 * FIXME: Some of these intersection tests should probably eventually be moved to the
	 * "maths/" directory as they might be useful for implementing spatial trees
	 * (used to speed up object co-registration in the data mining preprocessor).
	 */
	namespace GLIntersect
	{
		class Ray;
		class Sphere;

		/**
		 * Intersects a ray with a sphere and returns the closest distance from
		 * the ray's origin to the sphere's surface or false it doesn't intersect.
		 */
		boost::optional<GPlatesMaths::real_t>
		intersect_ray_sphere(
				const Ray &ray,
				const Sphere &sphere);
	}
}

#endif // GPLATES_OPENGL_GLINTERSECT_H
