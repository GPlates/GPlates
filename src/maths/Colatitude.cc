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
#include "Colatitude.h"
#include "Latitude.h"
#include "Real.h"
#include "ViolatedClassInvariantException.h"


GPlatesMaths::Colatitude::Colatitude(const Latitude &lat) :
	_rval(GPlatesMaths::PI_2 - lat.rval()) {  }


void
GPlatesMaths::Colatitude::AssertInvariant() {

	/*
	 * First, perform "almost exact" comparisons for the invariant.
	 */
	if (_rval < 0 || _rval > GPlatesMaths::PI) {

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

	} else if (_rval.isPreciselyGreaterThan(GPlatesMaths::PI)) {

		// It was just slightly greater than pi.  Clamp it.
		_rval = GPlatesMaths::PI;
	}
}
