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

#ifndef _GPLATES_MATHS_VECTOR3D_H_
#define _GPLATES_MATHS_VECTOR3D_H_

#include <ostream>
#include "types.h"  /* real_t */


namespace GPlatesMaths
{
	class UnitVector3D;

	/** 
	 * A three-dimensional vector.
	 * In contrast to UnitVector3D and DirVector3D, there are no
	 * conditions upon this vector.  Its components may take any values.
	 * It may be of any magnitude.  Hence, there is no invariant which
	 * must be maintained.
	 */
	class Vector3D
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
			Vector3D(const real_t& x_comp,
			         const real_t& y_comp,
			         const real_t& z_comp)
			 : _x(x_comp), _y(y_comp), _z(z_comp) {  }


			virtual
			~Vector3D() {  }


			const real_t &
			x() const { return _x; }

			const real_t &
			y() const { return _y; }

			const real_t &
			z() const { return _z; }

			/**
			 * Returns the square of the magnitude; that is,
			 * \f$ ( x^2 + y^2 + z^2 ) \f$
			 */
			real_t
			magSqrd() const {

				return ((_x * _x) + (_y * _y) + (_z * _z));
			}

			/**
			 * Returns the magnitude of the vector; that is,
			 * \f$ \sqrt{x^2 + y^2 + z^2} \f$
			 */
			virtual real_t
			magnitude() const {

				return sqrt(magSqrd());
			}

			/**
			 * Generate a vector having the same direction as @a this,
			 * but which has unit magnitude.
			 *
			 * @throws IndeterminateResultException if @a this has zero
			 *   magnitude.
			 */
			virtual UnitVector3D get_normalisation () const;

		protected:
			real_t _x,  /**< x-component. */
			       _y,  /**< y-component. */
			       _z;  /**< z-component. */
	};


	inline bool
	operator==(const Vector3D &v1, const Vector3D &v2) {

		return (v1.x() == v2.x()
		     && v1.y() == v2.y()
		     && v1.z() == v2.z());
	}


	inline bool
	operator!=(const Vector3D &v1, const Vector3D &v2) {

		return (v1.x() != v2.x()
		     || v1.y() != v2.y()
		     || v1.z() != v2.z());
	}


	inline real_t
	dot(const Vector3D &v1, const Vector3D &v2) {

		real_t x_dot = v1.x() * v2.x();
		real_t y_dot = v1.y() * v2.y();
		real_t z_dot = v1.z() * v2.z();

		return (x_dot + y_dot + z_dot);
	}


	inline Vector3D
	operator-(const Vector3D &v) {

		return Vector3D(-v.x(), -v.y(), -v.z());
	}


	inline Vector3D
	operator*(real_t r, const Vector3D &v) {

		return Vector3D(r * v.x(), r * v.y(), r * v.z());
	}


	inline Vector3D
	operator*(const Vector3D &v, real_t r) {

		return (r * v);
	}


	inline Vector3D
	operator+(const Vector3D &v1, const Vector3D &v2) {

		return Vector3D(v1.x() + v2.x(),
		                v1.y() + v2.y(),
		                v1.z() + v2.z());
	}


	inline Vector3D
	operator-(const Vector3D &v1, const Vector3D &v2) {

		return Vector3D(v1.x() - v2.x(),
		                v1.y() - v2.y(),
		                v1.z() - v2.z());
	}


	inline std::ostream &
	operator<< (std::ostream &os, const Vector3D &v)
	{
		os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
		return os;
	}


	/**
	 * This algorithm for testing whether two vectors are parallel
	 * is intended to remove the requirements that:
	 *  - the magnitudes of the vectors are already known
	 *  - any of the components are non-zero.
	 *
	 * Note that zero vectors are defined to be parallel to everything.
	 */
	inline bool
	parallel(const Vector3D &v1, const Vector3D &v2)
	{
		real_t dp = dot(v1, v2);
		return (dp >= v1.magnitude () * v2.magnitude ());
	}

	/**
	 * Test whether two vectors are perpendicular.
	 */
	inline bool
	perpendicular (const Vector3D &v1, const Vector3D &v2)
	{
		real_t dp = dot (v1, v2);
		return (abs (dp) <= 0.0);
	}


	/**
	 * This algorithm for testing whether two vectors are collinear
	 * (ie. parallel or antiparallel) is intended to remove the
	 * requirements that:
	 *  - the magnitudes of the vectors are already known
	 *  - any of the components are non-zero.
	 *
	 * Note that zero vectors are defined to be collinear to everything.
	 */
	inline bool
	collinear(const Vector3D &v1, const Vector3D &v2)
	{
		real_t adp = abs (dot (v1, v2));
		return (adp >= v1.magnitude () * v2.magnitude ());
	}


	Vector3D cross (const Vector3D &s1, const Vector3D &s2);
}

#endif  // _GPLATES_MATHS_VECTOR3D_H_
