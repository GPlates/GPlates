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

#ifndef _GPLATES_MATHS_AXIAL_H_
#define _GPLATES_MATHS_AXIAL_H_

#include "types.h"
#include "UnitVector3D.h"
#include "PointOnSphere.h"
#include "Rotation.h"

namespace GPlatesMaths
{
	/** 
	 * An object on a unit sphere which possesses an axial characteristic.
	 *
	 * This class will be the base class of an inheritance hierarchy
	 * containing the classes 'GreatCircle' and 'SmallCircle'.  NOTE:
	 * that polymorphism is not supposed to occur in the aforementioned
	 * inheritance hierarchy (since small circles and great circles
	 * are both <em>specialisations</em> of this class rather than
	 * <em>subtypes</em>, which would violate substitutability).
	 * The classes 'GreatCircle' and 'SmallCircle' will inherit from
	 * this class simply for the purpose of good, old-fashioned code-reuse.
	 */
	class Axial
	{
		public:
			/**
			 * Create an axial object, given its axis.
			 * @param axis The axis vector.
			 */
			Axial(const UnitVector3D &axis)
			 : _axisvector(axis) {  }

			/**
			 * The unit vector of the axis of this object.
			 */
			UnitVector3D
			axisvector() const { return _axisvector; }

			/**
			 * Given a point @a pt, return the new location
			 * of the point after it has been rotated by the
			 * angle @a rot_angle about the axis of this object.
			 */
			PointOnSphere
			rotateAboutAxis(const PointOnSphere &pt,
			                const real_t &rot_angle) const {

				Rotation r =
				 Rotation::Create(axisvector(), rot_angle);
				return PointOnSphere(r * pt);
			}

		private:
			UnitVector3D _axisvector;
	};
}

#endif  // _GPLATES_MATHS_AXIAL_H_
