/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include <sstream>

#include "StageRotation.h"
#include "IndeterminateResultException.h"
#include "Vector3D.h"


const GPlatesMaths::StageRotation
GPlatesMaths::scaleToNewTimeDelta(
 const StageRotation &sr,
 const real_t &new_time_delta) {

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
	if (represents_identity_rotation(sr.unit_quat())) {

		throw IndeterminateResultException("Attempted to scale a "
		 "stage rotation whose quaternion represents the identity "
		 "rotation.");
	}

	UnitQuaternion3D::RotationParams params =
	 sr.unit_quat().get_rotation_params();

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
	 UnitQuaternion3D::create_rotation(params.axis,
	  (new_time_delta * time_delta_reciprocal) * params.angle);

	return StageRotation(new_uq, new_time_delta);
}


#if 0  // Replaced by "FiniteRotation.{h,cc}:interpolate".
const GPlatesMaths::FiniteRotation
GPlatesMaths::interpolate(
 const FiniteRotation &more_recent, 
 const FiniteRotation &more_distant,
 const real_t &t) {

	/*
	 * Remember that 'more_distant' is a "larger" finite rotation than
	 * 'more_recent', so it should be the minuend of the subtraction
	 * [ http://mathworld.wolfram.com/Minuend.html ].
	 */
	StageRotation sr = subtractFiniteRots(more_distant, more_recent);
	if (represents_identity_rotation(sr.unit_quat())) {

		// the quaternions of the rotations were equivalent
		return FiniteRotation::create(more_recent.unit_quat(), t);
	}

	real_t new_time_delta = t - more_recent.time();
	StageRotation scaled_sr = scaleToNewTimeDelta(sr, new_time_delta);
	FiniteRotation interp = scaled_sr * more_recent;

	return interp;
}
#endif
