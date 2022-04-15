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

#include <utility>  // std::pair
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>

#include "maths/types.h"


namespace GPlatesOpenGL
{
	/**
	 * Contains intersection routines used for view frustum culling and pixel/texel
	 * projections from screen-space to world-space (used for level-of-detail selection).
	 *
	 * FIXME: Some of these intersection tests could probably eventually be moved to the
	 * "maths/" directory as they might be useful for implementing spatial trees
	 * (used to speed up object co-registration in the data mining preprocessor).
	 */
	namespace GLIntersect
	{
		class Cylinder;
		class OrientedBoundingBox;
		class Plane;
		class Ray;
		class Sphere;


		/**
		 * Intersects a ray with a sphere and returns the closest distance along ray from
		 * the ray's origin to the sphere's surface or false it doesn't intersect.
		 *
		 * If ray origin is *outside* the sphere (and intersects sphere) then first intersection along ray is returned.
		 * If ray origin is *inside* the sphere then it must intersect sphere (and there's only one intersection).
		 */
		boost::optional<GPlatesMaths::real_t>
		intersect_ray_sphere(
				const Ray &ray,
				const Sphere &sphere);

		/**
		 * Intersects an infinite line (specified as a ray) with a sphere and
		 * returns the distance to both intersection points (if intersected).
		 *
		 * Note that although it's an infinite line we still need to define it using a point
		 * (on the line) and a direction, which we specify as a ray. And distances are from
		 * the ray's origin and can be negative when intersects behind ray origin.
		 *
		 * The smaller signed distance is returned 'first'.
		 */
		boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>>
		intersect_line_sphere(
				const Ray &ray,
				const Sphere &sphere);


		/**
		 * Intersects a ray with an infinite cylinder and returns the closest distance along ray from
		 * the ray's origin to the cylinder's surface or false it doesn't intersect.
		 */
		boost::optional<GPlatesMaths::real_t>
		intersect_ray_cylinder(
				const Ray &ray,
				const Cylinder &cylinder);

		/**
		 * Intersects an infinite line (specified as a ray) with an infinite cylinder and
		 * returns the distance to both intersection points (if intersected).
		 *
		 * Note that although it's an infinite line we still need to define it using a point
		 * (on the line) and a direction, which we specify as a ray. And distances are from
		 * the ray's origin and can be negative when intersects behind ray origin.
		 *
		 * The smaller signed distance is returned 'first'.
		 */
		boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>>
		intersect_line_cylinder(
				const Ray &ray,
				const Cylinder &cylinder);


		/**
		 * Intersects a ray with a plane and returns the distance along ray from
		 * the ray's origin to the plane or false it doesn't intersect.
		 */
		boost::optional<GPlatesMaths::real_t>
		intersect_ray_plane(
				const Ray &ray,
				const Plane &plane);


		/**
		 * Intersects a @a Sphere with the planes of a frustum.
		 *
		 * The frustum is defined by the intersection of the *positive* half-spaces
		 * of the specified planes. In other words, the plane normals point towards
		 * the inside of the frustum.
		 *
		 * NOTE: This frustum region should *not* be concave. The intersection of the
		 * positive half-spaces of the planes should define a convex volume (although you
		 * are allowed to have a non-closed volume, for example, you could have just two planes).
		 *
		 * @param frustum_plane_mask specifies which frustum planes are active (max 31 planes) -
		 *        it also indirectly determines how many planes are expected to be present
		 *        in @a frustum_planes - for example, if you have six frustum planes then
		 *        you start out with a mask with 6 bits set (0x3f which is 111111 in binary).
		 *
		 * If @a sphere was not completely outside any frustum plane then true is returned
		 * to indicate a possible intersection - in this case a new plane mask is also returned
		 * that defines which planes intersected @a sphere. This is useful so that objects
		 * bounded by @a sphere can be intersection tested only against those planes. Bits in
		 * the returned plane mask that are zero mean the entire @a sphere was inside the plane
		 * represented by that bit flag and hence objects bounded by @a sphere do not need to
		 * be tested against that plane.
		 * Also 'true' is returned if 'frustum_plane_mask' is zero.
		 *
		 * Only if the entire @a sphere is outside *any* frustum plane will false be returned.
		 *
		 * @throws PreconditionViolationError if 32 planes are specified (maximum is 31).
		 */
		boost::optional<boost::uint32_t>
		intersect_sphere_frustum(
				const Sphere &sphere,
				const Plane frustum_planes[],
				boost::uint32_t frustum_plane_mask);


		/**
		 * Intersects an @a OrientedBoundingBox with the planes of a frustum.
		 *
		 * The frustum is defined by the intersection of the *positive* half-spaces
		 * of the specified planes. In other words, the plane normals point towards
		 * the inside of the frustum.
		 *
		 * NOTE: This frustum region should *not* be concave. The intersection of the
		 * positive half-spaces of the planes should define a convex volume (although you
		 * are allowed to have a non-closed volume, for example, you could have just two planes).
		 *
		 * @param frustum_plane_mask specifies which frustum planes are active (max 31 planes) -
		 *        it also indirectly determines how many planes are expected to be present
		 *        in @a frustum_planes - for example, if you have six frustum planes then
		 *        you start out with a mask with 6 bits set (0x3f which is 111111 in binary).
		 *
		 * If @a obb was not completely outside any frustum plane then true is returned
		 * to indicate a possible intersection - in this case a new plane mask is also returned
		 * that defines which planes intersected @a obb. This is useful so that objects
		 * bounded by @a obb can be intersection tested only against those planes. Bits in
		 * the returned plane mask that are zero mean the entire @a obb was inside the plane
		 * represented by that bit flag and hence objects bounded by @a obb do not need to
		 * be tested against that plane.
		 * Also 'true' is returned if 'frustum_plane_mask' is zero.
		 *
		 * Only if the entire @a obb is outside *any* frustum plane will false be returned.
		 *
		 * @throws PreconditionViolationError if 32 planes are specified (maximum is 31).
		 */
		boost::optional<boost::uint32_t>
		intersect_OBB_frustum(
				const OrientedBoundingBox &obb,
				const Plane frustum_planes[],
				boost::uint32_t frustum_plane_mask);
	}
}

#endif // GPLATES_OPENGL_GLINTERSECT_H
