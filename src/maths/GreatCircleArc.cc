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

	/*
	 * Note that together, u1 and u2 defined a plane, but they were not
	 * necessarily perpendicular.  The rotation axis is defined to be
	 * normal to the plane, and thus must be perpendicular to u1.
	 *
	 * Together, u1 and the rotation axis also define a plane, whose
	 * normal we will denote y.  Since u1 and the rotation axis are
	 * perpendicular, u1, y and the rotation axis together form a
	 * right-handed orthonormal basis.
	 *
	 * The reason we choose to consider the normal y to the new plane
	 * as the _second_ component of the basis (in effect, considering
	 * it as the y-axis), is so that our rotation axis will be the
	 * z-axis, and thus all rotation will take place in the xy-plane,
	 * rotating from u1 (the x-axis) towards the normal y the y-axis).
	 *
	 * We shall calculate y using the cross-product, with the additional
	 * knowledge that we _know_ that u1 is perpendicular to the rotation
	 * axis, so the resulting vector will immediately have magnitude 1.
	 */
	real_t x_comp = rot_axis.y() * u1.z() - rot_axis.z() * u1.y();
	real_t y_comp = rot_axis.z() * u1.x() - rot_axis.x() * u1.z();
	real_t z_comp = rot_axis.x() * u1.y() - rot_axis.y() * u1.x();

	UnitVector3D normal_y(x_comp, y_comp, z_comp);

	return GreatCircleArc(u1, normal_y, rot_axis, rot_angle);
}
