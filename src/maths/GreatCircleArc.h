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
 */

#ifndef GPLATES_MATHS_GREATCIRCLEARC_H
#define GPLATES_MATHS_GREATCIRCLEARC_H

#include "types.h"  /* real_t */
#include "PointOnSphere.h"
#include "UnitVector3D.h"

namespace GPlatesMaths {

	/** 
	 * A great-circle arc on the surface of a sphere.
	 * It has no public constructors.  To create an instance,
	 * use the 'create' static member functions.
	 *
	 * Note:
	 * - no great circle arc may have duplicate points as the start-
	 *    and end-point (or else no arc is specified).
	 * - no great circle arc may have antipodal endpoints (or else
	 *    the arc is not clearly specified).
	 * - no great circle may span an angle greater than pi radians
	 *    (this is an effect of the dot and cross products of vectors:
	 *    The angle between any two vectors is defined always to lie
	 *    in the range [0, PI] radians).
	 *
	 * Use the static member function 'test_parameter_status' to test
	 * in advance whether the endpoints are going to be valid.
	 */
	class GreatCircleArc {

		public:

		       enum ParameterStatus {

			       VALID,
			       INVALID_IDENTICAL_ENDPOINTS,
			       INVALID_ANTIPODAL_ENDPOINTS
		       };

		       /**
			* Test in advance whether the supplied great circle arc
			* creation parameters would be valid or not.
			*/
		       static
		       ParameterStatus
		       test_parameter_status(
			const PointOnSphere &p1,
			const PointOnSphere &p2);
			/**
			 * Make a great circle arc beginning at @a p1 and
			 * ending at @a p2.
			 *
			 * @throws IndeterminateResultException when one of the 
			 *   following occurs:
			 *   - @a p1 and @a p2 are the same;
			 *   - @a p1 and @a p2 are antipodal (that is, they are
			 *     diametrically opposite on the globe).
			 */
			static
			GreatCircleArc
			create(
			 const PointOnSphere &p1,
			 const PointOnSphere &p2);

			/**
			 * @overload
			 *
			 * @throws InvalidOperationException when @a rot_axis
			 *   is not collinear with the cross product of the
			 *   vectors pointing from the origin to @a p1 and
			 *   @a p2 respectively.
			 */
			static
			GreatCircleArc
			create(
			 const PointOnSphere &p1,
			 const PointOnSphere &p2,
			 const UnitVector3D &rot_axis);

			const PointOnSphere &
			start_point() const {
				
				return d_p1;
			}

			const PointOnSphere &
			end_point() const {
				
				return d_p2;
			}

			const UnitVector3D &
			rotation_axis() const {
				
				return d_rot_axis;
			}

		protected:

			GreatCircleArc(
			 const PointOnSphere &p1,
			 const PointOnSphere &p2,
			 const UnitVector3D &rot_axis) :
			 d_p1(p1),
			 d_p2(p2),
			 d_rot_axis(rot_axis) {  }

		private:

			PointOnSphere d_p1;
			PointOnSphere d_p2;
			UnitVector3D d_rot_axis;

	};
}

#endif  // GPLATES_MATHS_GREATCIRCLEARC_H
