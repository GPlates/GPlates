/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_SMALLCIRCLE_H_
#define _GPLATES_MATHS_SMALLCIRCLE_H_

#include <vector>
#include "Axial.h"
#include "types.h"
#include "GreatCircle.h"
#include "PointOnSphere.h"
#include "UnitVector3D.h"
#include "Colatitude.h"
#include "Vector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A small circle of a unit sphere.
	 *
	 * Degenerate circles (ie. circles whose colatitudes are 0 or pi,
	 * resulting in point-like circles) are allowed, as are circles whose
	 * colatitudes are exactly pi (which are technically great circles).
	 * @invariant \f$ \theta \in \left[ 0, \pi \right] \f$
	 */
	class SmallCircle : public Axial
	{
		public:
			/**
			 * Create a small circle, given its axis and an angle.
			 * @param axis The axis of the circle.
			 * @param theta Angle between axis and circumference
			 *   (aka the "colatitude").  In radians, as always.
			 *
			 * @image html fig_small_circle.png
			 * @image latex fig_small_circle.eps width=2.3in
			 *
			 * @throws ViolatedClassInvariantException if
			 *   @p abs(@p cos(@a theta)) > 1.
			 */
			SmallCircle (const UnitVector3D &axis,
			             const Colatitude &theta)
				: Axial (axis) {

				_cos_colat = cos (theta);
				AssertInvariantHolds ();
			}

			/**
			 * Create a small circle, given its axis and a point.
			 * @param axis The axis of the circle.
			 * @param pt A point through which the circle must pass.
			 */
			SmallCircle (const UnitVector3D &axis,
			             const PointOnSphere &pt)
				: Axial (axis) {

				_cos_colat = dot (normal(), pt.unitvector ());
				AssertInvariantHolds ();
			}

			/**
			 * Create a small circle, given its axis and the cosine
			 * of an angle.
			 * @param axis The axis of the circle.
			 * @param cos_theta The cosine of the angle between
			 *   axis and circumference (aka the "colatitude").
			 *   Obviously, it must lie in the range [-1, 1].
			 * 
			 * @throws ViolatedClassInvariantException if
			 *   @p abs(@a cosi_theta)) > 1.
			 */
			SmallCircle (const UnitVector3D &axis,
			             const real_t &cos_theta)
				: Axial (axis), _cos_colat (cos_theta) {

				AssertInvariantHolds ();
			}

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

				real_t dp = dot (normal(), pt.unitvector ());
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
			/**
			 * The cosine of the colatitude.
			 */
			real_t _cos_colat;
	};
}

#endif  // _GPLATES_MATHS_SMALLCIRCLE_H_
