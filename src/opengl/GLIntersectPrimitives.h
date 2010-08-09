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
 
#ifndef GPLATES_OPENGL_GLINTERSECTPRIMITIVES_H
#define GPLATES_OPENGL_GLINTERSECTPRIMITIVES_H

#include <boost/optional.hpp>

#include "maths/PointOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


namespace GPlatesOpenGL
{
	/**
	 * Contains primitives used in intersection routines.
	 *
	 * TODO: Some of these primitive types should probably eventually be moved to the
	 * "maths/" directory as they might be useful for implementing spatial trees
	 * (used to speed up object co-registration in the data mining preprocessor).
	 */
	namespace GLIntersect
	{
		/**
		 * A 3D infinite plane defined by a normal vector and any point on the plane.
		 */
		class Plane
		{
		public:
			//! The half space result when testing a point against a plane.
			enum HalfSpaceType
			{
				NEGATIVE = -1,
				ON_PLANE = 0,
				POSITIVE = 1
			};


			/**
			 * Define a plane with a normal vector and any point on the plane.
			 *
			 * NOTE: The normal does not have to be a unit vector.
			 */
			Plane(
					const GPlatesMaths::Vector3D &normal,
					const GPlatesMaths::Vector3D &point_on_plane);

			/**
			 * Define a plane using plane coefficients (a,b,c,d).
			 *
			 * The plane satisfies the equation:
			 *   a*x + b*y + c*z + d = 0
			 *
			 * @a a, @a b and @a c effectively define the plane normal and
			 * @a d effectively defines the signed distance of the plane *to* the origin
			 * multiplied by the magnitude of the vector (a,b,c).
			 */
			Plane(
					const double &a,
					const double &b,
					const double &c,
					const double &d) :
				d_normal(a, b, c),
				d_signed_distance_to_origin(d)
			{  }


			/**
			 * Returns whether @a point is in negative or positive half space or on the plane.
			 */
			HalfSpaceType
			classify_point(
					const GPlatesMaths::Vector3D& point) const;


			/**
			 * Returns the signed distance of @a point to 'this' plane *multiplied*
			 * by the magnitude of 'this' plane's normal vector.
			 *
			 * NOTE: If the normal vector is a unit vector then this returns the 'true' distance.
			 *
			 * Distance is positive if point is in positive half-space of this plane,
			 * otherwise it's negative.
			 */
			double
			signed_distance(
					const GPlatesMaths::Vector3D& point) const
			{
				return (dot(point, d_normal) + d_signed_distance_to_origin).dval();
			}


			/**
			 * Returns the plane normal vector.
			 */
			const GPlatesMaths::Vector3D &
			get_normal() const
			{
				return d_normal;
			}

			/**
			 * Returns the signed distance of the plane *to* the origin *multiplied*
			 * by the magnitude of the plane's normal vector.
			 *
			 * If you think of the plane equation as a*x + b*y + c*z + d = 0, then
			 * this method returns 'd'.
			 */
			const double &
			get_signed_distance_to_origin() const
			{
				return d_signed_distance_to_origin.dval();
			}

		private:
			GPlatesMaths::Vector3D d_normal;

			/**
			 * The signed distance *from* the plane *to* the origin multiplied.
			 * by the magnitude of the plane's normal vector.
			 *
			 * This is the dot product:
			 * - of a vector *from* any point on the plane *to* the origin, with
			 * - the normal.
			 */
			GPlatesMaths::real_t d_signed_distance_to_origin;
		};


		/**
		 * A ray with an origin point and a unit vector direction.
		 */
		class Ray
		{
		public:
			Ray(
					const GPlatesMaths::Vector3D &ray_origin,
					const GPlatesMaths::UnitVector3D &ray_direction);

			const GPlatesMaths::Vector3D &
			get_origin() const
			{
				return d_origin;
			}

			const GPlatesMaths::UnitVector3D &
			get_direction() const
			{
				return d_direction;
			}

			/**
			 * Returns position along ray that is @a t distance from ray's origin.
			 */
			GPlatesMaths::Vector3D
			get_point_on_ray(
					const GPlatesMaths::real_t &t) const
			{
				return d_origin + t * d_direction;
			}

		private:
			GPlatesMaths::Vector3D d_origin;
			GPlatesMaths::UnitVector3D d_direction;
		};


		/**
		 * A sphere with a centre point and a radius.
		 */
		class Sphere
		{
		public:
			Sphere(
					const GPlatesMaths::Vector3D &sphere_centre,
					const GPlatesMaths::real_t &sphere_radius);

			const GPlatesMaths::Vector3D &
			get_centre() const
			{
				return d_centre;
			}

			const GPlatesMaths::real_t &
			get_radius() const
			{
				return d_radius;
			}

		private:
			GPlatesMaths::Vector3D d_centre;
			GPlatesMaths::real_t d_radius;
		};


