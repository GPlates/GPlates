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
	 */
	class SmallCircle
	{
		public:
			/**
			 * Create a small circle, given its axis and an angle.
			 * @param axis The axis of the circle.
			 * @param angle Angle between axis and circumference.
			 */
			SmallCircle (const UnitVector3D &axis, real_t angle)
				: _normal (axis), _theta (angle)
			{
				AssertInvariantHolds ();
			}

			/**
			 * Create a small circle, given its axis and a point
			 * on it.
			 */
			SmallCircle (const UnitVector3D &axis,
			             const PointOnSphere &pt);

			const UnitVector3D &normal () const { return _normal; }
			real_t theta () const { return _theta; }

			real_t radius () const { return sin (_theta); }

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
			UnitVector3D _normal;
			real_t _theta;
	};
}

#endif  // _GPLATES_MATHS_SMALLCIRCLE_H_
