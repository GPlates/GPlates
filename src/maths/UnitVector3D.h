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

#ifndef _GPLATES_MATHS_UNITVECTOR3D_H_
#define _GPLATES_MATHS_UNITVECTOR3D_H_

#include <iostream>
#include "types.h"  /* real_t */
#include "DirVector3D.h"


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
	class UnitVector3D
	{
		public:
			/**
			 * Create a 3D unit vector from the specified
			 * x, y and z components.
			 * @param x_comp The x-component.
			 * @param y_comp The y-component.
			 * @param z_comp The z-component.
			 *
			 * On a Pentium IV processor, the creation of a
			 * UnitVector3D instance will cost about 26 clock
			 * cycles + the cost of a function call (assuming
			 * the invariant isn't violated).
			 */
			explicit 
			UnitVector3D(const real_t& x_comp,
			             const real_t& y_comp,
			             const real_t& z_comp)
				: _x(x_comp), _y(y_comp), _z(z_comp) {
				
				AssertInvariantHolds();
			}

			explicit
			UnitVector3D(DirVector3D v)
				: _x(v.x()), _y(v.y()), _z(v.z()) {

				AssertInvariantHolds();
			}

			real_t
			x() const { return _x; }

			real_t
			y() const { return _y; }

			real_t
			z() const { return _z; }

		protected:
			/** 
			 * Assert the class invariant.
			 * @throw ViolatedUnitVectorInvariantException
			 *   if the invariant has been violated.
			 */
			void
			AssertInvariantHolds() const;

		private:
			real_t _x,  /**< x-component. */
			       _y,  /**< y-component. */
			       _z;  /**< z-component. */
	};


	/**
	 * On a Pentium IV processor, this should cost about
	 * (3 * 9) + (2 * 1) = 29 clock cycles.
	 */
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


	/**
	 * On a Pentium IV processor, this should cost about
	 * (7 + 2 * 2) + (5 + 1) = 17 clock cycles.
	 */
	inline real_t
	dot(UnitVector3D v1, UnitVector3D v2) {

		real_t x_dot = v1.x() * v2.x();
		real_t y_dot = v1.y() * v2.y();
		real_t z_dot = v1.z() * v2.z();

		return (x_dot + y_dot + z_dot);
	}


	/**
	 * On a Pentium IV processor, this should cost about
	 * 17 + 7 = 24 clock cycles.
	 */
	inline bool
	parallel(UnitVector3D v1, UnitVector3D v2) {

		return (dot(v1, v2) >= 1.0);
	}


	/**
	 * On a Pentium IV processor, this should cost about
	 * 17 + 7 = 24 clock cycles.
	 */
	inline bool
	antiparallel(UnitVector3D v1, UnitVector3D v2) {

		return (dot(v1, v2) <= -1.0);
	}


	/**
	 * On a Pentium IV processor, this should cost about
	 * (7 + 2 * 2) + (5 + 1) + 7 = 24 clock cycles.
	 */
	inline bool
	parallel(UnitVector3D uv, DirVector3D dv) {

		real_t x_dot = uv.x() * dv.x();
		real_t y_dot = uv.y() * dv.y();
		real_t z_dot = uv.z() * dv.z();

		return (x_dot + y_dot + z_dot >= dv.magnitude());
	}


	inline bool
	parallel(DirVector3D dv, UnitVector3D uv) {

		return parallel(uv, dv);
	}


	inline std::ostream &
	operator<<(std::ostream &os, UnitVector3D v) {

		os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
		return os;
	}


	/**
	 * On a Pentium IV processor, it should cost about
	 * (7 + 5 * 2) + (5 + 2 * 2) = 26 clock cycles to calculate the
	 * components of the resultant vector, and at least 62 cycles
	 * + the cost of two function calls, to create the DirVector3D
	 * (assuming these two unit vectors are neither parallel nor
	 * antiparallel, which would result in a vector of zero length
	 * and indeterminate direction, which would violate the invariant
	 * of the DirVector3D, which would result in exception-throwing
	 * happy crazy fun!!!!!!1).
	 *
	 * So, there you have it, folks: at least 88 clock cycles + the cost
	 * of two function calls.
	 */
	DirVector3D cross(UnitVector3D v1, UnitVector3D v2);
}

#endif  // _GPLATES_MATHS_UNITVECTOR3D_H_
