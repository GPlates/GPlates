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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_POINTONSPHERE_H_
#define _GPLATES_MATHS_POINTONSPHERE_H_

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

			PointOnSphere(const real_t& lat, const real_t& lon);
			
			UnitVector3D
			unitvector() const { return _uv; }
#if 0
			real_t
			GetLatitude() const;

			real_t
			GetLongitude() const;
#endif
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
}

#endif  // _GPLATES_MATHS_POINTONSPHERE_H_
