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
#include "Real.h"
#include "FunctionDomainException.h"

/*
 * FIXME: this value was just a guess. Discover what this value should be.
 */
#define REAL_EPSILON (1.0e-15)

using namespace GPlatesMaths;

const double Real::Epsilon = REAL_EPSILON;
const double Real::NegativeEpsilon = -REAL_EPSILON;


Real
sqrt(Real r) {

	// First, perform "almost exact" comparison.
	if (r < 0.0) {

		/*
		 * Even allowing some flexibility of comparison,
		 * the argument is negative, which falls outside
		 * the domain of sqrt.
		 */
		std::ostringstream
		 oss("function 'sqrt' invoked with invalid argument ");
		oss << r;
		throw FunctionDomainException(oss.str().c_str());
	}

	/*
	 * Now, clean up after any errors which are caused by "almost valid"
	 * arguments.
	 */
	if (isNegative(r)) {

		// it was just slightly less than zero -- return sqrt of zero
		return Real(0.0);

	}
	return Real(std::sqrt(r.dval()));
}


/*
 * FIXME: This is often defined in the C Standard Library -- but is it
 * *always* defined *everywhere*, and to the *same precision*?
 */
#define MATHVALUE_PI_ON_2 (1.57079632679489661923)


/*
 * Calculate the arc sine of the Real r, which must lie in the valid domain
 * of the arc sine function, the range [-1, 1].
 *
 * Don't forget: the arc sine will be returned in radians, not degrees!
 */
Real
asin(Real r) {

	// First, perform "almost exact" comparisons for bounds of domain.
	if (r < -1.0 || r > 1.0) {

		/*
		 * Even allowing some flexibility of comparison, 
		 * the argument which falls outside the domain of asin.
		 */
		std::ostringstream 
		 oss("function 'asin' invoked with invalid argument ");
		oss << r;
		throw FunctionDomainException(oss.str().c_str());
	}

	/*
	 * Now, clean up after any errors which are caused by "almost valid"
	 * arguments.
	 */
	if (isLessThanMinusOne(r)) {

		// it was just slightly less than minus one
		// -- return asin of minus one
		return Real(-MATHVALUE_PI_ON_2);
	}
	if (isGreaterThanOne(r)) {

		// it was just slightly greater than one -- return asin of one
		return Real(MATHVALUE_PI_ON_2);
	}
	return Real(std::asin(r.dval()));
}
