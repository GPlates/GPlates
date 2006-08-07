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
			 *
			 * @throw ViolatedUnitVectorInvariantException if the
			 *   resulting vector does not have unit magnitude.
			 */
			explicit
			UnitVector3D(const real_t &x_comp,
				     const real_t &y_comp,
				     const real_t &z_comp)
				: DirVector3D (x_comp, y_comp, z_comp, 1.0)
			{
				AssertInvariant (__LINE__);
			}

			/**
			 * @throw ViolatedUnitVectorInvariantException if @a v
			 *   does not have unit magnitude.
			 */
			UnitVector3D (const Vector3D &v) : DirVector3D (v, 1.0)
			{
				AssertInvariant (__LINE__);
			}

			virtual
			~UnitVector3D() {  }

			/**
			 * @throw ViolatedUnitVectorInvariantException if @a v
			 *   does not have unit magnitude.
			 */
			UnitVector3D &
			operator= (const Vector3D &v)
			{
				_x = v.x ();
				_y = v.x ();
				_z = v.x ();

				AssertInvariant (__LINE__);
				return *this;
			}

			virtual real_t
			magnitude() const { return 1.0; }

			virtual UnitVector3D
			get_normalisation() const {

				return *this;
			}

			static inline UnitVector3D
			xBasis() {

				return UnitVector3D(1.0, 0.0, 0.0, 1.0);
			}

			static inline UnitVector3D
			yBasis() {

				return UnitVector3D(0.0, 1.0, 0.0, 1.0);
			}

			static inline UnitVector3D
			zBasis() {

				return UnitVector3D(0.0, 0.0, 1.0, 1.0);
			}

		protected:
			/**
			 * Super-secret backdoor constructor for the
			 * @a xBasis, @a yBasis and @a zBasis functions.
			 * The magnitude @a mag will be ignored.
			 *
			 * Those components had better define a unit vector!
			 */
			explicit
			UnitVector3D(const real_t &x_comp,
			             const real_t &y_comp,
			             const real_t &z_comp,
			             const real_t &mag) :

				DirVector3D (x_comp, y_comp, z_comp, 1.0) {  }


			/** 
			 * Assert the class invariant.
			 * @throw ViolatedUnitVectorInvariantException
			 *   if the invariant has been violated.
			 *
			 * Since this is invoked by constructors,
			 * it should not be a virtual function.
			 */
			void AssertInvariant (int line) const;
	};


	inline bool
	collinear (const UnitVector3D &s1, const UnitVector3D &s2)
	{
		real_t adp = abs (dot (s1, s2));
		return (adp >= 1.0);
	}


	/**
	 * Given the unit vector @a u, generate a unit vector perpendicular
	 * to it.
	 */
	UnitVector3D generatePerpendicular(const UnitVector3D &u);
}

#endif  // _GPLATES_MATHS_UNITVECTOR3D_H_
