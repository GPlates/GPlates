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

#ifndef _GPLATES_MATHS_FINITEROTATION_H_
#define _GPLATES_MATHS_FINITEROTATION_H_

#include "PointOnSphere.h"
#include "UnitQuaternion3D.h"
#include "types.h"  /* real_t */


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


	/** 
	 * Represents a so-called "finite rotation" of plate tectonics.
	 *
	 * Plate tectonics theory states that the motion of plates on the
	 * surface of the globe can be described by "Euler rotations".
	 * A finite rotation is the Euler rotation which transforms
	 * a point on the surface of the sphere from its present-day
	 * location to its calculated location at some point in the past.
	 * Thus, a finite rotation is associated with a particular point
	 * in time, as well as a particular transformation.
	 *
	 * An Euler rotation is a rotation about an "Euler pole" (a pole
	 * of rotation whose axis extends through the centre of the globe),
	 * by an angular distance.
	 *
	 * Euler poles are specified by a point on the surface of the globe.
	 * [The Euler pole is directed from the centre of the globe to this
	 * point on the surface, and thus may be considered an axial rotation
	 * vector.]
	 *
	 * Rotation angles are specified in radians, with the usual sense of
	 * rotation:  a positive angle represents an anti-clockwise rotation
	 * around the rotation vector;  a negative angle corresponds to a
	 * clockwise rotation.
	 */
	class FiniteRotation
	{
		public:
			/**
			 * Create a finite rotation with the given Euler pole
			 * and rotation angle, for the given point in time.
			 * As always, the rotation angle is in radians; the
			 * point in time is in Ma (millions of years Ago).
			 */
			static FiniteRotation
			CreateFiniteRotation(const PointOnSphere &euler_pole,
			                     const real_t &rotation_angle,
			                     const real_t &point_in_time);

		protected:
			FiniteRotation(const UnitQuaternion &q,
			               const real_t &t)

			 : _quat(q), _time(t) {  }

		private:
			UnitQuaternion3D _quat;
			real_t           _time;  // Millions of years ago
	};

}

#endif  // _GPLATES_MATHS_FINITEROTATION_H_
