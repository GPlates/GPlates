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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_POINTONSPHERE_H_
#define _GPLATES_MATHS_POINTONSPHERE_H_

#include <ostream>
#include "types.h"  /* real_t */
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	/** 
	 * Represents a point on the surface of a sphere. 
	 *
	 * Internally, this is stored as a 3D unit vector.
	 * There is a one-to-one mapping from the set of all points on the
	 * surface of a sphere and the set of all 3D unit vectors.
	 *
	 * As long as the invariant of the unit vector is maintained,
	 * the point will definitely lie on the surface of the sphere.
	 */
	class PointOnSphere
	{
		public:
			explicit 
			PointOnSphere(const UnitVector3D &uv) : _uv(uv) {  }

			UnitVector3D
			unitvector() const { return _uv; }

		private:
			UnitVector3D _uv;
	};


	inline bool
	operator==(const PointOnSphere &p1, const PointOnSphere &p2) {

		return (p1.unitvector() == p2.unitvector());
	}


	inline bool
	operator!=(const PointOnSphere &p1, const PointOnSphere &p2) {

		return (p1.unitvector() != p2.unitvector());
	}

	/**
	 * True iff points are equal or antipodal
	 */
	inline bool diametric (const PointOnSphere &s1, const PointOnSphere &s2)
	{
		return (collinear (s1.unitvector (), s2.unitvector ()));
	}


	inline std::ostream &
	operator<<(std::ostream &os, const PointOnSphere &p) {

		os << p.unitvector();
		return os;
	}
}

#endif  // _GPLATES_MATHS_POINTONSPHERE_H_
