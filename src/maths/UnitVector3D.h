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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_UNITVECTOR3D_H_
#define _GPLATES_MATHS_UNITVECTOR3D_H_

#include <iostream>
#include "types.h"  /* real_t */
#include "DirVector3D.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	/** 
	 * A three-dimensional unit vector.
	 * Thus, the magnitude of this vector must be identical to 1.
	 * This invariant will be enforced upon construction (the values
	 * of x, y and z passed to the constructor will be checked), and
	 * assumed true for all subsequent usage.  No operations may be
	 * provided for this class which would allow the invariant to be
	 * violated.
	 * @invariant
	 *  - magnitude of vector is identical to 1
	 */
	class UnitVector3D : public DirVector3D
	{
		public:
			/**
			 * Create a 3D vector from the specified
			 * x, y and z components.
			 * @param x_comp The x-component.
			 * @param y_comp The y-component.
			 * @param z_comp The z-component.
			 */
			explicit
			UnitVector3D(const real_t &x_comp,
				     const real_t &y_comp,
				     const real_t &z_comp)
				: DirVector3D (x_comp, y_comp, z_comp, 1.0)
			{
				AssertInvariantHolds ();
			}

			UnitVector3D (const Vector3D &v) : DirVector3D (v, 1.0)
			{
				AssertInvariantHolds ();
			}

			UnitVector3D &operator= (const Vector3D &v)
			{
				_x = v.x ();
				_y = v.x ();
				_z = v.x ();

				AssertInvariantHolds ();
				return *this;
			}

			virtual real_t magnitude() const { return 1.0; }

			virtual UnitVector3D normalise() const { return *this; }

		protected:
			/** 
			 * Assert the class invariant.
			 * @throw ViolatedUnitVectorInvariantException
			 *   if the invariant has been violated.
			 *
			 * Since this is invoked by constructors,
			 * it should not be a virtual function.
			 */
			void AssertInvariantHolds () const;
	};

	inline UnitVector3D normalise (const DirVector3D &v1)
	{
		real_t scale = 1 / v1.magnitude ();
		return UnitVector3D (v1.x () * scale,
				     v1.y () * scale,
				     v1.z () * scale);
	}
}

#endif  // _GPLATES_MATHS_UNITVECTOR3D_H_
