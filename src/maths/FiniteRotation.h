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

#ifndef _GPLATES_MATHS_FINITEROTATION_H_
#define _GPLATES_MATHS_FINITEROTATION_H_

#include "UnitVector3D.h"
#include "UnitQuaternion3D.h"
#include "PointOnSphere.h"
#include "GreatCircleArc.h"
#include "types.h"  /* real_t */


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


	/** 
	 * Represents a so-called "finite rotation" of plate tectonics.
	 *
	 * Plate tectonics theory states that the motion of plates on the
	 * surface of the globe can be described by "Euler rotations".
	 * A finite rotation is the Euler rotation which transforms
	 * a point on the surface of the sphere from its present-day
	 * location to its calculated location at some point in the past.
	 * Thus, a finite rotation is associated with a particular point
	 * in time, as well as a particular transformation.
	 *
	 * An Euler rotation is a rotation about an "Euler pole" (a pole
	 * of rotation whose axis extends through the centre of the globe),
	 * by an angular distance.
	 *
	 * Euler poles are specified by a point on the surface of the globe.
	 * [The Euler pole is directed from the centre of the globe to this
	 * point on the surface, and thus may be considered an axial rotation
	 * vector.]
	 *
	 * Rotation angles are specified in radians, with the usual sense of
	 * rotation:  a positive angle represents an anti-clockwise rotation
	 * around the rotation vector;  a negative angle corresponds to a
	 * clockwise rotation.
	 */
	class FiniteRotation
	{
		public:
			/**
			 * Create a finite rotation with the given Euler pole
			 * and rotation angle, for the given point in time.
			 * As always, the rotation angle is in radians; the
			 * point in time is in Ma (Millions of years ago).
			 */
			static FiniteRotation
			CreateFiniteRotation(const PointOnSphere &euler_pole,
			                     const real_t &rotation_angle,
			                     const real_t &point_in_time);


			/**
			 * Create a finite rotation consisting of the given
			 * unit quaternion and the given point in time.
			 * The point in time is in Ma (Millions of years ago).
			 */
			static FiniteRotation
			CreateFiniteRotation(const UnitQuaternion3D &uq,
			                     const real_t &point_in_time);


			UnitQuaternion3D
			quat() const { return _quat; }


			real_t
			time() const { return _time; }


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
			 *
			 * On a Pentium IV processor, this should cost about
			 * 113 clock cycles + the cost of three function calls.
			 */
			UnitVector3D
			operator*(const UnitVector3D &uv) const;

		protected:
			FiniteRotation(const UnitQuaternion3D &q,
			               const real_t &t,
			               const real_t &d,
			               const Vector3D &e)

			 : _quat(q), _time(t), _d(d), _e(e) {  }

		private:
			UnitQuaternion3D _quat;
			real_t           _time;  // Millions of years ago

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
	 * Compose two FiniteRotations of the same point in time.
	 *
	 * Note: order of composition is important!
	 * Quaternion multiplication is not commutative!
	 * This operation is not commutative!
	 *
	 * This composition of rotations is very much in the style of matrix
	 * composition by premultiplication:  You take 'r2', then apply 'r1'
	 * to it (in front of it).
	 *
	 * If r1 describes the rotation of a moving plate 'M1' with respect
	 * to a fixed plate 'F1', and r2 describes the rotation of a moving
	 * plate 'M2' with respect to 'F2', then:
	 *
	 *  + M1 should equal F2  ("should equal" instead of "must equal",
	 *     since this function cannot enforce this equality).
	 *
	 *  + if the result of this operation is called 'rr', then rr will
	 *     describe the motion of the moving plate M2 with respect to
	 *     the fixed plate F1.  Thus, the unit vector which is rotated
	 *     by the resulting finite rotation will "sit" on M2.
	 *
	 * If these finite rotations are considered the branches of a tree-
	 * like hierarchy of plate-motion (with the stationary "globe" at
	 * the root of the tree, and the motion of any given plate specified
	 * relative to the plate root-ward of it), then the finite rotation
	 * r1 should be one branch root-ward of the finite rotation r2.
	 */
	FiniteRotation
	operator*(const FiniteRotation &r1, const FiniteRotation &r2);


	/**
	 * Apply the given rotation to the given point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	inline PointOnSphere
	operator*(const FiniteRotation &r, const PointOnSphere &p) {

		UnitVector3D rot_uv = r * p.unitvector();
		return PointOnSphere(rot_uv);
	}


	/**
	 * Apply the given rotation to the given great circle arc.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	inline GreatCircleArc
	operator*(const FiniteRotation &r, const GreatCircleArc &g) {

		UnitVector3D rot_start    = r * g.startPoint();
		UnitVector3D rot_end      = r * g.endPoint();
		UnitVector3D rot_rot_axis = r * g.rotationAxis();

		return GreatCircleArc::CreateGreatCircleArc(rot_start, rot_end,
		 rot_rot_axis);
	}
}

#endif  // _GPLATES_MATHS_FINITEROTATION_H_
