/* $Id$ */

/**
 * \file
 * A collection of functions to aid in the formatting of strings.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include <sstream>
#include <iomanip>

#include "StringFormattingUtils.h"
#include "global/GPlatesAssert.h"


namespace
{
	/*
	 * Remove any unnecessary zero digits after the decimal place.
	 *
	 * For example:
	 * 123.000 -> 123.0
	 * 123.00 -> 123.0
	 * 123.0 -> 123.0
	 * 123.400 -> 123.4
	 * 123.004 -> 123.004
	 * 123.4 -> 123.4
	 */
	const std::string
	remove_trailing_zeroes(
			const std::string &s)
	{
		std::string::size_type index_of_period = s.find('.');
		if (index_of_period == std::string::npos) {
			// No period found.  Something strange is happening here.  Let's just
			// abort.
			return s;
		}

		// Loop until there are no more unnecessary zero digits.
		std::string dup = s;
		for (;;) {
			std::string::size_type index_of_last_zero = dup.rfind('0');
			if (index_of_last_zero == std::string::npos) {
				// No zeroes found, so nothing to do.
				break;
			}
			if (index_of_last_zero < dup.size() - 1) {
				// There is a non-zero character after this zero.
				break;
			}
			if (index_of_last_zero == index_of_period + 1) {
				// This zero follows directly after the period, hence is not
				// unneccessary.
				break;
			}
			dup.erase(index_of_last_zero);
		}
		return dup;
	}
}


const std::string
GPlatesUtils::formatted_double_to_string(
		const double &val, 
		unsigned width,
		int prec,
		bool elide_trailing_zeroes)
{
	GPlatesGlobal::Assert(width > 0,
		InvalidFormattingParametersException(GPLATES_EXCEPTION_SOURCE,
			"Attempt to format a real number using a negative width."));

	if (prec != IGNORE_PRECISION)
	{
		GPlatesGlobal::Assert(prec > 0,
			InvalidFormattingParametersException(GPLATES_EXCEPTION_SOURCE,
			"Attempt to format a real number using a negative precision."));

		// The number 3 below is the number of characters required to
		// represent (1) the decimal point, (2) the minus sign, and (3)
		// at least one digit to the left of the decimal point.
		GPlatesGlobal::Assert(width >= (static_cast<unsigned>(prec) + 3), 
			InvalidFormattingParametersException(GPLATES_EXCEPTION_SOURCE,
			"Attempted to format a real number with parameters that don't "\
			"leave enough space for the decimal point, sign, and integral part."));
	}

	std::ostringstream oss;
	oss << std::setw(width)			// Use 'width' chars of space
		<< std::right				// Right-justify
		<< std::setfill(' ')		// Fill in with spaces
		<< std::fixed 				// Always use decimal notation
		<< std::showpoint;			// Always show the decimal point

	if (prec != IGNORE_PRECISION)
	{
		oss << std::setprecision(prec);	// Show prec digits after the decimal point
	}
	
	oss	<< val;				// Print the actual value

	if (elide_trailing_zeroes) {
		return remove_trailing_zeroes(oss.str());
	} else {
		return oss.str();
	}
}


const std::string
GPlatesUtils::formatted_int_to_string(
		int val,
		unsigned width,
		char fill_char)
{
	GPlatesGlobal::Assert(width > 0,
		InvalidFormattingParametersException(GPLATES_EXCEPTION_SOURCE,
			"Attempt to format an integer using a negative width."));

	std::ostringstream oss;
	oss << std::setw(width) 
		<< std::right 
		<< std::setfill(fill_char) 
		<< val;
	return oss.str();
}

