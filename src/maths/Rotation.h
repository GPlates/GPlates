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

#ifndef _GPLATES_MATHS_ROTATION_H_
#define _GPLATES_MATHS_ROTATION_H_

#include "PointOnSphere.h"
#include "types.h"  /* real_t */

namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	/** 
	 * Rotation of a PointOnSphere.
	 * Function class that represents the Euler rotation of a point 
	 * around a pole by a particular angle (in radians) on the surface 
	 * of a sphere. Rotations are counter-clockwise around the pole when 
	 * the angle is positive, and clockwise when the angle is negative.
	 * @todo Confirm that rotations are indeed counter-clockwise.
	 */
	class Rotation
	{
		public:
			/** 
			 * Create a rotation with pole (0, 0) and angle 0.
			 */
			Rotation() : _angle(0.0) { }

			/** 
			 * Create a rotation with the given pole and angle.
			 */
			Rotation(const PointOnSphere& pole,
			         const real_t& angle)
				: _pole(pole), _angle(angle) { }

			/** 
			 * @return A copy of the pole of rotation. 
			 */
			PointOnSphere
			GetPole() const { return _pole; }

			/** 
			 * @return A copy of the angle of rotation.
			 */
			real_t
			GetAngle() const { return _angle; }

			/** 
			 * Rotate the given PointOnSphere.
			 * @param point The point to which the rotation should be applied.
			 * @return The rotated point.
			 * @todo Yet to be implemented. 
			 */
			PointOnSphere
			ApplyTo(const PointOnSphere& point) const;

			/** 
			 * Compose this Rotation with another.
			 * @warning Composition does not commute, i.e. if A and B are
			 *   Rotations, then A + B != B + A in general.
			 * @todo Yet to be implemented. 
			 * @see Greiner, pp211-3.
			 */
			Rotation&
			operator+=(const Rotation&) { return *this; }
			
		private:
			PointOnSphere _pole;
			real_t _angle;
	};

	/**
	 * Non-member alternative to Rotation::operator+=.
	 * @return A rotation that is the 'rotational sum' of @a A and @a B .
	 */
	Rotation
	operator+(const Rotation& A, const Rotation& B);
}

#endif  // _GPLATES_MATHS_ROTATION_H_
