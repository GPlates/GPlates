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


	inline bool
	parallel(UnitVector3D v1, UnitVector3D v2) {

		return (v1 == v2);
	}


	inline bool
	antiparallel(UnitVector3D v1, UnitVector3D v2) {

		return (v1.x() == -v2.x()
		     && v1.y() == -v2.y()
		     && v1.z() == -v2.z());
	}


	inline real_t
	operator*(UnitVector3D v1, UnitVector3D v2) {

		return (v1.x() * v2.x()
		      + v1.y() * v2.y()
		      + v1.z() * v2.z());
	}


	inline std::ostream &
	operator<<(std::ostream &os, UnitVector3D v) {

		os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
		return os;
	}


	DirVector3D cross(UnitVector3D v1, UnitVector3D v2);
}

#endif  // _GPLATES_MATHS_UNITVECTOR3D_H_
