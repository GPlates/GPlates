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
#include "Latitude.h"
#include "Colatitude.h"
#include "ViolatedClassInvariantException.h"


/*
 * FIXME: This is often defined in the C Standard Library -- but is it
 * *always* defined *everywhere*, and to the *same precision*?
 */
#define MATHVALUE_PI_ON_2 (1.57079632679489661923)


GPlatesMaths::Latitude::Latitude(const Colatitude &colat) :
	_rval(MATHVALUE_PI_ON_2 - colat.rval()) {  }


void
GPlatesMaths::Latitude::AssertInvariant() {

	/*
	 * First, perform "almost exact" comparisons for the invariant.
	 */
	if (_rval < -MATHVALUE_PI_ON_2 || _rval > MATHVALUE_PI_ON_2) {

		/*
		 * Even allowing some flexibility of comparison,
		 * the Real value still falls outside the interval of valid
		 * latitudes.
		 */
		std::ostringstream oss;
		oss << "Attempted to create a Latitude of "
		 << _rval << " radians.";
		throw ViolatedClassInvariantException(oss.str().c_str());
	}

	/*
	 * Now, clean up after any violations which would be caused by
	 * "almost valid" values.
	 */
	if (_rval.isPreciselyLessThan(-MATHVALUE_PI_ON_2)) {

		// It was just slightly less than -pi/2.  Clamp it.
		_rval = -MATHVALUE_PI_ON_2;

	} else if (_rval.isPreciselyGreaterThan(MATHVALUE_PI_ON_2)) {

		// It was just slightly greater than pi/2.  Clamp it.
		_rval = MATHVALUE_PI_ON_2;
	}
}
