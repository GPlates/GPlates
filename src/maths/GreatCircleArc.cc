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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <sstream>
#include "GreatCircleArc.h"
#include "DirVector3D.h"
#include "Vector3D.h"
#include "IndeterminateResultException.h"
#include "InvalidOperationException.h"


using namespace GPlatesMaths;

GreatCircleArc
GreatCircleArc::CreateGreatCircleArc(UnitVector3D u1, UnitVector3D u2) {

	/*
	 * As a speed-up, since parallel(u1,u2) iff dot(u1,u2) >= 1.0,
	 * and antiparallel(u1,u2) iff dot(u1,u2) <= -1.0, we can factor
	 * out the dot product computation, which is what I do here.
	 *	- DAS 17/12/2003
	 */
	real_t dotP = dot(u1, u2);

	/*
	 * First, we ensure that these two unit vectors do in fact define
	 * a single unique great-circle arc.
	 */
	if (dotP >= 1.0) {

		// start-point same as end-point => no arc
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from duplicate endpoints ");
		oss << u1 << " and " << u2 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}
	if (dotP <= -1.0) {

		// start-point and end-point antipodal => indeterminate arc
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from antipodal endpoints ");
		oss << u1 << " and " << u2 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}

	/*
	 * Now we want to calculate the unit vector normal to the plane
	 * of rotation (this vector also known as the "rotation axis").
	 *
	 * To do this, we calculate the cross product.
	 */
	DirVector3D v = cross(u1, u2);

	/*
	 * Since u1 and u2 are unit vectors, the magnitude of v will be
	 * in the half-open range (0, 1], and will be equal to the sine
	 * of the smaller of the two angles between u1 and u2.
	 *
	 * Note that the magnitude of v cannot be _equal_ to zero,
	 * since the vectors are neither parallel nor antiparallel.
	 */
	UnitVector3D rot_axis = v.normalise();

	return GreatCircleArc(u1, u2, rot_axis);
}


GreatCircleArc
GreatCircleArc::CreateGreatCircleArc(UnitVector3D u1, UnitVector3D u2,
	UnitVector3D rot_axis) {

	/*
	 * (apply the same speedup as the other CreateGreatCircleArc, above)
	 *	- DAS 17/12/2003
	 */
	real_t dotP = dot(u1, u2);

	/*
	 * First, we ensure that these two unit vectors do in fact define
	 * a single unique great-circle arc.
	 */
	if (dotP >= 1.0) {

		// start-point same as end-point => no arc
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from duplicate endpoints ");
		oss << u1 << " and " << u2 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}
	if (dotP <= -1.0) {

		// start-point and end-point antipodal => indeterminate arc
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from antipodal endpoints ");
		oss << u1 << " and " << u2 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}

	/*
	 * Ensure that 'rot_axis' does, in fact, qualify as a rotation axis
	 * (ie. that it is parallel to the cross product of 'u1' and 'u2').
	 *
	 * To do this, we calculate the cross product.
	 * (We use the Vector3D cross product, which is faster).
	 */
	Vector3D v = cross(Vector3D(u1), Vector3D(u2));
	if ( ! parallel(v, Vector3D(rot_axis))) {

		// 'rot_axis' is not the axis which rotates 'u1' into 'u2'
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from an invalid triple of vectors (");
		oss << u1 << " and " << u2 << " around " << rot_axis << ").";
		throw InvalidOperationException(oss.str().c_str());
	}

	return GreatCircleArc(u1, u2, rot_axis);
}
