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
#include "Real.h"
#include "FunctionDomainException.h"

/*
 * FIXME: the value below was just a guess. Discover what this value should be.
 * 
 * According to:
 *  http://www.cs.berkeley.edu/~demmel/cs267/lecture21/lecture21.html
 * and
 *  http://www.ma.utexas.edu/documentation/lapack/node73.html
 * the machine epsilon for an IEEE 754-compliant machine is about 1.2e-16.
 *
 * According to these documents, the machine epsilon (aka "macheps") is
 * half the distance between 1 and the next largest fp value.
 *
 * Not only do I wish to allow for rounding errors due to the limits of
 * floating-point precision, I also wish to allow for a small accumulation
 * of such rounding errors.
 *
 * If macheps is 1.2e-16, then I -guessed- that it might be a good idea to
 * allow a flexibility of about two orders of magnitude, ie. 1.0e-14.
 *
 * The situations where such flexibility might be really important would
 * occur when deviations outside the epsilon could cause exceptions to be
 * thrown.  For example, say we are rotating a unitvector by multiplication
 * with a matrix.  That's 9 fpmuls and 6 fpadds to perform the rotation,
 * and then 3 fpmuls and 2 fpadds to check the magnitude of the resulting
 * unitvector.  That's a lot of error that can accumulate, and that's only
 * a _single_ rotation.  Of course, bear in mind that I have no background
 * in numerical analysis (yet...) so this is all just handwaving.  End rant.
 *
 * Update, 2004-02-05: well, it seems 1.0e-14 is too strict... so let's try
 * 1.0e-12, I guess...  I really need to do this stuff properly. --JB
 */
#define REAL_EPSILON (1.0e-12)

using namespace GPlatesMaths;

const double Real::Epsilon = REAL_EPSILON;
const double Real::NegativeEpsilon = -REAL_EPSILON;


Real
GPlatesMaths::sqrt(Real r) {

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


/**
 * Calculate the arc sine of the Real r, which must lie in the valid domain
 * of the arc sine function, the range [-1, 1].
 *
 * Don't forget: the arc sine will be returned in radians, not degrees!
 */
Real
GPlatesMaths::asin(Real r) {

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
		return Real(-GPlatesMaths::PI_2);
	}
	if (isGreaterThanOne(r)) {

		// it was just slightly greater than one -- return asin of one
		return Real(GPlatesMaths::PI_2);
	}
	return Real(std::asin(r.dval()));
}


/**
 * Calculate the arc cosine of the Real r, which must lie in the valid domain
 * of the arc cosine function, the range [-1, 1].
 *
 * Don't forget: the arc cosine will be returned in radians, not degrees!
 */
Real
GPlatesMaths::acos(Real r) {

	// First, perform "almost exact" comparisons for bounds of domain.
	if (r < -1.0 || r > 1.0) {

		/*
		 * Even allowing some flexibility of comparison, 
		 * the argument which falls outside the domain of asin.
		 */
		std::ostringstream 
		 oss("function 'acos' invoked with invalid argument ");
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
		return Real(-GPlatesMaths::PI_2);
	}
	if (isGreaterThanOne(r)) {

		// it was just slightly greater than one -- return asin of one
		return Real(GPlatesMaths::PI_2);
	}
	return Real(std::acos(r.dval()));
}
