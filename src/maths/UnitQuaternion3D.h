/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_MATHS_UNITQUATERNION3D_H
#define GPLATES_MATHS_UNITQUATERNION3D_H

#include <iosfwd>
#include <boost/optional.hpp>

#include "types.h"  /* real_t */
#include "Vector3D.h"
#include "UnitVector3D.h"


namespace GPlatesMaths {

	/**
	 * A unit quaternion with three-dimensional operations.
	 *
	 * @section overview Overview
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
	 * @section details Details
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
	 * @section bibliography Bibliography
	 *
	 * The following references are either cited in the documentation or
	 * have played a significant role in the design and implementation of
	 * this class:
	 *  - Burger89:  Peter Burger and Duncan Gillies, <i>Interactive
	 *     Computer Graphics: Functional, Procedural and Device-Level
	 *     Methods</i>.  Addison-Wesley, 1989.
	 *  - Kuipers02:  Jack B. Kuipers, <i>Quaternions and Rotation
	 *     Sequences</i>, Princeton University Press, 2002.
	 *  - Weisstein05a:  Eric W. Weisstein, "Division Algebra".  
	 *     <i>MathWorld</i> -- A Wolfram Web Resource [online].
	 *     http://mathworld.wolfram.com/DivisionAlgebra.html  [Accessed 9
	 *     April 2005]
	 *  - Weisstein05b:  Eric W. Weisstein, "Quaternion".  <i>MathWorld</i>
	 *     -- A Wolfram Web Resource [online].
	 *     http://mathworld.wolfram.com/Quaternion.html  [Accessed 9 April
	 *     2005]
	 *  - Wikipedia05a:  Wikipedia, "Quaternion".  <i>Wikipedia</i>
	 *     [online].  http://en.wikipedia.org/wiki/Quaternion  [Accessed 9
	 *     April 2005]
	 *  - Wikipedia05b:  Wikipedia, "Quaternions and spatial rotation".
	 *     <i>Wikipedia</i> [online].
	 *     http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
	 *     [Accessed 9 April 2005]
	 *
	 * CVS informs us that this class was first committed in September
	 * 2003, by which time all the online references had already been
	 * consulted.  However, they were not properly cited until the 9th
	 * of April, 2005.
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
		 * Access the scalar part of the quaternion as a duple (scalar, vector).
		 */
		const real_t &
		scalar_part() const {
			
			return m_scalar_part;
		}


		/**
		 * Access the vector part of the quaternion as a duple (scalar, vector).
		 */
		const Vector3D &
		vector_part() const {
			
			return m_vector_part;
		}


		/**
		 * Access the w-component of the quaternion as a 4-tuple (w, x, y, z).
		 */
		const real_t &
		w() const {
			
			return m_scalar_part;
		}


		/**
		 * Access the x-component of the quaternion as a 4-tuple (w, x, y, z).
		 */
		const real_t &
		x() const {
			
			return m_vector_part.x();
		}


		/**
		 * Access the y-component of the quaternion as a 4-tuple (w, x, y, z).
		 */
		const real_t &
		y() const {
			
			return m_vector_part.y();
		}


		/**
		 * Access the z-component of the quaternion as a 4-tuple (w, x, y, z).
		 */
		const real_t &
		z() const {
			
			return m_vector_part.z();
		}


		/**
		 * Return the conjugate of this unit quaternion.
		 *
		 * This operation is used in the calculation of the multiplicative inverse.
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


		/**
		 * Calculate the square of the <em>actual</em> norm of this
		 * quaternion (rather than just assuming it is equal to 1).
		 */
		const real_t
		get_actual_norm_sqrd() const {

			/*
			 * This is equivalent to:
			 *  (scalar_part() * scalar_part()) +
			 *  dot(vector_part(), vector_part())
			 */
			return (w()*w() + x()*x() + y()*y() + z()*z());
		}


		/**
		 * Renormalise the quaternion if necessary.
		 *
		 * (What exactly is "necessary" is decided by this function.)
		 */
		void
		renormalise_if_necessary();


		/**
		 * This struct is used to contain the reverse-engineered
		 * rotation parameters of an arbitrary (ie, not necessarily
		 * user-specified; possibly machine-calculated by interpolation
		 * or other means) unit-quaternion.
		 *
		 * Not much happens with this struct once it's been created
		 * (its members are quickly accessed and the struct instance
		 * is discarded), but it was felt that it was slightly better
		 * design (more type-safe, more self-documenting, etc.) to
		 * provide an explicit type for the rotation parameters (rather
		 * than using, say, a std::pair<,>).
		 */
		struct RotationParams {

			RotationParams(
			 const UnitVector3D &rot_axis,
			 const real_t &rot_angle) :
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
		get_rotation_params(
				const boost::optional<UnitVector3D> &axis_hint) const;


		/**
		 * This struct is used to contain the short-lived
		 * in-general,-not-a-unit-quaternion object created during the
		 * spherical linear interpolation between two unit-quaternions.
		 * 
		 * Not much happens with this struct once it's been created 
		 * (its members are quickly accessed and the struct instance
		 * is discarded), but it was felt that it was slightly better 
		 * design (more type-safe, more self-documenting, etc.) to 
		 * provide an explicit type for the non-unit-quaternion (rather
		 * than using, say, a std::pair<,>).
		 */
		struct NonUnitQuaternion {

