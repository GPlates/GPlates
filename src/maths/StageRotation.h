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

#ifndef _GPLATES_MATHS_STAGEROTATION_H_
#define _GPLATES_MATHS_STAGEROTATION_H_

#include "UnitQuaternion3D.h"
#include "UnitVector3D.h"
#include "FiniteRotation.h"
#include "types.h"  /* real_t */


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


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
	class StageRotation
	{
		public:
			/**
			 * Create a stage rotation consisting of the given
			 * unit quaternion and the given change in time.
			 * The change in time is in Millions of years.
			 *
			 * NOTE that the unit quaternion argument MAY NOT
			 * represent an identity rotation.
			 */
			static StageRotation
			CreateStageRotation(const UnitQuaternion3D &uq,
			                    const real_t &time_delta);


			UnitQuaternion3D
			quat() const { return _quat; }


			real_t
			timeDelta() const { return _time_delta; }


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
			FiniteRotation
			operator*(const FiniteRotation &r) const {

				UnitQuaternion3D res_uq = quat() * r.quat();
				real_t res_time = r.time() + timeDelta();

				return
				 FiniteRotation::CreateFiniteRotation(res_uq,
				  res_time);
			}

		protected:
			StageRotation(const UnitQuaternion3D &uq,
			              const real_t &time_delta,
			              const UnitVector3D &pole,
			              const real_t &rot_angle)

			 : _quat(uq), _time_delta(time_delta),
			   _euler_pole(pole), _rotation_angle(rot_angle) {  }

		private:
			UnitQuaternion3D _quat;
			real_t           _time_delta;  // Millions of years
			UnitVector3D     _euler_pole;
			real_t           _rotation_angle;  // radians
	};


	/**
	 * Returns the difference between two finite rotations as a
	 * stage rotation.
	 *
	 * Note that the quaternions of the two finite rotations may not be
	 * equivalent, or else the stage rotation would be indeterminate.
	 *
	 * Note also that the times of the two finite rotations may not be
	 * equal, or else the stage rotation would describe a rotation through
	 * a nonzero angle which occurs in a zero time delta.  In addition to
	 * being physically impossible (it would imply an infinite angular
	 * speed), a zero time delta would mean that scaling the stage rotation
	 * by time would be undefined.
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
	StageRotation
	subtractFinite(const FiniteRotation &r1, const FiniteRotation &r2);
}

#endif  // _GPLATES_MATHS_STAGEROTATION_H_
