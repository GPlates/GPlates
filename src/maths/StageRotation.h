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
 */

#ifndef GPLATES_MATHS_STAGEROTATION_H
#define GPLATES_MATHS_STAGEROTATION_H

#include "UnitQuaternion3D.h"
#include "FiniteRotation.h"
#include "types.h"  /* real_t */


namespace GPlatesMaths
{
	/** 
	 * Represents a so-called "stage rotation" of plate tectonics.
	 *
	 * If a "finite rotation" represents the particular rotation used
	 * to transform a point on the sphere from the present back to a
	 * particular point in time, a stage rotation can be considered
	 * the delta between a pair of finite rotations.  It represents
	 * the change in rotation over the change in time.  [Note that
	 * the stage rotation itself is not a time-derivative, but it
	 * could very easily be used to calculate a time-derivative.]
	 *
	 * Alternately, if a finite rotation is considered as a point in a
	 * 4-dimensional rotation-space, a stage rotation is a displacement
	 * between two points.
	 */
	class StageRotation {

		public:

			/**
			 * Create a stage rotation consisting of the given
			 * unit quaternion and the given change in time.
			 * The change in time is in Millions of years.
			 */
			StageRotation(
			 const UnitQuaternion3D &uq,
			 const real_t &time_delta) :
			 m_unit_quat(uq),
			 m_time_delta(time_delta) {  }

			const UnitQuaternion3D &
			unit_quat() const {

				return m_unit_quat;
			}

			const real_t &
			timeDelta() const {

				return m_time_delta;
			}

			/**
			 * Apply this stage rotation to a finite rotation.
			 *
			 * Note that this function is a member function for
			 * a reason: to enforce the concept that the operation
			 * of a stage rotation is APPLIED TO a finite rotation
			 * -- it is very much a PREmultiplication, in the style
			 * of traditional matrix operations.
			 *
			 * This operation is not supposed to be symmetrical.
			 */
			const FiniteRotation
			operator*(
			 const FiniteRotation &r) const {

				UnitQuaternion3D res_uq =
				 unit_quat() * r.unit_quat();
				real_t res_time = r.time() + timeDelta();

				return FiniteRotation::create(res_uq, res_time);
			}

		private:

			UnitQuaternion3D m_unit_quat;
			real_t           m_time_delta;  // Millions of years
	};


	/**
	 * Returns the difference between two finite rotations as a
	 * stage rotation.
	 *
	 * If 'r1' describes the rotation of a moving plate 'M1' with respect
	 * to a fixed plate 'F1', and 'r2' describes the rotation of a moving
	 * plate 'M2' with respect to a fixed plate 'F2', then:
	 *
	 *  + F1 should equal F2  ("should equal" instead of "must equal",
	 *     since this function cannot enforce this equality).
	 *
	 *  + M1 should equal M2.
	 *
	 *  + if the result of this operation is called 'sr', then sr will
	 *     describe the motion of the moving plate.
	 *
	 * Note that, in contrast to most of the stage-rotation / finite-
	 * rotation / point-on-sphere operations, this operation is NOT
	 * read right-to-left (ie. starting with the right-most object,
	 * then moving left as successive operations are applied) in the
	 * style of premultiplication.  Rather, r1 is "taken", and then r2
	 * is "subtracted" from it (or rather, the inverse of r2 is applied
	 * to it).  In mathematical symbols, if:
	 *  C := A - B == A + (-B)
	 *             == B.inverse() * A  [where '+ X' is intended in the
	 * style of position vectors and displacements, and may be read as
	 * "then apply X"; while 'X *' is intended in the style of matrices,
	 * and may be read as "premultiply X, in order to apply it"], then:
	 *  A == B + C
	 *    == C * B.
	 * Due to this irregularity in the order-of-evaluation of arguments,
	 * this operation will be left as a named function (rather than being
	 * provided as an overloaded arithmetic operator), with the aim of
	 * minimising the confusion experienced when trying to read the code
	 * which invokes these operations (and trying to work out whether one
	 * is currently supposed to be reading the symbols from left-to-right
	 * or right-to-left).
	 */
	inline
	const StageRotation
	subtractFiniteRots(
	 const FiniteRotation &r1,
	 const FiniteRotation &r2) {

		UnitQuaternion3D res_uq =
		 r2.unit_quat().get_inverse() * r1.unit_quat();
		real_t time_delta = r1.time() - r2.time();

		return StageRotation(res_uq, time_delta);
	}


	/**
	 * Scale the given stage rotation such that its time delta is equal
	 * to the specified new time delta, and return the result.
	 *
	 * Note that the quaternion of the stage rotation argument MAY NOT
	 * represent an identity rotation, or else the rotation axis would be
	 * indeterminate.
	 *
	 * Note also that the time delta of the stage rotation argument MAY NOT
	 * be zero:  Aside from the physical impossibility of such a rotation
	 * (a rotation which occurs in zero time implies an infinite angular
	 * speed), a zero time delta obviously renders any attempt to scale
	 * the rotation *by its time delta* impossible.
	 *
	 * @throws IndeterminateResultException if the quaternion associated
	 *   with @a sr represents the indentity rotation.  @code
	 *   sr.quat().isIdentity() @endcode
	 * @throws IndeterminateResultException if the time delta of @a sr
	 *   is zero.  @code sr.timeDelta() == 0 @endcode
	 */
	const StageRotation
	scaleToNewTimeDelta(
	 const StageRotation &sr,
	 const real_t &new_time_delta);


#if 0  // Replaced by "FiniteRotation.{h,cc}:interpolate".
	/**
	 * Calculate and return the finite rotation which is the interpolation
	 * of the two finite rotations @a more_recent and @a more_distant to
	 * the time @a t.
	 *
	 * To be precise, this is really only an "interpolation" if @a t is
	 * between @a more_recent and @a more_distant, otherwise it is an
	 * "extrapolation".  Originally, this function was only designed to
	 * perform interpolations between finite rotations;  it is now also
	 * used to extrapolate into the future.
	 *
	 * Note to CG programmers: this is a "slerp".
	 */
	const FiniteRotation
	interpolate(
	 const FiniteRotation &more_recent,
	 const FiniteRotation &more_distant,
	 const real_t &t);
#endif

}

#endif  // GPLATES_MATHS_STAGEROTATION_H
