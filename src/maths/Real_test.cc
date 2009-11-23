/* $Id$ */

/**
 * \file 
 * Tests for the Real class.
 *
 * Compile using:
 * g++ -o real_test maths/Real_test.cc maths/Real.cc global/GPlatesException.cc utils/CallStackTracker.cc -I .
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <iostream>
#include "maths/Real.h"

using namespace GPlatesMaths;

void print(int id, bool passed)
{
	std::cout << id << ": " << (passed ? "True" : "False") << std::endl;
}

int main()
{
	double zero = 0.0;

	// positive infinity
	print(0, positive_infinity() == Real::positive_infinity().dval());
	print(1, positive_infinity() == 1.0 / zero);
	print(2, is_infinity(positive_infinity()));
	print(3, is_positive_infinity(positive_infinity()));
	print(4, !is_negative_infinity(positive_infinity()));
	print(5, !is_nan(positive_infinity()));
	print(5, !is_zero(positive_infinity()));

	// negative infinity
	print(100, negative_infinity() == Real::negative_infinity().dval());
	print(101, negative_infinity() == -1.0 / zero);
	print(102, is_infinity(negative_infinity()));
	print(103, !is_positive_infinity(negative_infinity()));
	print(104, is_negative_infinity(negative_infinity()));
	print(105, !is_nan(negative_infinity()));
	print(106, !is_zero(negative_infinity()));

	// nan
	print(200, (nan() == Real::nan().dval()) == false);
	print(201, (nan() == zero / zero) == false);
	print(202, !is_infinity(nan()));
	print(203, !is_positive_infinity(nan()));
	print(204, !is_negative_infinity(nan()));
	print(205, is_nan(nan()));
	print(206, !is_zero(nan()));

	// zero
	print(302, !is_infinity(0.0));
	print(303, !is_positive_infinity(0.0));
	print(304, !is_negative_infinity(0.0));
	print(305, !is_nan(0.0));
	print(306, is_zero(0.0));

	return 0;
}

