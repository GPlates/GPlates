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

#ifndef _GPLATES_MATHS_ROTATION_H_
#define _GPLATES_MATHS_ROTATION_H_

#include "UnitVector3D.h"
#include "UnitQuaternion3D.h"
#include "PointOnSphere.h"
#include "types.h"  /* real_t */


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


	/** 
	 * Represents a rotation by a particular angle about a particular axis.
	 *
	 * NOTE: This class should NOT be used for plate tectonic rotations
	 * -- for those rotations, you should use the 'FiniteRotation' and
	 * 'StageRotation' classes.  The rotations effected by this class are
	 * independent of time.
	 *
	 * Rotation angles are specified in radians, with the usual sense of
	 * rotation:  a positive angle represents an anti-clockwise rotation
	 * around the rotation vector;  a negative angle corresponds to a
	 * clockwise rotation.
	 */
	class Rotation
	{
			/*
			 * Declare this as a friend to enable it to use
			 * the protected creation function.
			 */
			friend Rotation operator*(const Rotation &r1,
			                          const Rotation &r2);

		public:
			/**
			 * Create a rotation with the given rotation axis
			 * and rotation angle.
			 *
			 * As always, the rotation angle is in radians.
			 */
			static Rotation
			Create(const UnitVector3D &rotation_axis,
			       const real_t &rotation_angle);


			/**
			 * Create a rotation which transforms @a initial
			 * to @a final.
			 */
			static Rotation
			Create(const UnitVector3D &initial,
			       const UnitVector3D &final);


			UnitVector3D
			axis() const { return _axis; }


			real_t
			angle() const { return _angle; }


			UnitQuaternion3D
			quat() const { return _quat; }


			/**
			 * Return the reverse of this rotation.
			 */
			Rotation
			reverse() const {

				UnitQuaternion3D rev_quat = quat().inverse();
				UnitVector3D rev_axis = -axis();

				return
				 Rotation::Create(rev_quat, rev_axis, angle());
			}


			/**
			 * Apply this rotation to a unit vector.
			 *
			 * Note that this function is a member function for
			 * two (2) reasons:
			 *
			 *  (i) to enable it to access the private member data
			 *       '_d' and '_e'.
			 *
			 *  (ii) to enforce the concept that the operation of
			 *        a finite rotation is APPLIED TO a vector --
			 *        it is very much a PREmultiplication, in the
			 *        style of traditional matrix operations.
			 *
			 * This operation is not supposed to be symmetrical.
			 */
			UnitVector3D
			operator*(const UnitVector3D &uv) const;

		protected:
			/**
			 * Create the rotation described by the supplied
			 * quaternion.  The supplied axis and angle
			 * must match those inside the quaternion.
			 * This requirement will not be checked!
			 *
			 * As always, the rotation angle is in radians.
			 */
			static Rotation
			Create(const UnitQuaternion3D &uq,
			       const UnitVector3D &rotation_axis,
			       const real_t &rotation_angle);


			Rotation(const UnitVector3D &ax,
			         const real_t &ang,
			         const UnitQuaternion3D &q,
			         const real_t &d,
			         const Vector3D &e)

			 : _axis(ax), _angle(ang), _quat(q), _d(d), _e(e) {  }

		private:
			UnitVector3D     _axis;
			real_t           _angle;  // in radians

			/**
			 * The unit quaternion which effects the rotation
			 * described by the rotation axis and angle.
			 */
			UnitQuaternion3D _quat;

			/*
			 * And now for the mysterious values of '_d' and '_e' !
			 *
			 * These are only used to rotate points on the sphere,
			 * and are calculated purely for optimisation purposes.
			 * I don't know whether they have any physical meaning.
			 * I suspect not.
			 */
			real_t   _d;
			Vector3D _e;
	};


	/**
	 * Compose two Rotations.
	 *
	 * Note: order of composition is important!
	 * Quaternion multiplication is not commutative!
	 * This operation is not commutative!
	 *
	 * This composition of rotations is very much in the style of matrix
	 * composition by premultiplication:  You take 'r2', then apply 'r1'
	 * to it (in front of it).
	 */
	Rotation
	operator*(const Rotation &r1, const Rotation &r2);


	/**
	 * Create a rotation which transforms the point @a initial
	 * to the point @a final.
	 */
	inline Rotation
	CreateRotation(const PointOnSphere &initial,
		const PointOnSphere &final) {

		return
		 Rotation::Create(initial.unitvector(), final.unitvector());
	}


	/**
	 * Apply the given rotation to the given point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	inline PointOnSphere
	operator*(const Rotation &r, const PointOnSphere &p) {

		UnitVector3D u = p.unitvector();
		return PointOnSphere(r * u);
	}
}

#endif  // _GPLATES_MATHS_ROTATION_H_
