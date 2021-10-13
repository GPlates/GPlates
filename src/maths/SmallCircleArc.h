/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_SMALLCIRCLEARC_H
#define GPLATES_MATHS_SMALLCIRCLEARC_H

#include <vector>
#include "types.h"
#include "GreatCircle.h"
#include "PointOnSphere.h"
#include "Rotation.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A small circle arc on the surface of a sphere.
	 *
	 * An arc is specified by a rotation axis, a start-point and an angular extent (rotation about axis).
	 *
	 * Degenerate arcs (ie. arcs whose start point lies at the axis vector (or its antipodal)
	 * are allowed, as are small circle arcs whose "colatitudes" around the "North Pole" of their
	 * axes are exactly pi (which are technically great circles).
	 *
	 * The angle spanned by the arc can lie in the closed range [0, 2 * PI].
	 * If it's '0' then the arc is point-like and if it's '2*PI' then the arc is effectively
	 * a complete small circle.
	 * @invariant \f$ \phi \in \left[ 0, 2\pi \right] \f$
	 */
	class SmallCircleArc
	{
		public:

			/**
			 * Create a small circle arc, given its axis, starting point and angular extent.
			 *
			 * @param axis The axis of the circle.
			 * @param start_point The starting point of the arc.
			 * @param angular_extent The rotation angle about @a axis (in radians) spanned by the arc.
			 *
			 * Note that @a angular_extent must lie in the closed range [0, 2*PI] and represents
			 * an anti-clockwise rotation around the small circle axis.
			 */
			static
			const SmallCircleArc
			create(
					const UnitVector3D &axis_,
					const PointOnSphere &start_point_,
					const double &angular_extent_)
			{
				return SmallCircleArc(axis_, start_point_, angular_extent_);
			}


			/**
			 * The unit vector indicating the direction of the axis of this great circle arc.
			 */
			const UnitVector3D &
			axis_vector() const
			{
				return d_axis;
			}

			/**
			 * Return the start-point of the arc.
			 */
			const PointOnSphere &
			start_point() const
			{
				return d_start_point;
			}

			/**
			 * Returns the angular extent of the arc.
			 */
			const real_t &
			angular_extent() const
			{
				return d_angular_extent;
			}

			/**
			 * Return the end-point of the arc.
			 *
			 * NOTE: This calculates the end point by rotating the start point.
			 */
			PointOnSphere
			end_point() const
			{
				return PointOnSphere(
						Rotation::create(d_axis, d_angular_extent) * start_point().position_vector());
			}

			/**
			 * The colatitude angle (angle from the axis vector to a point on the small circle arc).
			 */
			real_t
			colatitude() const
			{
				return acos(cos_colatitude());
			}

			/**
			 * The cosine of the colatitude angle.
			 */
			real_t
			cos_colatitude() const
			{
				return dot(d_axis, d_start_point.position_vector());
			}

		protected:
			/**
			 * Assert the class invariant: that the cosine of the
			 * colatitude lies within the range [-1, 1].
			 *
			 * @throws ViolatedClassInvariantException if
			 *   @p abs(@a _cos_colat) > 1.
			 */
			void
			AssertInvariantHolds() const;

		private:
			UnitVector3D d_axis;
			PointOnSphere d_start_point;
			real_t d_angular_extent;


			SmallCircleArc(
					const UnitVector3D &axis_,
					const PointOnSphere &start_point_,
					const double &angular_extent_) :
				d_axis(axis_),
				d_start_point(start_point_),
				d_angular_extent(angular_extent_)
			{
				AssertInvariantHolds();
			}
	};


	/**
	 * Uniformly subdivides a small circle arc into smaller segments and appends the
	 * sequence of subdivided points to @a tessellation_points.
	 *
	 * The subdivided segments have a maximum angular extent of @a max_segment_angular_extent radians
	 * when viewed from the centre of the small circle that the arc lies on.
	 * Each segment will extend the same angle (*uniform* subdivision) which will be less than or
	 * equal to @a max_segment_angular_extent radians.
	 */
	void
	tessellate(
			std::vector<PointOnSphere> &tessellation_points,
			const SmallCircleArc &small_circle_arc,
			const real_t &max_segment_angular_extent);
}

#endif // GPLATES_MATHS_SMALLCIRCLEARC_H
