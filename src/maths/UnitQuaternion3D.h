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

#ifndef _GPLATES_MATHS_UNITQUATERNION3D_H_
#define _GPLATES_MATHS_UNITQUATERNION3D_H_

#include <iostream>
#include "types.h"  /* real_t */
#include "Vector3D.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * A three-dimensional unit quaternion.
	 *
	 * For more information on quaternions, see:
	 *  - MathWorld:
	 *     http://mathworld.wolfram.com/Quaternion.html
	 *  - Wikipedia
	 *     http://www.wikipedia.org/wiki/Quaternion
	 *
	 * To quote briefly from Wikipedia:
	 *
	 *   Quaternions are sometimes used in computer graphics (and
	 *   associated geometric analysis) to represent rotations or
	 *   orientations of objects in 3d space.  The advantages are:
	 *   non-singular representation (compared with Euler angles
	 *   for example), more compact (and faster) than matrices
	 *   [since matrices can represent all sorts of transformations,
	 *   but (unit) quaternions can only represent rotations -- JB].
	 *
	 *   The set of all unit quaternions forms a 3-dimensional
	 *   sphere S^3 and a group (even a Lie group) under
	 *   multiplication.  S^3 is the double cover of the group
	 *   SO(3,R) of real orthogonal 3x3 matrices of determinant 1
	 *   since two unit quaternions correspond to every rotation
	 *   under the above correspondence.
	 *
	 *
	 * The components of this quaternion will be x, y, z and w,
	 * where a quaternion Q == (x, y, z, w) == w + xi + yj + zk.
	 *
	 * Alternately, if Q == (s, v) [where s is a scalar and v is
	 * a vector], then s == w and v == (x, y, z).
	 *
	 * Since this is a unit quaternion, its magnitude must always be
	 * identical to 1.  This invariant will be enforced upon construction
	 * (the values of w, x, y and z passed to the constructor will be
	 * checked), and assumed true for all subsequent usage.  No operations
	 * may be provided for this class which would allow the invariant to
	 * be violated.
	 * @invariant
	 *  - magnitude of quaternion is identical to 1
	 */
	class UnitQuaternion3D
	{
			// this function uses the protected constructor
			friend UnitQuaternion3D
			operator*(UnitQuaternion3D q1, UnitQuaternion3D q2);

		public:
			/**
			 * Create a 3D unit quaternion from the specified
			 * x, y, z and w components.
			 * @param x_comp The x-component.
			 * @param y_comp The y-component.
			 * @param z_comp The z-component.
			 * @param w_comp The w-component.
			 */
			explicit 
			UnitQuaternion3D(const real_t &x_comp,
			                 const real_t &y_comp,
			                 const real_t &z_comp,
			                 const real_t &w_comp)
			 : _s(w_comp), _v(x_comp, y_comp, z_comp) {

				AssertInvariantHolds();
			}


			/*
			 * Access quaternion as Q = (s, v).
			 */
			real_t
			s() const { return _s; }

			Vector3D
			v() const { return _v; }


			/*
			 * Access quaternion as Q = (x, y, z, w).
			 */
			real_t
			x() const { return _v.x(); }

			real_t
			y() const { return _v.y(); }

			real_t
			z() const { return _v.z(); }

			real_t
			w() const { return _s; }


			/**
			 * The conjugate of a quaternion is required
			 * for the multiplicative inverse.
			 */
			UnitQuaternion3D
			conjugate() const { return UnitQuaternion3D(_s, -_v); }


			/**
			 * If a unit quaternion is representing a rotation,
			 * the inverse of that quaternion is the reverse of
			 * the rotation).
			 *
			 * A neat feature of unit quaternions:
			 * its inverse is identical to its conjugate.
			 */
			UnitQuaternion3D
			inverse() const { return conjugate(); }


			/**
			 * Create a unit quaternion to represent the following
			 * Euler rotation around the unit vector of the Euler
			 * pole, by the given rotation angle.
			 *
			 * As always, the rotation angle is in radians.
			 */
			static UnitQuaternion3D
			CreateEulerRotation(const UnitVector3D &euler_pole, 
			                    const real_t &rotation_angle);

		protected:
			/**
			 * Create a 3D unit quaternion from the specified
			 * (s, v) components.
			 * @param s_comp The scalar-component.
			 * @param v_comp The vector-component.
			 *
			 * This constructor is protected because it
			 * <b>assumes</b> that the scalar and vector
			 * with which it is supplied will maintain
			 * the invariant.  This constructor will not
			 * check the invariant.
			 */
			explicit 
			UnitQuaternion3D(const real_t &s_comp,
			                 const Vector3D &v_comp)
			 : _s(s_comp), _v(v_comp) {  }


			/** 
			 * Assert the class invariant.
			 * @throw ViolatedUnitQuaternionInvariantException
			 *   if the invariant has been violated.
			 */
			void
			AssertInvariantHolds() const;

		private:
			real_t   _s;
			Vector3D _v;
	};


	inline bool
	operator==(UnitQuaternion3D q1, UnitQuaternion3D q2) {

		return (q1.x() == q2.x()
		     && q1.y() == q2.y()
		     && q1.z() == q2.z()
		     && q1.w() == q2.w());
	}


	inline bool
	operator!=(UnitQuaternion3D q1, UnitQuaternion3D q2) {

		return (q1.x() != q2.x()
		     || q1.y() != q2.y()
		     || q1.z() != q2.z()
		     || q1.w() != q2.w());
	}


	inline std::ostream &
	operator<<(std::ostream &os, UnitQuaternion3D v) {

		os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ", "
		 << v.w() <<  ")";
		return os;
	}


	UnitQuaternion3D operator*(UnitQuaternion3D q1, UnitQuaternion3D q2);
}

#endif  // _GPLATES_MATHS_UNITQUATERNION3D_H_
