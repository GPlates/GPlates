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


using namespace GPlatesMaths;


StageRotation
StageRotation::CreateStageRotation(const UnitQuaternion3D &uq,
	const real_t &time_delta) {

	real_t theta_on_2 = acos(uq.s());
	real_t sin_of_theta_on_2 = sin(theta_on_2);

	/*
	 * Ensure that the time delta is not zero.
	 */
	if (time_delta == 0) {

		throw IndeterminateResultException("Attempted to calculate a "
		 "stage rotation for a zero time delta.");
	}

	/*
	 * Ensure that the unit quaternion argument does not represent
	 * an identity rotation.
	 *
	 * In an identity rotation, the angle of rotation is (2 * n * pi),
	 * for some integer 'n':  this results in a sine of (n * pi), which
	 * is always zero.  This would result in a division by zero when
	 * attempting to calculate the euler pole, which is geometrically
	 * equivalent to the fact that, in an identity rotation, the euler
	 * pole is indeterminate.
	 */
	if (uq.isIdentity()) {

		throw IndeterminateResultException("Attempted to calculate a "
		 "stage rotation from a quaternion which represents the "
		 "identity rotation.");
	}
	// else, sin_of_theta_on_2 is nonzero
	Vector3D euler_pole = (1 / sin_of_theta_on_2) * uq.v();

	return StageRotation(uq, time_delta,
	 UnitVector3D(euler_pole.x(), euler_pole.y(), euler_pole.z()),
	  theta_on_2 * 2);
}


StageRotation
subtractFinite(const FiniteRotation &r1, const FiniteRotation &r2) {

	if (representEquivRotations(r1.quat(), r2.quat())) {

		throw IndeterminateResultException("Attempted to calculate a "
		 "stage rotation between two finite rotations defined by "
		 "equivalent quaternions.");
	}
	if (r1.time() == r2.time()) {

		throw IndeterminateResultException("Attempted to calculate a "
		 "stage rotation between two finite rotations defined for the "
		 "same point in time.");
	}

	UnitQuaternion3D res_uq = r2.quat().inverse() * r1.quat();
	real_t time_delta = r1.time() - r2.time();

	return StageRotation::CreateStageRotation(res_uq, time_delta);
}
