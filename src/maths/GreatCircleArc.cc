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
 *
 */

#include <sstream>
#include "GreatCircleArc.h"
#include "DirVector3D.h"
#include "Vector3D.h"
#include "IndeterminateResultException.h"
#include "InvalidOperationException.h"


using namespace GPlatesMaths;

GreatCircleArc
GreatCircleArc::CreateGreatCircleArc(PointOnSphere p1, PointOnSphere p2) {

	UnitVector3D u1 = p1.unitvector();
	UnitVector3D u2 = p2.unitvector();

	real_t dp = dot(u1, u2);

	/*
	 * First, we ensure that these two unit vectors do in fact define
	 * a single unique great-circle arc.
	 */
	if (dp >= 1.0) {

		// parallel => start-point same as end-point => no arc
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from duplicate endpoints ");
		oss << u1 << " and " << u2 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}
	if (dp <= -1.0) {

		// antiparallel => start-point and end-point antipodal =>
		// indeterminate arc
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
	Vector3D v = cross (u1, u2);

	/*
	 * Since u1 and u2 are unit vectors, the magnitude of v will be
	 * in the half-open range (0, 1], and will be equal to the sine
	 * of the smaller of the two angles between u1 and u2.
	 *
	 * Note that the magnitude of v cannot be _equal_ to zero,
	 * since the vectors are neither parallel nor antiparallel.
	 */
	UnitVector3D rot_axis = v.normalise();

	return GreatCircleArc(p1, p2, rot_axis);
}


GreatCircleArc
GreatCircleArc::CreateGreatCircleArc(PointOnSphere p1, PointOnSphere p2,
	UnitVector3D rot_axis) {

	UnitVector3D u1 = p1.unitvector();
	UnitVector3D u2 = p2.unitvector();

	real_t dp = dot(u1, u2);

	/*
	 * First, we ensure that these two unit vectors do in fact define
	 * a single unique great-circle arc.
	 */
	if (dp >= 1.0) {

		// start-point same as end-point => no arc
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from duplicate endpoints ");
		oss << u1 << " and " << u2 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}
	if (dp <= -1.0) {

		// start-point and end-point antipodal => indeterminate arc
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from antipodal endpoints ");
		oss << u1 << " and " << u2 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}

	/*
	 * Ensure that 'rot_axis' does, in fact, qualify as a rotation axis
	 * (ie. that it is collinear with (ie. parallel or antiparallel to)
	 * the cross product of 'u1' and 'u2').
	 *
	 * To do this, we calculate the cross product.
	 * (We use the Vector3D cross product, which is faster).
	 */
	Vector3D v = cross(Vector3D(u1), Vector3D(u2));
	if ( ! collinear(v, Vector3D(rot_axis))) {

		// 'rot_axis' is not the axis which rotates 'u1' into 'u2'
		std::ostringstream oss("Attempted to calculate a great-circle "
		 "arc from an invalid triple of vectors (");
		oss << u1 << " and " << u2 << " around " << rot_axis << ").";
		throw InvalidOperationException(oss.str().c_str());
	}

	return GreatCircleArc(p1, p2, rot_axis);
}
