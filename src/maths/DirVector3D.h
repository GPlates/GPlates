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

#ifndef _GPLATES_MATHS_DIRVECTOR3D_H_
#define _GPLATES_MATHS_DIRVECTOR3D_H_

#include "global/types.h"  /* real_t */
#include "ViolatedDirVectorInvariantException.h"

namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	// forward declaration for 'DirVector3D::normalise'
	class UnitVector3D;


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
	class DirVector3D
	{
		public:
			/**
			 * Create a 3D direction vector from the specified
			 * x, y and z components.
			 * @param x The x-component.
			 * @param y The y-component.
			 * @param z The z-component.
			 */
			explicit 
			DirVector3D(const real_t& x,
			            const real_t& y,
			            const real_t& z)
				: _x(x), _y(y), _z(z) {
				
				AssertInvariantHolds();
			}

			real_t
			x() const { return _x; }

			real_t
			y() const { return _y; }

			real_t
			z() const { return _z; }

			real_t
			magnitude() const { return _mag; }

			UnitVector3D
			normalise() const;

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

			real_t _mag;
	};


	inline bool
	operator==(DirVector3D v1, DirVector3D v2) {

		return (v1.x() == v2.x()
		     && v1.y() == v2.y()
		     && v1.z() == v2.z());
	}


	inline bool
	operator!=(DirVector3D v1, DirVector3D v2) {

		return (v1.x() != v2.x()
		     || v1.y() != v2.y()
		     || v1.z() != v2.z());
	}


	inline real_t
	operator*(DirVector3D v1, DirVector3D v2) {

		return (v1.x() * v2.x()
		      + v1.y() * v2.y()
		      + v1.z() * v2.z());
	}


	DirVector3D cross(DirVector3D v1, DirVector3D v2);
}

#endif  // _GPLATES_MATHS_DIRVECTOR3D_H_
