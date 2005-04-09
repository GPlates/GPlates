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

#ifndef GPLATES_MATHS_UNITQUATERNION3D_H
#define GPLATES_MATHS_UNITQUATERNION3D_H

#include <iosfwd>

#include "types.h"  /* real_t */
#include "Vector3D.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * A unit quaternion with three-dimensional operations.
	 *
	 * @section Overview
	 *
	 * Unit quaternions are used in this context to efficiently calculate
	 * rotations about arbitrarily-oriented rotation axes.
	 *
	 * To quote from Wikipedia (Wikipedia05a, Wikipedia05b):
	 *
	 * @par
	 *   Quaternions are often used in computer graphics (and associated
	 *   geometric analysis) to represent rotations (see quaternions and
	 *   spatial rotation) and orientations of objects in 3d space. They
	 *   are smaller than other representations such as matrices, and
	 *   operations on them such as composition can be computed more
	 *   efficiently.
	 *
	 * @par
	 *   The representation of a rotation as a quaternion (4 numbers) is
	 *   more compact than the representation as an orthogonal matrix (9
	 *   numbers). Furthermore, for a given axis and angle, one can easily
	 *   construct the corresponding quaternion, and conversely, for a
	 *   given quaternion one can easily read off the axis and the angle.
	 *   Both of these are much harder with matrices or Euler angles.
	 *
	 * @par
	 *   In computer games and other applications, one is often interested
	 *   in smooth rotations, meaning that the scene should slowly rotate
	 *   and not in a single step. This can be accomplished by choosing a
	 *   curve in the quaternions, with one endpoint being the identity
	 *   transformation 1 and the other being the intended total rotation.
	 *   This is more problematic with other representations of rotations.
	 *
	 * @section Details
	 *
	 * To quote a little more from Wikipedia (Wikipedia05a):
	 *
	 * @par
	 *   The set of all unit quaternions forms a 3-dimensional sphere S^3
	 *   and a group (a Lie group) under multiplication.  S^3 is the double
	 *   cover of the group SO(3,R) of real orthogonal 3x3 matrices of
	 *   determinant 1 since two unit quaternions correspond to every
	 *   rotation under the above correspondence.
	 *
	 * Quaternions form a "noncommutative division algebra" (Weisstein05b).
	 * In practical terms, this tells us (Weisstein05a):
	 *  - multiplication is associative but NOT commutative.
	 *  - there exists a multiplicative identity.
	 *  - for every non-zero element there exists a multiplicative inverse.
	 *
	 * The components of this quaternion will be x, y, z and w, where a
	 * quaternion Q = (w, x, y, z) = w + xi + yj + zk.
	 *
	 * Alternately, if Q is considered as a duple (scalar, vector), then
	 * scalar = w and vector = (x, y, z).
	 *
	 * Since this is a unit quaternion, its magnitude (norm) must always be
	 * identical to 1.  This invariant will be enforced upon construction
	 * and assumed true for all subsequent usage.  No operations should be
	 * provided for this class which would allow the invariant to be
	 * violated.
	 *
	 * @invariant
	 *  - magnitude (norm) of quaternion is identical to 1
	 *
	 * @section Bibliography
	 *
	 * The following references are either cited in the documentation or
	 * played a significant role in the design and implementation of this
	 * class:
	 *  - Kuipers02: Jack B. Kuipers, <i>Quaternions and Rotation
	 *     Sequences</i>, Princeton University Press, 2002.
	 *  - Weisstein05a: Eric W. Weisstein, "Division Algebra".  From
	 *     <i>MathWorld</i> -- A Wolfram Web Resource [online].
	 *     http://mathworld.wolfram.com/DivisionAlgebra.html  [Accessed 9
	 *     April 2005]
	 *  - Weisstein05b: Eric W. Weisstein, "Quaternion".  From
	 *     <i>MathWorld</i> -- A Wolfram Web Resource [online].
	 *     http://mathworld.wolfram.com/Quaternion.html  [Accessed 9 April
	 *     2005]
	 *  - Wikipedia05a: Wikipedia, "Quaternion".  From <i>Wikipedia</i>
	 *     [online].  http://en.wikipedia.org/wiki/Quaternion  [Accessed 9
	 *     April 2005]
	 *  - Wikipedia05b: Wikipedia, "Quaternions and spatial rotation".
	 *     From <i>Wikipedia</i> [online].
	 *     http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
	 *     [Accessed 9 April 2005]
	 *
	 * CVS informs us that this class was first committed in September
	 * 2003, by which time all these references (with the exception of
	 * Kuipers02) had already been consulted.  However, they were not
	 * properly cited until the 9th of April, 2005.
	 */
	class UnitQuaternion3D {

		/*
		 * These functions use the protected constructor.
		 */

		friend
		const UnitQuaternion3D
		operator-(
		 const UnitQuaternion3D &q);

		friend
		const UnitQuaternion3D
		operator*(
		 const UnitQuaternion3D &q1,
		 const UnitQuaternion3D &q2);

	 public:

		/**
		 * Access quaternion as the duple (scalar, vector).
		 */
		const real_t &
		scalar_part() const {
			
			return m_scalar_part;
		}


		/**
		 * Access quaternion as the duple (scalar, vector).
		 */
		const Vector3D &
		vector_part() const {
			
			return m_vector_part;
		}


		/**
		 * Access quaternion as the real 4-tuple (w, x, y, z).
		 */
		const real_t &
		w() const {
			
			return m_scalar_part;
		}


		/**
		 * Access quaternion as the real 4-tuple (w, x, y, z).
		 */
		const real_t &
		x() const {
			
			return m_vector_part.x();
		}


		/**
		 * Access quaternion as the real 4-tuple (w, x, y, z).
		 */
		const real_t &
		y() const {
			
			return m_vector_part.y();
		}


		/**
		 * Access quaternion as the real 4-tuple (w, x, y, z).
		 */
		const real_t &
		z() const {
			
			return m_vector_part.z();
		}


		/**
		 * Return the conjugate of this unit quaternion.
		 *
		 * This operation is used in the calculation of the
		 * multiplicative inverse.
		 */
		const UnitQuaternion3D
		get_conjugate() const {

			return UnitQuaternion3D(m_scalar_part, -m_vector_part);
		}


		/**
		 * Return the multiplicative inverse of this unit quaternion.
		 *
		 * If a unit quaternion is representing a rotation, the inverse
		 * of that quaternion is the reverse of the rotation).
		 *
		 * A neat feature of the unit quaternion: its inverse is
		 * identical to its conjugate.
		 */
		const UnitQuaternion3D
		get_inverse() const {

			return get_conjugate();
		}


		struct RotationParams
		{
			RotationParams(
			 const UnitVector3D &rot_axis,
			 real_t rot_angle) :
			 axis(rot_axis),
			 angle(rot_angle) {  }

			UnitVector3D axis;
			real_t angle;  // in radians
		};


		/**
		 * Calculate the rotation parameters of this unit quaternion.
		 *
		 * @throws IndeterminateResultException if this function is
		 * invoked upon a unit quaternion instance which represents an
		 * identity rotation.
		 */
		const RotationParams
		get_rotation_params() const;


		/**
		 * Create a unit quaternion to represent the following rotation
		 * around the given unit vector @a axis, by the given rotation
		 * angle @a angle.
		 *
		 * As always, the rotation angle is in radians.
		 */
		static
		const UnitQuaternion3D
		create_rotation(
		 const UnitVector3D &axis, 
		 real_t angle);

	 protected:

		/**
		 * Create a unit quaternion composed of the specified
		 * (scalar, vector) parts.
		 *
		 * @param s The scalar part.
		 * @param v The vector part.
		 *
		 * This constructor is protected because it <em>assumes</em>
		 * that the scalar and vector with which it is supplied will
		 * maintain the invariant.  Again, this constructor does NOT
		 * check the invariant.
		 */
		UnitQuaternion3D(
		 real_t s,
		 const Vector3D &v) :
		 m_scalar_part(s),
		 m_vector_part(v) {  }


		/**
		 * Calculate the square of the <em>actual</em> norm of this
		 * quaternion (rather than just assuming it is equal to 1).
		 *
		 * This operation is used in the assertion of the class
		 * invariant.
		 */
		const real_t
		get_actual_norm_sqrd() const {

			return
			 (scalar_part() * scalar_part() +
			  dot(vector_part(), vector_part()));
		}


		/** 
		 * Assert the class invariant.
		 *
		 * @throw ViolatedClassInvariantException if the invariant has
		 * been violated.
		 */
		void
		assert_invariant() const;

	 private:

		real_t m_scalar_part;

		Vector3D m_vector_part;

	};


	/**
	 * Return whether these two unit quaternions @a q1 and @a q2 are equal.
	 *
	 * NOTE that this does not imply that they represent equivalent
	 * rotations.  For that, use the function @a represent_equiv_rotations.
	 */
	inline
	bool
	operator==(
	 const UnitQuaternion3D &q1,
	 const UnitQuaternion3D &q2) {

		return (q1.x() == q2.x()
		     && q1.y() == q2.y()
		     && q1.z() == q2.z()
		     && q1.w() == q2.w());
	}


	/**
	 * Return whether these two unit quaternions @a q1 and @a q2 are
	 * not equal.
	 *
	 * NOTE that this does not imply that they do not represent equivalent
	 * rotations.  For that, use the function @a represent_equiv_rotations.
	 */
	inline
	bool
	operator!=(
	 const UnitQuaternion3D &q1,
	 const UnitQuaternion3D &q2) {

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
	 *
	 * This operation is used in the test of whether two quaternions
	 * represent equivalent rotations.
	 */
	inline
	const UnitQuaternion3D
	operator-(
	 const UnitQuaternion3D &q) {

		return UnitQuaternion3D(-q.scalar_part(), -q.vector_part());
	}


	/**
	 * Return whether this unit quaternion @a q represents an identity
	 * rotation (ie. a rotation which maps a vector to itself).
	 */
	inline
	bool
	represents_identity_rotation(
	 const UnitQuaternion3D &q) {

		/*
		 * An identity rotation: theta = n * 2 * PI.
		 *
		 * s := the scalar part of the quat.
		 *
		 * First consider even n (n = 0, 2, ..., 2 * N, ...):
		 *  theta = 2 * 2 * N * PI
		 *  s = cos(theta/2)
		 *    = cos(2 * N * PI)
		 *    = 1
		 *
		 * Next consider odd n (n = 1, 3, ..., 2 * N + 1, ...):
		 *  theta = 2 * (2 * N + 1) * PI
		 *  s = cos(theta/2)
		 *    = cos((2 * N + 1) * PI)
		 *    = cos(2 * N * PI + PI)
		 *    = cos(2 * N * PI) * cos(PI) -
		 *       sin(2 * N * PI) * sin(PI)
		 *    = 1 * (-1) - 0 * 0
		 *    = -1
		 *
		 * Thus, (abs(s) = 1) implies an identity rotation.
		 */
		return (abs(q.scalar_part()) == 1.0);
	}


	/**
	 * Return whether these two unit quaternions @a q1 and @a q2 represent
	 * equivalent rotations.
	 */
	inline
	bool
	represent_equiv_rotations(
	 const UnitQuaternion3D &q1,
	 const UnitQuaternion3D &q2) {

		/*
		 * A rotation is defined by an axis of rotation and an angle
		 * through which points are rotated about this axis.  Define
		 * 'theta' to be the angle of rotation, and 'U' to be a unit
		 * vector pointing in the direction of the axis of rotation.
		 * Together, U and theta are used to define a unit quaternion
		 * which describes the rotation.  [Assume theta is contained
		 * in the half-open range (-PI, PI].]
		 *
		 * It may be observed that a rotation of theta about U is
		 * equivalent to a rotation of (2 * PI - theta) about (-U).
		 * Accordingly, the quaternion 'Q1' (defined by theta and U)
		 * describes a rotation equivalent to that described by the
		 * quaternion 'Q2' (defined by (2 * PI - theta) and (-U)).
		 *
		 * In fact, it may be shown that Q2 is equivalent to (-Q1).
		 */
		return (q1 == q2 || q1 == -q2);
	}


	/**
	 * Multiply the two quaternions @a q1 and @a q2.
	 *
	 * NOTE that quaternion multiplication is <em>NOT</em> commutative.
	 */
	const UnitQuaternion3D
	operator*(
	 const UnitQuaternion3D &q1,
	 const UnitQuaternion3D &q2);


	std::ostream &
	operator<<(
	 std::ostream &os,
	 const UnitQuaternion3D &u);

}

#endif  // GPLATES_MATHS_UNITQUATERNION3D_H
