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
#include "Vector3D.h"


using namespace GPlatesMaths;


StageRotation
GPlatesMaths::scaleToNewTimeDelta(StageRotation sr, real_t new_time_delta) {

	/*
	 * The basic algorithm used in this function is:
	 * 1. given a unit quaternion, reverse-engineer the rotation axis
	 *     and the rotation angle, 'theta'.
	 * 2. scale theta by the ratio (new time delta / time delta).
	 * 3. create a new stage rotation which represents a rotation around
	 *     the rotation axis, by the scaled rotation angle.
	 */

	/*
	 * Ensure that the quaternion of the stage rotation argument does not
	 * represent an identity rotation.
	 *
	 * In an identity rotation, the angle of rotation is (2 * n * pi),
	 * for some integer 'n':  this would later result in an evaluation of
	 * the sine of some (n * pi), which is always zero.  This, in turn,
	 * would result in a division by zero when attempting to calculate
	 * the rotation axis, which is geometrically equivalent to the fact
	 * that, in an identity rotation, the axis is indeterminate.
	 */
	if (sr.quat().isIdentity()) {

		throw IndeterminateResultException("Attempted to scale a "
		 "stage rotation whose quaternion represents the identity "
		 "rotation.");
	}
	/*
	 * Thus, we can be sure that the angle of rotation ('theta') is not a
	 * multiple of two pi, and the axis of rotation is clearly determined.
	 */
	real_t theta_on_2 = acos(sr.quat().s());  // not a multiple of pi
	real_t sin_of_theta_on_2 = sin(theta_on_2);  // not zero

	Vector3D axis_v = (1 / sin_of_theta_on_2) * sr.quat().v();
	UnitVector3D axis_uv(axis_v.x(), axis_v.y(), axis_v.z());

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
	 * ((new time delta / time delta) * theta) about the axis specified by
	 * 'axis_uv'.
	 */
	UnitQuaternion3D new_uq =
	 UnitQuaternion3D::CreateEulerRotation(axis_uv,
	  (new_time_delta * time_delta_reciprocal) * (theta_on_2 * 2));

	return StageRotation(new_uq, new_time_delta);
}


FiniteRotation
GPlatesMaths::interpolate(const FiniteRotation &more_recent, 
	const FiniteRotation &more_distant, const real_t &t) {

	/*
	 * Remember that 'more_distant' is a "larger" finite rotation than
	 * 'more_recent', so it should be the minuend of the subtraction
	 * [ http://mathworld.wolfram.com/Minuend.html ].
	 */
	StageRotation sr = subtractFiniteRots(more_distant, more_recent);

	real_t new_time_delta = t - more_recent.time();
	StageRotation scaled_sr = scaleToNewTimeDelta(sr, new_time_delta);
	FiniteRotation interp = scaled_sr * more_recent;

	return interp;
}
