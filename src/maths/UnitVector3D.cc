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
#include "UnitVector3D.h"
#include "ViolatedUnitVectorInvariantException.h"


using namespace GPlatesMaths;

void UnitVector3D::AssertInvariantHolds () const
{
	/*
	 * Calculate magnitude of vector to ensure that it actually _is_ 1.
	 * For efficiency, don't bother sqrting yet.
	 *
	 * On a Pentium IV processor, the calculation of 'mag_sqrd' should
	 * cost about (7 + 2 * 2) + (5 + 1) = 17 clock cycles, while the
	 * FP comparison for unity should cost about 9 clock cycles, making
	 * a total cost of about 26 clock cycles.
	 *
	 * Obviously, if the comparison returns false, performance will go
	 * right out the window.
	 */
	real_t mag_sqrd = (_x * _x) + (_y * _y) + (_z * _z);
	if (mag_sqrd != 1.0) {
		// invariant has been violated
		std::ostringstream oss ("UnitVector3D has magnitude ");
		oss << sqrt (mag_sqrd);
		throw ViolatedUnitVectorInvariantException(oss.str().c_str());
	}
}
