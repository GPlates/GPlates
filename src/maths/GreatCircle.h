/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_MATHS_GREATCIRCLE_H
#define GPLATES_MATHS_GREATCIRCLE_H

#include "types.h"
#include "UnitVector3D.h"
#include "PointOnSphere.h"
#include "Vector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A great circle of a unit sphere.
	 */
	class GreatCircle {

		public:
			/**
			 * Create a great circle, given its axis.
			 * @param axis The axis vector.
			 */
			explicit
			GreatCircle(
			 const UnitVector3D &axis) :
			 d_axis(axis) { }

			/**
			 * Create a great circle, given two points on it.
			 * @param p1 One point.
			 * @param p2 Another point. Must be distinct from v1.
			 *
			 * @throws IndeterminateResultException if @a p1 and 
			 *   @a p2 are either coincident or antipodal.
			 *
			 * FIXME: This should become a 'create' function.
			 */
			GreatCircle(
			 const PointOnSphere &p1,
			 const PointOnSphere &p2);
			

			/**
			 * The unit vector indicating the direction of the axis
			 * of this great circle.
			 * FIXME: This should return a reference to a const.
			 * FIXME: s/axisvector/axis/
			 */
			UnitVector3D
			axis_vector() const { return d_axis; }


			/**
			 * FIXME: Remove this.
			 */
			UnitVector3D
			normal() const {
				
				return axis_vector();
			}


			/**
			 * Evaluate whether the point @a pt lies on this
			 * great circle.
			 */
			bool
			contains(
			 const PointOnSphere &pt) const {

				return
				 perpendicular(axis_vector(), pt.position_vector());
			}

#if 0  // No longer needed
			/**
			 * Computes one intersection point of this GreatCircle
			 * with another. The other intersection point is the
			 * antipodal point to this.
			 * @param other The other GreatCircle.
			 */
			UnitVector3D intersection (const GreatCircle &other)
			 const;
#endif

		private:

			/**
			 * Given two unit vectors, @a v1 and @a v2, calculate
			 * the normal of the great circle they define.
			 *
			 * This function is invoked by one of the constructors.
			 * 
			 * @throws IndeterminateResultException if the vectors 
			 *   @a v1 and @a v2 are either parallel or antiparallel.
			 *
			 * FIXME: This should be moved into "UnitVector.h",
			 * gaining an extra const char * 'invoked_by' param.
			 */
			static UnitVector3D calc_normal(const UnitVector3D &u1,
			                               const UnitVector3D &u2);

			UnitVector3D d_axis;

	};

	inline GreatCircle
	operator- (const GreatCircle &c)
	{
		UnitVector3D v = -c.normal ();
		return GreatCircle (v);
	}

	inline bool
	are_equivalent (const GreatCircle &a, const GreatCircle &b)
	{
		// faster than two vector comparisons and a vector negation...
		return collinear(a.normal(), b.normal());
	}
}

#endif  // _GPLATES_MATHS_GREATCIRCLE_H_
