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

#include <sstream>

#include "StageRotation.h"
#include "IndeterminateResultException.h"
#include "DirVector3D.h"
#include "Vector3D.h"


GPlatesMaths::StageRotation
GPlatesMaths::scaleToNewTimeDelta(StageRotation sr, real_t new_time_delta) {

	/*
	 * The basic algorithm used in this function is:
	 * 1. given a unit quaternion, reverse-engineer the rotation axis
	 *     and the rotation angle.
	 * 2. scale the rot-angle by the ratio (new time delta / time delta).
	 * 3. create a new stage rotation which represents a rotation around
	 *     the rot-axis, by the scaled rot-angle.
	 */

	/*
	 * Ensure that the quaternion of the stage rotation argument does not
	 * represent an identity rotation.
	 */
	if (sr.quat().isIdentity()) {

		throw IndeterminateResultException("Attempted to scale a "
		 "stage rotation whose quaternion represents the identity "
		 "rotation.");
	}

	UnitQuaternion3D::RotationParams params =
	 sr.quat().calcRotationParams();

	/*
	 * Ensure that the time delta of the stage rotation argument is not
	 * zero.
	 */
	if (sr.timeDelta() == 0) {

		throw IndeterminateResultException("Attempted to scale a "
		 "stage rotation whose time delta is zero.");
	}
	real_t time_delta_reciprocal = 1.0 / sr.timeDelta();

	/*
	 * Finally, create a unit quaternion which represents a rotation of
	 * ((new time delta / time delta) * params.angle) about 'params.axis'.
	 */
	UnitQuaternion3D new_uq =
	 UnitQuaternion3D::CreateEulerRotation(params.axis,
	  (new_time_delta * time_delta_reciprocal) * params.angle);

	return StageRotation(new_uq, new_time_delta);
}


GPlatesMaths::FiniteRotation
GPlatesMaths::interpolate(const FiniteRotation &more_recent, 
	const FiniteRotation &more_distant, const real_t &t) {

	/*
	 * Remember that 'more_distant' is a "larger" finite rotation than
	 * 'more_recent', so it should be the minuend of the subtraction
	 * [ http://mathworld.wolfram.com/Minuend.html ].
	 */
	StageRotation sr = subtractFiniteRots(more_distant, more_recent);
	if (sr.quat().isIdentity()) {

		// the quaternions of the rotations were equivalent
		return FiniteRotation::Create(more_recent.quat(), t);
	}

	real_t new_time_delta = t - more_recent.time();
	StageRotation scaled_sr = scaleToNewTimeDelta(sr, new_time_delta);
	FiniteRotation interp = scaled_sr * more_recent;

	return interp;
}
