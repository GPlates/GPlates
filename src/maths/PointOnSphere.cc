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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#include <sstream>
#include "PointOnSphere.h"

using namespace GPlatesMaths;

/**
 * Assert the class invariant.
 * Throw ViolatedCoordinateInvariantException if the invariant has been
 * violated.
 */
void
PointOnSphere::AssertInvariantHolds() const {

	if (90.0 < _lat || _lat < -90.0) {

		std::ostringstream oss("PointOnSphere latitude is ");
		oss << _lat;
		throw ViolatedCoordinateInvariantException(oss.str());

	} else if (180.0 < _long || _long < -180.0) {

		std::ostringstream oss("PointOnSphere longitude is ");
		oss << _long;
		throw ViolatedCoordinateInvariantException(oss.str());
	}
}


