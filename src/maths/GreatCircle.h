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

#ifndef _GPLATES_MATHS_GREATCIRCLE_H_
#define _GPLATES_MATHS_GREATCIRCLE_H_

#include "types.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A great circle of a unit sphere.
	 */
	class GreatCircle
	{
		public:
			/**
			 * Create a great circle, given its axis.
			 * @param axis The axis vector.
			 */
			GreatCircle (UnitVector3D &axis)
				: _normal (axis) { }
			/**
			 * Create a great circle, given two points on it.
			 * @param v1 One point.
			 * @param v2 Another point. Must be distinct from v1.
			 */
			GreatCircle (const UnitVector3D &v1, const UnitVector3D &v2);

			UnitVector3D normal () const { return _normal; }

		private:
			UnitVector3D _normal;
	};

	inline bool operator== (const GreatCircle &a, const GreatCircle &b)
	{
		UnitVector3D an = a.normal (), bn = b.normal ();

		return (an == bn) || (an == -bn);
	}
}

#endif  // _GPLATES_MATHS_GREATCIRCLE_H_
