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

#ifndef _GPLATES_MATHS_VECTOR3D_H_
#define _GPLATES_MATHS_VECTOR3D_H_

#include "types.h"  /* real_t */

namespace GPlatesMaths
{
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
			 : _x(x_comp), _y(y_comp), _z(z_comp) {

				// Calculate magnitude of vector.
				_mag = sqrt((_x * _x) + (_y * _y) + (_z * _z));
			}

			real_t
			x() const { return _x; }

			real_t
			y() const { return _y; }

			real_t
			z() const { return _z; }

			real_t
			magnitude() const { return _mag; }

		private:
			real_t _x,  /**< x-component. */
			       _y,  /**< y-component. */
			       _z;  /**< z-component. */

			real_t _mag;
	};


	inline bool
	operator==(Vector3D v1, Vector3D v2) {

		return (v1.x() == v2.x()
		     && v1.y() == v2.y()
		     && v1.z() == v2.z());
	}


	inline bool
	operator!=(Vector3D v1, Vector3D v2) {

		return (v1.x() != v2.x()
		     || v1.y() != v2.y()
		     || v1.z() != v2.z());
	}


	inline Vector3D
	operator-(Vector3D v) {

		return Vector3D(-v.x(), -v.y(), -v.z());
	}


	inline Vector3D
	operator*(real_t r, Vector3D v) {

		return Vector3D(r * v.x(), r * v.y(), r * v.z());
	}


	inline Vector3D
	operator*(Vector3D v, real_t r) {

		return (r * v);
	}


	inline Vector3D
	operator+(Vector3D v1, Vector3D v2) {

		return Vector3D(v1.x() + v2.x(),
		                v1.y() + v2.y(),
		                v1.z() + v2.z());
	}


	inline real_t
	operator*(Vector3D v1, Vector3D v2) {

		return (v1.x() * v2.x()
		      + v1.y() * v2.y()
		      + v1.z() * v2.z());
	}


	Vector3D cross(Vector3D v1, Vector3D v2);
}

#endif  // _GPLATES_MATHS_VECTOR3D_H_
