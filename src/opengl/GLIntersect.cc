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

#include "GLIntersect.h"

#include "GLIntersectPrimitives.h"


//
// Algorithm from "Real-Time Rendering" book (1st edition).
//
boost::optional<GPlatesMaths::real_t>
GPlatesOpenGL::GLIntersect::intersect_ray_sphere(
		const Ray &ray,
		const Sphere &sphere)
{
	const GPlatesMaths::Vector3D l = sphere.get_centre() - ray.get_origin();
	const GPlatesMaths::real_t d = dot(l, ray.get_direction());

	const GPlatesMaths::real_t l2 = dot(l, l);
	const GPlatesMaths::real_t r2 = sphere.get_radius() * sphere.get_radius();
	if (d < 0 && l2 > r2)
	{
		return boost::none;
	}

	const GPlatesMaths::real_t m2 = l2 - d * d;
	if (m2 > r2)
	{
		return boost::none;
	}

	const GPlatesMaths::real_t q = sqrt(r2 - m2);

	return (l2 > r2) ? (d - q) : (d + q);
}
