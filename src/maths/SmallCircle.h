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
#include "Vector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A small circle of a unit sphere.
	 *
	 * Degenerate circles (ie. circles whose colatitudes are 0 or pi,
	 * resulting in point-like circles) are allowed, as are circles whose
	 * colatitudes are exactly pi (which are technically great circles).
	 */
	class SmallCircle : public Axial
	{
		public:
			/**
			 * Create a small circle, given its axis and an angle.
			 * @param axis The axis of the circle.
			 * @param theta Angle between axis and circumference,
			 *   aka the "colatitude".  In radians, as always.
			 *
			 * @image html fig_small_circle.png
			 * @image latex fig_small_circle.eps width=2.3in
			 * @invariant \f$ \theta \in \left[ 0, \pi \right] \f$
			 */
			SmallCircle (const UnitVector3D &axis, real_t theta)
				: Axial (axis), _colatitude (theta) {

				AssertInvariantHolds ();
			}

			/**
			 * Create a small circle, given its axis and a point
			 * on it.
			 */
			SmallCircle (const UnitVector3D &axis,
			             const PointOnSphere &pt);

			UnitVector3D
			normal () const { return axisvector(); }

			real_t
			colatitude () const { return _colatitude; }

			real_t
			radius () const { return sin (_colatitude); }

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
			void AssertInvariantHolds () const;

		private:
			real_t _colatitude;
	};
}

#endif  // _GPLATES_MATHS_SMALLCIRCLE_H_
