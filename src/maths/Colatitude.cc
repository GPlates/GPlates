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
#include "Colatitude.h"
#include "Latitude.h"
#include "ViolatedClassInvariantException.h"


/*
 * FIXME: These are often defined in the C Standard Library -- but are they
 * *always* defined *everywhere*, and to the *same precision*?
 */
#define MATHVALUE_PI      (3.14159265358979323846)
#define MATHVALUE_PI_ON_2 (1.57079632679489661923)


GPlatesMaths::Colatitude::Colatitude(const Latitude &lat) :
	_rval(MATHVALUE_PI_ON_2 - lat.rval()) {  }


void
GPlatesMaths::Colatitude::AssertInvariant() {

	/*
	 * First, perform "almost exact" comparisons for the invariant.
	 */
	if (_rval < 0 || _rval > MATHVALUE_PI) {

		/*
		 * Even allowing some flexibility of comparison,
		 * the Real value still falls outside the interval of valid
		 * latitudes.
		 */
		std::ostringstream oss;
		oss << "Attempted to create a Colatitude of "
		 << _rval << " radians.";
		throw ViolatedClassInvariantException(oss.str().c_str());
	}

	/*
	 * Now, clean up after any violations which would be caused by
	 * "almost valid" values.
	 */
	if (_rval.isPreciselyLessThan(0)) {

		// It was just slightly less than 0.  Clamp it.
		_rval = 0;

	} else if (_rval.isPreciselyGreaterThan(MATHVALUE_PI)) {

		// It was just slightly greater than pi.  Clamp it.
		_rval = MATHVALUE_PI;
	}
}
