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
#include "Latitude.h"
#include "Colatitude.h"
#include "Real.h"
#include "ViolatedClassInvariantException.h"


GPlatesMaths::Latitude::Latitude(const Colatitude &colat) :
	_rval(GPlatesMaths::PI_2 - colat.rval()) {  }


void
GPlatesMaths::Latitude::AssertInvariant() {

	/*
	 * First, perform "almost exact" comparisons for the invariant.
	 */
	if (_rval < -GPlatesMaths::PI_2 || _rval > GPlatesMaths::PI_2) {

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
	if (_rval.isPreciselyLessThan(-GPlatesMaths::PI_2)) {

		// It was just slightly less than -pi/2.  Clamp it.
		_rval = -GPlatesMaths::PI_2;

	} else if (_rval.isPreciselyGreaterThan(GPlatesMaths::PI_2)) {

		// It was just slightly greater than pi/2.  Clamp it.
		_rval = GPlatesMaths::PI_2;
	}
}
