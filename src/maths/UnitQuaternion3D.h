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
			// these functions use the protected constructor
			friend UnitQuaternion3D
			operator-(UnitQuaternion3D q);

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
			 *
			 * @throws ViolatedUnitQuatInvariantException if the
			 * resulting quaternion's magnitude is not 1.
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
			 * Return the conjugate of this unit quaternion.
			 *
			 * The conjugate of a quaternion is required for the
			 * multiplicative inverse.
			 */
			UnitQuaternion3D
			conjugate() const {

				return UnitQuaternion3D(_s, -_v);
			}


			/**
			 * Return the inverse of this unit quaternion.
			 *
			 * If a unit quaternion is representing a rotation,
			 * the inverse of that quaternion is the reverse of
			 * the rotation).
			 *
			 * A neat feature of the unit quaternion: its inverse
			 * is identical to its conjugate.
			 */
			UnitQuaternion3D
			inverse() const {

				return conjugate();
			}


			/**
			 * Return whether this unit quaternion represents
			 * an identity rotation (ie. a rotation which maps
			 * a unit vector to itself).
			 */
			bool
			isIdentity() const {

				return (w() == 1.0);
			}


			struct RotationParams
			{
				RotationParams(const UnitVector3D &rot_axis,
				               const real_t &rot_angle) :
				 axis(rot_axis), angle(rot_angle) {  }

				UnitVector3D axis;
				real_t angle;  // in radians
			};


			/**
			 * Calculate the rotation parameters of this unit
			 * quaternion.
			 *
			 * @throws IndeterminateResultException if this
			 * function is invoked upon a unit quaternion
			 * instance which represents an identity rotation.
			 */
			RotationParams
			calcRotationParams() const;


			/**
			 * Create a unit quaternion to represent the following
			 * Euler rotation around the given unit vector @a axis,
			 * by the given rotation angle @a angle.
			 *
			 * As always, the rotation angle is in radians.
			 */
			static UnitQuaternion3D
			CreateEulerRotation(const UnitVector3D &axis, 
			                    const real_t &angle);

		protected:
			/**
			 * Create a 3D unit quaternion from the specified
			 * (s, v) components.
			 * @param s_comp The scalar-component.
			 * @param v_comp The vector-component.
			 *
			 * This constructor is protected because it
			 * <em>assumes</em> that the scalar and vector with
			 * which it is supplied will maintain the invariant.
			 * This constructor will not check the invariant.
			 */
			explicit 
			UnitQuaternion3D(const real_t &s_comp,
			                 const Vector3D &v_comp)
			 : _s(s_comp), _v(v_comp) {  }


			/** 
			 * Assert the class invariant.
			 *
			 * @throw ViolatedUnitQuaternionInvariantException
			 * if the invariant has been violated.
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


	/**
	 * Return the negative of the unit quaternion @a q.
	 *
	 * NOTE that the negative of a quaternion is <em>NOT</em> the same as
	 * its conjugate or inverse.
	 */
	inline UnitQuaternion3D
	operator-(UnitQuaternion3D q) {

		return UnitQuaternion3D(-q.s(), -q.v());
	}


	/**
	 * Return whether these two unit quaternions @a q1 and @a q2 represent
	 * equivalent rotations.
	 */
	inline bool
	representEquivRotations(UnitQuaternion3D q1, UnitQuaternion3D q2) {

		/*
		 * A rotation is defined by an axis of rotation and an angle
		 * through which points are rotated about this axis.  Define
		 * 'theta' to be the angle of rotation, and 'U' to be a unit
		 * vector pointing in the direction of the axis of rotation.
		 * Together, U and theta are used to define a unit quaternion
		 * which describes the rotation.  [Assume theta is contained
		 * in the half-open range (pi, pi].]
		 *
		 * It may be observed that a rotation of theta about U is
		 * equivalent to a rotation of (2 pi - theta) about (-U).
		 * Accordingly, the quaternion 'Q1' (defined by theta and U)
		 * describes a rotation equivalent to that described by the
		 * quaternion 'Q2' (defined by (2 pi - theta) and (-U)).
		 *
		 * In fact, it may be shown that Q2 is equivalent to (-Q1).
		 */
		return (q1 == q2 || q1 == -q2);
	}


	inline std::ostream &
	operator<<(std::ostream &os, UnitQuaternion3D v) {

		os << "(" << v.x() << ", " << v.y() << ", " << v.z() << ", "
		 << v.w() <<  ")";
		return os;
	}


	/**
	 * Multiply quaternion @a q1 with @a q2.
	 *
	 * Note that, in the context of rotations, quaternion multiplication
	 * behaves a lot like matrix multiplication: it can be considered a
	 * <em>composition</em> of the quaternions, in the sense that @a q1
	 * is being applied to @a q2.  This is known as "premultiplication".
	 */
	UnitQuaternion3D operator*(UnitQuaternion3D q1, UnitQuaternion3D q2);
}

#endif  // _GPLATES_MATHS_UNITQUATERNION3D_H_
