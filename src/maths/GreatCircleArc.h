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
			/**
			 * Make a great circle arc beginning at @a p1 and ending
			 * at @a p2.
			 *
			 * @throws IndeterminateResultException when one of the 
			 *   following occurs:
			 *   - @a p1 and @a p2 are the same;
			 *   - @a p1 and @a p2 are antipodal (that is, they are
			 *     diametrically opposite on the globe).
			 */
			static GreatCircleArc
			CreateGreatCircleArc(PointOnSphere p1,
			                     PointOnSphere p2);

			/**
			 * @overload
			 *
			 * @throws InvalidOperationException when @a rot_axis
			 *   is not collinear with the cross product of the vectors
			 *   pointing from the origin to @a p1 and @a p2 respectively.
			 */
			static GreatCircleArc
			CreateGreatCircleArc(PointOnSphere p1,
			                     PointOnSphere p2,
			                     UnitVector3D rot_axis);

			PointOnSphere
			startPoint() const { return _p1; }

			PointOnSphere
			endPoint() const { return _p2; }

			UnitVector3D
			rotationAxis() const { return _rot_axis; }

		protected:
			GreatCircleArc(PointOnSphere p1, PointOnSphere p2,
			               UnitVector3D rot_axis)
				: _p1(p1), _p2(p2),
				  _rot_axis(rot_axis) {  }

		private:
			PointOnSphere _p1, _p2;
			UnitVector3D _rot_axis;
	};
}

#endif  // _GPLATES_MATHS_GREATCIRCLEARC_H_
