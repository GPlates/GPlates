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

#ifndef _GPLATES_MATHS_DIRVECTOR3D_H_
#define _GPLATES_MATHS_DIRVECTOR3D_H_

#include "types.h"  /* real_t */
#include "Vector3D.h"
#include "ViolatedDirVectorInvariantException.h"

namespace GPlatesMaths
{
	/** 
	 * A three-dimensional direction vector.
	 * Thus, the magnitude of this vector must be greater than 0.
	 * This invariant will be enforced upon construction (the values
	 * of x, y and z passed to the constructor will be checked), and
	 * assumed true for all subsequent usage.  No operations must be
	 * provided for this class which would allow the invariant to be
	 * violated.
	 * @invariant
	 *  - magnitude of vector is greater than 0
	 */
	class DirVector3D : public Vector3D
	{
		public:
			/**
			 * Create a 3D direction vector from the specified
			 * x, y and z components.
			 * @param x_comp The x-component.
			 * @param y_comp The y-component.
			 * @param z_comp The z-component.
			 *
			 * @throws ViolatedDirVectorInvariantException if the magnitude
			 *   of the resulting DirVector is <= 0.
			 * 
			 * On a Pentium IV processor, the first line should
			 * cost about (7 + 2 * 2) + (5 + 1) = 17 clock cycles
			 * + the cost of the 'sqrt' function call (presumed
			 * to be at least 47 clock cycles + the cost of two
			 * function calls);  the second should cost about 7
			 * clock cycles + the cost of a function call (as
			 * long as the invariant isn't violated).
			 *
			 * Thus, the creation of a DirVector3D instance will
			 * cost at least 71 clock cycles + the cost of three
			 * function calls.
			 */
			explicit 
			DirVector3D(const real_t& x_comp,
			            const real_t& y_comp,
			            const real_t& z_comp)
			 : Vector3D (x_comp, y_comp, z_comp) {

				UpdateMagnitude();
				AssertInvariant();
			}

			/**
			 * @throws ViolatedDirVectorInvariantException if the magnitude
			 *   of the @a v is <= 0.
			 */
			DirVector3D (const Vector3D &v) : Vector3D (v)
			{
				UpdateMagnitude();
				AssertInvariant();
			}

			virtual real_t
			magnitude() const { return _mag; }

			virtual UnitVector3D normalise() const;

			/**
			 * @throws ViolatedDirVectorInvariantException if the magnitude
			 *   of the @a v is <= 0.
			 */
			DirVector3D &
			operator= (const Vector3D &v)
			{
				_x = v.x ();
				_y = v.y ();
				_z = v.z ();

				UpdateMagnitude();
				AssertInvariant();

				return *this;
			}

		protected:
			/**
			 * Super-secret backdoor constructor for specialised
			 * base classes. The magnitude better be correct!
			 */
			DirVector3D(const real_t &x_comp,
			            const real_t &y_comp,
			            const real_t &z_comp,
				    const real_t &mag)
			 : Vector3D (x_comp, y_comp, z_comp), _mag (mag) {  }

			/**
			 * Another super-secret backdoor constructor for
			 * specialised base classes.  The magnitude better
			 * be correct!
			 */
			DirVector3D(const Vector3D &v, const real_t &mag)
			 : Vector3D (v), _mag (mag) {  }

			/**
			 * Calculate and set the magnitude.
			 */
			void
			UpdateMagnitude()
			{
				_mag = Vector3D::magnitude();
			}

			/** 
			 * Assert the class invariant.
			 * @throw ViolatedDirVectorInvariantException
			 *   if the invariant has been violated.
			 *
			 * Since this is invoked by constructors,
			 * it should not be a virtual function.
			 */
			void AssertInvariant () const;

			real_t _mag;
	};
}

#endif  // _GPLATES_MATHS_DIRVECTOR3D_H_