			NonUnitQuaternion(
			 const real_t &scalar_part,
			 const Vector3D &vector_part) :
			 d_scalar_part(scalar_part),
			 d_vector_part(vector_part) {  }

			real_t d_scalar_part;
			Vector3D d_vector_part;

		};


		/**
		 * Create a unit quaternion to represent the following rotation
		 * around the given unit vector @a axis, by the given rotation
		 * angle @a angle.
		 *
		 * As always, the rotation angle is in radians.
		 *
		 * FIXME:  Rename this to 'create_axis_angle_rotation'.
		 */
		static
		const UnitQuaternion3D
		create_rotation(
		 const UnitVector3D &axis, 
		 const real_t &angle);


		/**
		 * Create a unit quaternion to represent an identity rotation.
		 */
		static
		const UnitQuaternion3D
		create_identity_rotation();


		/**
		 * Attempt to create a unit quaternion from @a q.
		 * 
		 * This function will enforce the invariant.
		 */
		static
		const UnitQuaternion3D
		create(
		 const NonUnitQuaternion &q);

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
		 *
		 * Update, 2005-07-28:  OK, slight change of plans -- this
		 * function @em is going to check the invariant, but if it (the
		 * invariant) is not intact, it (the function) won't throw an
		 * exception; rather, it will @em renormalise the quaternion.
		 *
		 * FIXME: Do this invariant check and renormalisation properly.
		 */
		UnitQuaternion3D(
		 const real_t &s,
		 const Vector3D &v);


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

		// FIXME:  Should this become a dot-product, like the vectors?
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

		// FIXME:  Should this become a dot-product, like the vectors?
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
		 *    = cos(2 * N * PI) * cos(PI) - sin(2 * N * PI) * sin(PI)
		 *    = 1 * (-1) - 0 * 0
		 *    = -1
		 *
		 * Thus, (abs(s) = 1) implies an identity rotation.
		 *
		 * (Obviously, since this is a *unit* quaternion, if the scalar part is equal to
		 * one, then the vector part must be the zero vector.  Recall that the norm-sqrd of
		 * a quaternion is ((s * s) + dot(v, v)); the only way that dot(v, v) can be equal
		 * to zero is if v is the zero vector.)
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
	 * Take the (4D, hypersphere) dot-product of the non-unit-quaternions
	 * @a q1 and @a q2.
	 */
	inline
	const real_t
	dot(
	 const UnitQuaternion3D::NonUnitQuaternion &q1,
	 const UnitQuaternion3D::NonUnitQuaternion &q2) {

		const real_t   &s1 = q1.d_scalar_part, &s2 = q2.d_scalar_part;
		const Vector3D &v1 = q1.d_vector_part, &v2 = q2.d_vector_part;

		return ((s1 * s2) + dot(v1, v2));
	}


	/**
	 * Take the (4D, hypersphere) dot-product of the unit-quaternions @a q1
	 * and @a q2.
	 */
	inline
	const real_t
	dot(
	 const UnitQuaternion3D &q1,
	 const UnitQuaternion3D &q2) {

		/*
		 * This is equivalent to:
		 *  (q1.scalar_part() * q2.scalar_part()) +
		 *  dot(q1.vector_part(), q2.vector_part())
		 */
		return
		 (q1.w() * q2.w() +
		  q1.x() * q2.x() +
		  q1.y() * q2.y() +
		  q1.z() * q2.z());
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


	/**
	 * Multiply the scalar @a c by the unit-quaternion @a q, producing a
	 * <em>non</em>-unit-quaternion result.
	 *
	 * This operation is commutative (hence the symmetrical version below).
	 */
	inline
	const UnitQuaternion3D::NonUnitQuaternion
	operator*(
	 const real_t &c,
	 const UnitQuaternion3D &q) {

		real_t   s = c * q.scalar_part();
		Vector3D v = c * q.vector_part();

		return UnitQuaternion3D::NonUnitQuaternion(s, v);
	}


	/**
	 * Multiply the scalar @a c by the unit-quaternion @a q, producing a
	 * <em>non</em>-unit-quaternion result.
	 *
	 * This operation is commutative (hence the symmetrical version below).
	 */
	inline
	const UnitQuaternion3D::NonUnitQuaternion
	operator*(
	 const UnitQuaternion3D &q,
	 const real_t &c) {

		return (c * q);
	}


	/**
	 * Add the two non-unit-quaternions @a q1 and @a q2, producing a
	 * non-unit-quaternion result.
	 */
	inline
	const UnitQuaternion3D::NonUnitQuaternion
	operator+(
	 const UnitQuaternion3D::NonUnitQuaternion &q1,
	 const UnitQuaternion3D::NonUnitQuaternion &q2) {

		real_t   s = q1.d_scalar_part + q2.d_scalar_part;
		Vector3D v = q1.d_vector_part + q2.d_vector_part;

		return UnitQuaternion3D::NonUnitQuaternion(s, v);
	}


	std::ostream &
	operator<<(
	 std::ostream &os,
	 const UnitQuaternion3D &u);


	std::ostream &
	operator<<(
	 std::ostream &os,
	 const UnitQuaternion3D::NonUnitQuaternion &q);

}

#endif  // GPLATES_MATHS_UNITQUATERNION3D_H