		/**
		 * A bounding box whose axes are orthogonal but not necessarily
		 * aligned with the coordinate axes.
		 */
		class OrientedBoundingBox
		{
		public:
			/**
			 * Constructs a bounding box using the orthogonal axes and centre point.
			 *
			 * The length of the bounding box along is x-axis is twice the magnitude
			 * of @a half_length_x_axis.
			 * This similarly applies to @a half_length_y_axis and @a half_length_z_axis.
			 *
			 * NOTE: @a half_length_x_axis, @a half_length_y_axis and @a half_length_z_axis
			 * are expected to be orthogonal, but this is not checked.
			 * However it's probably ok if they're only roughly orthogonal since
			 * the bounding volume will still be convex but just won't be rectangular
			 * (might be a bit trapezoidal-like).
			 */
			OrientedBoundingBox(
					const GPlatesMaths::Vector3D &centre,
					const GPlatesMaths::Vector3D &half_length_x_axis,
					const GPlatesMaths::Vector3D &half_length_y_axis,
					const GPlatesMaths::Vector3D &half_length_z_axis);

			/**
			 * Returns the centre of this OBB.
			 */
			const GPlatesMaths::Vector3D &
			get_centre() const
			{
				return d_centre;
			}

			//
			// NOTE: These axes are not unit length.
			// They are this OBB's unit vector axes multiplied by the respective half-lengths.
			//

			const GPlatesMaths::Vector3D &
			get_half_length_x_axis() const
			{
				return d_half_length_x_axis;
			}

			const GPlatesMaths::Vector3D &
			get_half_length_y_axis() const
			{
				return d_half_length_y_axis;
			}

			const GPlatesMaths::Vector3D &
			get_half_length_z_axis() const
			{
				return d_half_length_z_axis;
			}

		private:
			//! The centre of the bounding box.
			GPlatesMaths::Vector3D d_centre;

			//
			// The orthogonal (not orthonormal) axes of the oriented bounding box.
			// These are the unit length axes of the bounding box multiplied by
			// their respective half-lengths.
			//
			GPlatesMaths::Vector3D d_half_length_x_axis;
			GPlatesMaths::Vector3D d_half_length_y_axis;
			GPlatesMaths::Vector3D d_half_length_z_axis;
		};


		/**
		 * Used to incrementally build an @a OrientedBoundingBox.
		 */
		class OrientedBoundingBoxBuilder
		{
		public:
			/**
			 * Builds a bounding box that will be aligned with the specified axes.
			 *
			 * NOTE: @a obb_x_axis, @a obb_y_axis and @a obb_z_axis
			 * are expected to be orthonormal, but this is not checked.
			 * However it's probably ok if they're only roughly orthonormal since
			 * the bounding volume will still be convex but just won't be rectangular
			 * (might be a bit trapezoidal-like).
			 */
			OrientedBoundingBoxBuilder(
					const GPlatesMaths::UnitVector3D &obb_x_axis,
					const GPlatesMaths::UnitVector3D &obb_y_axis,
					const GPlatesMaths::UnitVector3D &obb_z_axis);

			/**
			 * Expand the current bounding box (if necessary) to include @a point.
			 */
			void
			add(
					const GPlatesMaths::PointOnSphere &point);

			/**
			 * Expand the current bounding box (if necessary) to include another
			 * oriented bounding box @a obb (that may have different axes).
			 */
			void
			add(
					const OrientedBoundingBox &obb);

			/**
			 * Returns the oriented box bounding all points added so far.
			 *
			 * It's possible some dimensions of the returned oriented bounding box are degenerate.
			 * A degenerate dimension happens when all bounded points project to the same
			 * point (or very close to the same point) for a particular axis of the box.
			 * In this case the half-length vector of the degenerate axis will not be smaller
			 * than a minimum value (to prevent almost-zero-length half-length axis vectors).
			 *
			 * @throws @a PreconditionViolationError if no @a add overloads have been called so far.
			 */
			OrientedBoundingBox
			get_oriented_bounding_box() const;

		private:
			// OBB axes
			GPlatesMaths::UnitVector3D d_x_axis;
			GPlatesMaths::UnitVector3D d_y_axis;
			GPlatesMaths::UnitVector3D d_z_axis;

			// Min/max projection of bounded points onto OBB x-axis
			GPlatesMaths::real_t d_min_dot_x_axis;
			GPlatesMaths::real_t d_max_dot_x_axis;

			// Min/max projection of bounded points onto OBB y-axis
			GPlatesMaths::real_t d_min_dot_y_axis;
			GPlatesMaths::real_t d_max_dot_y_axis;

			// Min/max projection of bounded points onto OBB z-axis
			GPlatesMaths::real_t d_min_dot_z_axis;
			GPlatesMaths::real_t d_max_dot_z_axis;

			/**
			 * The half-length for a degenerate dimension of the bounding box.
			 * A degenerate dimension happens when all bounded points project to the same
			 * point (or very close to the same point) for a particular axis of the box.
			 */
			static const GPlatesMaths::real_t DEGENERATE_HALF_LENGTH_THRESHOLD;

			//! Project @a obb along one of our axes and expand as necessary.
			void
			add_projection(
					const OrientedBoundingBox &obb,
					GPlatesMaths::UnitVector3D &axis,
					GPlatesMaths::real_t &min_dot_axis,
					GPlatesMaths::real_t &max_dot_axis);
		};
	}
}

#endif // GPLATES_OPENGL_GLINTERSECTPRIMITIVES_H
