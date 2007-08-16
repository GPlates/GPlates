/* $Id$ */

/**
 * @file
 * Contains the definitions of the functions of the namespace FloatingPointComparisons.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 James Boyden <jboy@jboy.id.au>
 *
 * This file is derived from the source file "FloatingPointComparisons.cc",
 * which is part of the ReconTreeViewer software:
 *  http://sourceforge.net/projects/recontreeviewer/
 *
 * ReconTreeViewer is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2, as published
 * by the Free Software Foundation.
 *
 * ReconTreeViewer is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <cmath>  // std::fabs
#include "FloatingPointComparisons.h"

// Allow an equality-comparison epsilon of about 1.0e-14.
#define EXPONENT 14
#define RE_EPS(y) (1.0##e##-##y)  // Yes, we *really do* need all those hashes.
#define REAL_EPSILON(y) RE_EPS(y)  // Yes, we *really do* need this extra macro.


bool
GPlatesUtil::FloatingPointComparisons::geo_times_are_approx_equal(
		const double &geo_time1,
		const double &geo_time2)
{
	// Allow a small epsilon of about 1.0e-11, in case of accumulated floating-point error.
	//
	// My justification for choosing 1.0e-11 is as follows:
	//  1. The largest value I have seen for a geo-time in a rotation file is 850.
	//  2. Let's call this "order of magnitude of 1000".
	//  3. Let's imagine that, one day in the future, someone might use this program to view a
	//     rotation file which contains a geo-time of the order of magnitude of 10000.
	//  4. For a double-precision IEEE754 floating-point number, the absolute value of a delta
	//     of 5 ULPs from 10000.0 is 0.000000000009094947.
	//  5. 1.0e-12 is less than this delta; 1.0e-11 is greater.
	//  6. Thus, an epsilon of 1.0e-11 allows for accumulated floating-point error of 5 ULPs at
	//     10000.0.
	//  7. An epsilon of 1.0e-11 is obviously smaller than the smallest expected precision of
	//     geological times, 3 decimal places.
	//
	// Read these articles for an introduction to ULPs:
	//  http://docs.sun.com/source/806-3568/ncg_math.html
	//  http://docs.sun.com/source/806-3568/ncg_goldberg.html
	static const double epsilon = 1.0e-11;

	return (std::fabs(geo_time1 - geo_time2) < epsilon);
}
