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

#include "types.h"
#include "UnitVector3D.h"
#include "PointOnSphere.h"
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
			 */
			SmallCircle (const UnitVector3D &axis, real_t angle)
				: _normal (axis), _theta (angle) { }

			/**
			 * Create a small circle, given its axis and a point
			 * on it.
			 */
			SmallCircle (const UnitVector3D &axis,
			             const PointOnSphere &pt);

			const UnitVector3D &normal () const { return _normal; }
			const real_t theta () const { return _theta; }

			// TODO
			//UnitVector3D intersection (const GreatCircle &other)
			// const;

		protected:
			void AssertInvariantHolds () const;

		private:
			UnitVector3D _normal;
			real_t _theta;
	};
}

#endif  // _GPLATES_MATHS_SMALLCIRCLE_H_
