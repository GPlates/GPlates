/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _GPLATES_MATHS_SMALLCIRCLE_H_
#define _GPLATES_MATHS_SMALLCIRCLE_H_

#include <vector>
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
			SmallCircle (const UnitVector3D &axis,
			             const PointOnSphere &p)
				: _axis (axis),
				  _cos_colat(dot(axis, p.position_vector())) {

				AssertInvariantHolds ();
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
			 *   @p abs(@a cos_colat)) > 1.
			 */
			SmallCircle (const UnitVector3D &axis,
			             const real_t &cos_colat)
				: _axis (axis), _cos_colat (cos_colat) {

				AssertInvariantHolds ();
			}


			/**
			 * The unit vector indicating the direction of the axis
			 * of this great circle.
			 * FIXME: This should return a reference to a const.
			 * FIXME: s/axisvector/axis/
			 */
			UnitVector3D
			axisvector() const { return _axis; }

			/**
			 * FIXME: Remove this.
			 */
			UnitVector3D
			normal () const { return axisvector(); }

			real_t
			colatitude () const { return acos (_cos_colat); }

			real_t
			cosColatitude () const { return _cos_colat; }

			/**
			 * Evaluate whether the point @a pt lies on this
			 * small circle.
			 */
			bool
			contains (const PointOnSphere &pt) const {

				real_t dp = dot(normal(), pt.position_vector());
				return (dp == _cos_colat);
			}

			/**
			 * Find the intersection points (if any) of this
			 * SmallCircle and the given GreatCircle. Intersection
			 * points are added to @a points, and the number of
			 * intersections is returned.
			 * @todo Allow any container to be passed in.
			 */
			unsigned int intersection (const GreatCircle &other,
				std::vector<PointOnSphere> &points) const;

		protected:
			/**
			 * Assert the class invariant: that the cosine of the
			 * colatitude lies within the range [-1, 1].
			 *
			 * @throws ViolatedClassInvariantException if
			 *   @p abs(@a _cos_colat) > 1.
			 */
			void AssertInvariantHolds () const;

		private:

			UnitVector3D _axis;

			real_t _cos_colat;

	};
}

#endif  // _GPLATES_MATHS_SMALLCIRCLE_H_
