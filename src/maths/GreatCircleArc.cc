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
#include "GreatCircleArc.h"
#include "DirVector3D.h"
#include "InvalidGreatCircleArcException.h"


using namespace GPlatesMaths;

GreatCircleArc
GreatCircleArc::CreateGreatCircleArc(UnitVector3D u1, UnitVector3D u2) {

	/*
	 * First, we ensure that these two unit vectors do in fact define
	 * a single unique great-circle arc.
	 */
	if (u1 == u2) {

		// start-point same as end-point => no arc
		std::ostringstream
		 oss("Arc has duplicate unit-vector endpoints ");
		oss << u1 << " and " << u2;
		throw InvalidGreatCircleArcException(oss.str().c_str());
	}
	if (antiparallel(u1, u2)) {

		// start-point and end-point antipodal => indeterminate arc
		std::ostringstream
		 oss("Arc has antipodal unit-vector endpoints ");
		oss << u1 << " and " << u2;
		throw InvalidGreatCircleArcException(oss.str().c_str());
	}

	/*
	 * Now we want to calculate:
	 *  - the angle of rotation
	 *  - the unit vector normal to the plane of rotation (or
	 *     the "rotation axis")
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
	real_t rot_angle = asin(v.magnitude());
	UnitVector3D rot_axis = v.normalise();

	return GreatCircleArc(u1, rot_axis, rot_angle);
}
