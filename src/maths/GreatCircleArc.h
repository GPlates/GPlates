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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_GREATCIRCLEARC_H_
#define _GPLATES_MATHS_GREATCIRCLEARC_H_

#include "types.h"  /* real_t */
#include "PointOnSphere.h"
#include "UnitVector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A great-circle arc on the surface of a sphere.
	 * It has no public constructors.  To create an instance,
	 * use the 'CreateGreatCircleArc' static member function.
	 *
	 * Note:
	 * - no great circle arc may have duplicate points as the start-
	 *    and end-point (or else no arc is specified).
	 * - no great circle arc may have antipodal endpoints (or else
	 *    the arc is not clearly specified).
	 * - no great circle may span an angle greater than pi radians
	 *    (this is an effect of the dot and cross products of vectors:
	 *    the angle between any two vectors is defined to always lie
	 *    in the range [0, pi] radians).
	 */
	class GreatCircleArc
	{
		public:
			static GreatCircleArc
			CreateGreatCircleArc(UnitVector3D u1,
			                     UnitVector3D u2);

			static UnitVector3D
			convertToUnitVector(PointOnSphere pt);

		protected:
			GreatCircleArc(UnitVector3D u1,
			               UnitVector3D rot_axis,
			               real_t rot_angle)
				: _u1(u1),
				  _rot_axis(rot_axis),
				  _rot_angle(rot_angle) {  }

		private:
			UnitVector3D _u1;
			UnitVector3D _rot_axis;
			real_t       _rot_angle;  // in radians
	};


// FIXME: port these functions to GreatCircleArc also...
#if 0
	inline bool
	operator==(UnitVector3D v1, UnitVector3D v2) {

		return (v1.x() == v2.x()
		     && v1.y() == v2.y()
		     && v1.z() == v2.z());
	}


	inline bool
	operator!=(UnitVector3D v1, UnitVector3D v2) {

		return (v1.x() != v2.x()
		     || v1.y() != v2.y()
		     || v1.z() != v2.z());
	}


	inline real_t
	operator*(UnitVector3D v1, UnitVector3D v2) {

		return (v1.x() * v2.x()
		      + v1.y() * v2.y()
		      + v1.z() * v2.z());
	}


	DirVector3D cross(UnitVector3D v1, UnitVector3D v2);
#endif
}

#endif  // _GPLATES_MATHS_GREATCIRCLEARC_H_
