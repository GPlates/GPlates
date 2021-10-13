/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_MATHS_SMALLCIRCLE_H_
#define _GPLATES_MATHS_SMALLCIRCLE_H_

#include <vector>
#include <boost/optional.hpp>

#include "types.h"
#include "GreatCircle.h"
#include "PointOnSphere.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A small circle of a unit sphere.
	 *
	 * Degenerate circles (ie. circles whose colatitudes are 0 or pi,
	 * resulting in point-like circles) are allowed, as are circles whose
	 * "colatitudes" around the "North Pole" of their axes are exactly pi
	 * (which are technically great circles).
	 * @invariant \f$ \theta \in \left[ 0, \pi \right] \f$
	 */
	class SmallCircle
	{
		public:

			/**
			 * Create a small circle, given its axis and a point.
			 * @param axis The axis of the circle.
			 * @param p A point through which the circle must pass.
			 */
			static
			const SmallCircle
			create(
					const UnitVector3D &axis,
					const PointOnSphere &p)
			{
				return SmallCircle(axis, dot(axis, p.position_vector()));
			}

			/**
			 * Create a small circle, given its axis and the "colatitude" of the
			 * small circle around the "North Pole" of its axis.
			 *
			 * @param axis The axis of the circle.
			 * @param colat The angle between axis and circumference (aka the "colatitude").
			 * 
			 * @image html fig_small_circle.png
			 * @image latex fig_small_circle.eps width=2.3in
			 *
			 * @throws ViolatedClassInvariantException if
			 *   @p abs(@a cos(colat)) > 1.
			 *
			 * NOTE: Use this method instead of @a create_cos_colatitude if you have the angle.
			 * In other words don't call 'create_cos_colatitude(axis, cos(colat))' since it's more
			 * expensive later if you need to retrieve the angle (colat) since an 'acos' is required.
			 */
			static
			const SmallCircle
			create_colatitude(
					const UnitVector3D &axis,
					const real_t &colat)
			{
				return SmallCircle(axis, cos(colat), colat);
			}

			/**
			 * Create a small circle, given its axis and the cosine
			 * of the "colatitude" of the small circle around the
			 * "North Pole" of its axis.
			 *
			 * @param axis The axis of the circle.
			 * @param cos_colat The cosine of the angle between
			 *   axis and circumference (aka the "colatitude").
			 *   Obviously, it must lie in the range [-1, 1].
			 * 
			 * @image html fig_small_circle.png
			 * @image latex fig_small_circle.eps width=2.3in
			 *
			 * @throws ViolatedClassInvariantException if
			 *   @p abs(@a cos_colat) > 1.
			 */
			static
			const SmallCircle
			create_cosine_colatitude(
					const UnitVector3D &axis,
					const real_t &cos_colat)
			{
				return SmallCircle(axis, cos_colat);
			}


			/**
			 * The unit vector indicating the direction of the axis
			 * of this great circle.
			 * FIXME: s/axisvector/axis/
			 */
			const UnitVector3D &
			axis_vector() const
			{
				return d_axis;
			}

			/**
			 * FIXME: Remove this.
			 */
			const UnitVector3D &
			normal() const
			{
				return axis_vector();
			}

			const real_t &
			cos_colatitude() const
			{
				return d_cos_colat;
			}

			real_t
			colatitude() const
			{
				if (!d_colat)
				{
					d_colat = acos(d_cos_colat);
				}
				return d_colat.get();
			}

			/**
			 * Evaluate whether the point @a pt lies on this
			 * small circle.
			 */
			bool
			contains(
					const PointOnSphere &pt) const
			{
				real_t dp = dot(normal(), pt.position_vector());
				return (dp == d_cos_colat);
			}

			/**
			 * Find the intersection points (if any) of this
			 * SmallCircle and the given GreatCircle. Intersection
			 * points are added to @a points, and the number of
			 * intersections is returned.
			 * @todo Allow any container to be passed in.
			 */
			unsigned int
			intersection(
					const GreatCircle &other,
					std::vector<PointOnSphere> &points) const;

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

			/**
			 * The cosine of the colatitude.
			 */
			real_t d_cos_colat;

			/**
			 * The colatitude in radians.
			 *
			 * Since 'acos' is expensive to evaluate, this is only calculated if:
			 *  - small circle constructed using colatitude directly, or
			 *  - small circle constructed using cosine-of-colatitude and colatitude subsequently requested.
			 */
			mutable boost::optional<real_t> d_colat;


			SmallCircle(
					const UnitVector3D &axis,
					const real_t &cos_colat,
					boost::optional<real_t> colat = boost::none) :
				d_axis (axis),
				d_cos_colat (cos_colat),
				d_colat(colat)
			{
				AssertInvariantHolds();
			}
	};


	/**
	 * Uniformly subdivides a small circle into smaller segments and appends the
	 * sequence of subdivided points to @a tessellation_points.
	 *
	 * NOTE: The end point is not added to @a tessellation_points. The end point is the same as the
	 * start point so you can close off the loop by copying the first point.
	 *
	 * The subdivided segments have a maximum angular extent of @a max_segment_angular_extent radians
	 * when viewed from the centre of the small circle.
	 * Each segment will extend the same angle (*uniform* subdivision) which will be less than or
	 * equal to @a max_segment_angular_extent radians.
	 */
	void
	tessellate(
			std::vector<PointOnSphere> &tessellation_points,
			const SmallCircle &small_circle,
			const real_t &max_segment_angular_extent);
}

#endif  // _GPLATES_MATHS_SMALLCIRCLE_H_
