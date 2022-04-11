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
	if (d.dval() < 0 && l2.dval() > r2.dval())
	{
		// Ray origin is outside sphere (l2>r2) and sphere is behind ray origin.
		// So positive direction along ray cannot intersect sphere.
		return boost::none;
	}

	const GPlatesMaths::real_t m2 = l2 - d * d;
	if (m2.dval() > r2.dval())
	{
		// Infinite line along ray does not intersect the sphere.
		return boost::none;
	}

	const GPlatesMaths::real_t q = sqrt(r2 - m2);

	// If ray origin is outside sphere (l2>r2) then we know sphere is also in front of ray origin
	// (since we already know that 'd<0 && l2>r2' is not true, so must have 'd>=0').
	// In this case we choose the first intersection since it's closest to ray origin.
	// Otherwise ray origin is inside sphere (l2<=r2) and can only intersect sphere once.
	// In this case we choose the second intersection (because first intersection is behind ray origin).
	return (l2.dval() > r2.dval()) ? (d - q) : (d + q);
}


boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>>
GPlatesOpenGL::GLIntersect::intersect_line_sphere(
		const Ray &ray,
		const Sphere &sphere)
{
	const GPlatesMaths::Vector3D l = sphere.get_centre() - ray.get_origin();
	const GPlatesMaths::real_t d = dot(l, ray.get_direction());

	const GPlatesMaths::real_t l2 = dot(l, l);
	const GPlatesMaths::real_t r2 = sphere.get_radius() * sphere.get_radius();

	const GPlatesMaths::real_t m2 = l2 - d * d;
	if (m2.dval() > r2.dval())
	{
		// Infinite line along ray does not intersect the sphere.
		return boost::none;
	}

	const GPlatesMaths::real_t q = sqrt(r2 - m2);

	return std::make_pair(d - q, d + q);
}


//
// Algorithm from "Real-Time Collision Detection" book.
//
boost::optional<GPlatesMaths::real_t>
GPlatesOpenGL::GLIntersect::intersect_ray_cylinder(
		const Ray &ray,
		const Cylinder &cylinder)
{
	// First find intersections of infinite line with infinite cylinder.
	boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>> intersect_result =
			intersect_line_cylinder(ray, cylinder);
	if (!intersect_result)
	{
		return boost::none;
	}

	// If the first intersection is in front of ray origin then return it.
	if (intersect_result->first.dval() >= 0)
	{
		return intersect_result->first;
	}

	// The first intersection is behind ray origin.
	// If the second (further) intersection is in front of ray origin then return it.
	if (intersect_result->second.dval() >= 0)
	{
		return intersect_result->second;
	}

	// Both intersections are behind the ray origin, so no intersection with ray.
	return boost::none;
}


boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>>
GPlatesOpenGL::GLIntersect::intersect_line_cylinder(
		const Ray &ray,
		const Cylinder &cylinder)
{
	const GPlatesMaths::UnitVector3D &n = ray.get_direction();
	const GPlatesMaths::UnitVector3D &d = cylinder.get_axis();
	const GPlatesMaths::real_t r = cylinder.get_radius();

	const GPlatesMaths::Vector3D m = ray.get_origin() - cylinder.get_base_point();
	const GPlatesMaths::real_t n_d = dot(n, d);
	const GPlatesMaths::real_t m_d = dot(m, d);

	const GPlatesMaths::real_t a = 1.0 - n_d * n_d;
	const GPlatesMaths::real_t b = dot(m, n) - n_d * m_d;
	const GPlatesMaths::real_t c = dot(m, m) - r * r - m_d * m_d;

	GPlatesMaths::real_t h = b * b - a * c;
	if (h.dval() < 0.0)
	{
		// Infinite line along ray does not intersect the cylinder.
		return boost::none;
	}
	const GPlatesMaths::real_t sqrt_h = sqrt(h);

	if (a == 0)  // Note: this is an epsilon test.
	{
		// The line and cylinder are parallel and hence never intersect.
		return boost::none;
	}
	const GPlatesMaths::real_t inv_a = 1.0 / a;

	return std::make_pair(inv_a * (-b - sqrt_h), inv_a * (-b + sqrt_h));
}


