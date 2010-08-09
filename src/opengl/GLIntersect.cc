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

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

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


boost::optional<boost::uint32_t>
GPlatesOpenGL::GLIntersect::intersect_OBB_frustum(
		const OrientedBoundingBox& obb,
		const Plane frustum_planes[],
		boost::uint32_t in_frustum_plane_mask)
{
	// Make sure the most-significant bit is zero since we use that
	// to terminate the frustum plane iteration loop.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			in_frustum_plane_mask < 0x80000000,
			GPLATES_ASSERTION_SOURCE);

	boost::uint32_t out_frustum_plane_mask = 0;
	const Plane *p = frustum_planes;

	const GPlatesMaths::Vector3D& m = obb.get_centre();
	const GPlatesMaths::Vector3D& u = obb.get_half_length_x_axis();
	const GPlatesMaths::Vector3D& v = obb.get_half_length_y_axis();
	const GPlatesMaths::Vector3D& w = obb.get_half_length_z_axis();

	// Frustum plane bit flag - start with the first plane.
	boost::uint32_t mk = 1;

	while (mk <= in_frustum_plane_mask)
	{
		// See if we need to test the current frustum plane.
		if ((in_frustum_plane_mask & mk) != 0)
		{
			// The current frustum plane normal.
			// The normal points inwards towards the inside of the frustum.
			const GPlatesMaths::Vector3D& n = p->get_normal();

			// The signed distance of obb's centre point from the plane multiplied by
			// the magnitude of the plane's normal vector.
			const GPlatesMaths::real_t mp = p->signed_distance(m);

			// The maximum signed distance of any corner point of 'obb' (from its centre) along
			// the current plane's normal vector. Again multiplied by the magnitude of
			// the plane's normal vector.
			const GPlatesMaths::real_t np = abs(dot(n,u)) + abs(dot(n,v)) + abs(dot(n,w));

			// Test if the extremal point of the OBB (along the plane normal), that is
			// closest to the *positive* half-space of the plane, is in the negative half-space
			// of the plane. If it is then it means the entire OBB is in the negative
			// half-space and hence we can say it is completely outside the *convex* frustum.
			if (is_strictly_negative(mp + np))
			{
				// Outside.
				return boost::none;
			}

			// Test if the other extremal point of the OBB (along the plane normal), that is
			// closest to the *negative* half-space of the plane, is in the negative half-space
			// of the plane. If it is then it means the OBB is intersected by the current
			// frustum plane so we mark it as such by setting a bit flag. If it is *not* then
			// the entire OBB is in the positive half-space of the plane and anything bounded
			// by the OBB will not need intersection testing against this frustum plane.
			if (is_strictly_negative(mp - np))
			{
				out_frustum_plane_mask |= mk;
			}
		}

		// The next mask flag.
		mk += mk; // mk = (1<<iter)

		// Move to the next frustum plane.
		++p;
	}

	// The OBB was not completely outside any frustum plane so we cannot
	// say definitively that the OBB is outside the frustum.
	//
	// Note that it's still possible that it's outside though, but most
	// importantly we never say that it's outside when it's possible that it intersects.
	return out_frustum_plane_mask;
}