boost::optional<GPlatesMaths::real_t>
GPlatesOpenGL::GLIntersect::intersect_ray_plane(
		const Ray &ray,
		const Plane &plane)
{
	//
	// Points on the plane satisfy:
	// 
	//   N.R + d = 0
	//   N.(R0 + t*Rd) + d = 0
	//
	// ...where "R = R0 + t*Rd" is the ray (with origin R0 and direction Rd) and N is plane normal
	// (could be unnormalized) and d is signed unnormalized distance of plane to origin.
	//
	// Rearranging gives:
	//
	//   t = -d - N.R0
	//       ---------
	//         N.Rd
	//

	const GPlatesMaths::real_t denom = dot(plane.get_normal_unnormalised(), ray.get_direction());
	if (denom == 0)  // Note: this is an epsilon test.
	{
		// The ray line is perpendicular to the plane and hence they either they never intersect
		// or the ray lies on the plane and there's an infinity of intersections.
		// For both cases we just return no intersection.
		return boost::none;
	}

	const GPlatesMaths::real_t t =
			(-plane.get_signed_distance_to_origin_unnormalised() - dot(plane.get_normal_unnormalised(), ray.get_origin())) / denom;
	if (t.dval() < 0)
	{
		// The ray's line intersects the plane, but it intersects behind the ray.
		return boost::none;
	}

	return t;
}


boost::optional<boost::uint32_t>
GPlatesOpenGL::GLIntersect::intersect_sphere_frustum(
		const Sphere &sphere,
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

	const GPlatesMaths::Vector3D &c = sphere.get_centre();
	const GPlatesMaths::real_t &r = sphere.get_radius();

	// Frustum plane bit flag - start with the first plane.
	boost::uint32_t mk = 1;

	while (mk <= in_frustum_plane_mask)
	{
		// See if we need to test the current frustum plane.
		if ((in_frustum_plane_mask & mk) != 0)
		{
			// The signed distance of sphere's centre point from the plane.
			const GPlatesMaths::real_t d = p->signed_distance(c);

			// Test if the extremal point of the sphere, that is closest to the *positive*
			// half-space of the plane, is in the negative half-space of the plane.
			// If it is then it means the entire sphere is in the negative half-space and
			// hence we can say it is completely outside the *convex* frustum.
			if (is_strictly_negative(d + r))
			{
				// Outside.
				return boost::none;
			}

			// Test if the other extremal point of the sphere, that is closest to the *negative*
			// half-space of the plane, is in the negative half-space of the plane.
			// If it is then it means the sphere is intersected by the current frustum plane so we
			// mark it as such by setting a bit flag. If it is *not* then the entire sphere is in
			// the positive half-space of the plane and anything bounded by the sphere will not
			// need intersection testing against this frustum plane.
			if (is_strictly_negative(d - r))
			{
				out_frustum_plane_mask |= mk;
			}
		}

		// The next mask flag.
		mk += mk; // mk = (1<<iter)

		// Move to the next frustum plane.
		++p;
	}

	// The Sphere was not completely outside any frustum plane so we cannot
	// say definitively that the Sphere is outside the frustum.
	//
	// Note that it's still possible that it's outside though, but most
	// importantly we never say that it's outside when it's possible that it intersects.
	return out_frustum_plane_mask;
}


boost::optional<boost::uint32_t>
GPlatesOpenGL::GLIntersect::intersect_OBB_frustum(
		const OrientedBoundingBox &obb,
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

	const GPlatesMaths::Vector3D &m = obb.get_centre();
	const GPlatesMaths::Vector3D &u = obb.get_half_length_x_axis();
	const GPlatesMaths::Vector3D &v = obb.get_half_length_y_axis();
	const GPlatesMaths::Vector3D &w = obb.get_half_length_z_axis();

	// Frustum plane bit flag - start with the first plane.
	boost::uint32_t mk = 1;

	while (mk <= in_frustum_plane_mask)
	{
		// See if we need to test the current frustum plane.
		if ((in_frustum_plane_mask & mk) != 0)
		{
			// The current frustum plane normal.
			// The normal points inwards towards the inside of the frustum.
			const GPlatesMaths::Vector3D &n = p->get_normal_unnormalised();

			// The signed distance of obb's centre point from the plane multiplied by
			// the magnitude of the plane's normal vector.
			const GPlatesMaths::real_t mp = p->signed_distance_unnormalised(m);

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
