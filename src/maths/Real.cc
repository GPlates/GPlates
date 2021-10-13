/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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
#include <iostream>

#include "Real.h"

#include "FunctionDomainException.h"
#include "HighPrecision.h"


const GPlatesMaths::Real
GPlatesMaths::sqrt(
		const Real &r)
{
	if (is_strictly_negative(r)) {
		// The value of 'r' is not strictly valid as the argument to 'sqrt'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r < 0.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is
			// negative, which falls outside the domain of 'sqrt'.

			std::ostringstream oss("function 'sqrt' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// zero.  We'll return the sqrt of zero, which is zero.
			// FIXME:  We should log that we "corrected" this here.
			return Real(0.0);
		}
	}

	// Else, the value of 'r' is valid as the argument to 'sqrt'.
	return Real(std::sqrt(r.dval()));
}


const GPlatesMaths::Real
GPlatesMaths::asin(
		const Real &r)
{
	if (is_strictly_less_than_minus_one(r)) {
		// The value of 'r' is not strictly valid as the argument to 'asin'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r < -1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is less
			// than minus one, which falls outside the domain of 'asin'.

			std::ostringstream oss("function 'asin' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// minus one.  We'll return the asin of minus one, which is minus pi on
			// two.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected asin(" << HighPrecision<Real>(r) << ") to asin(-1)." << std::endl;
			return Real(-GPlatesMaths::HALF_PI);
		}
	}

	if (is_strictly_greater_than_one(r)) {
		// The value of 'r' is not strictly valid as the argument to 'asin'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r > 1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is
			// greater than one, which falls outside the domain of 'asin'.

			std::ostringstream oss("function 'asin' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// one.  We'll return the asin of one, which is pi on two.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected asin(" << HighPrecision<Real>(r) << ") to asin(1)." << std::endl;
			return Real(GPlatesMaths::HALF_PI);
		}
	}

	// Else, the value of 'r' is valid as the argument to 'asin'.
	return Real(std::asin(r.dval()));
}


const GPlatesMaths::Real
GPlatesMaths::acos(
		const Real &r)
{
	if (is_strictly_less_than_minus_one(r)) {
		// The value of 'r' is not strictly valid as the argument to 'acos'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r < -1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is less
			// than minus one, which falls outside the domain of 'acos'.

			std::ostringstream oss("function 'acos' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// minus one.  We'll return the acos of minus one, which is pi.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected acos(" << HighPrecision<Real>(r) << ") to acos(-1)." << std::endl;
			return Real(GPlatesMaths::PI);
		}
	}

	if (is_strictly_greater_than_one(r)) {
		// The value of 'r' is not strictly valid as the argument to 'acos'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r > 1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is
			// greater than one, which falls outside the domain of 'acos'.

			std::ostringstream oss("function 'acos' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// one.  We'll return the acos of one, which is zero.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected acos(" << HighPrecision<Real>(r) << ") to acos(1)." << std::endl;
			return Real(0.0);
		}
	}

	// Else, the value of 'r' is valid as the argument to 'acos'.
	return Real(std::acos(r.dval()));
}


const GPlatesMaths::Real
GPlatesMaths::atan2(
		const Real &y,
		const Real &x)
{
	// 0.0 is the only real floating-point value for which exact equality comparison is valid.
	if ((y == 0.0) && (x == 0.0)) {
		// Recall that we've defined atan2(0, 0) to be equal to zero.
		return Real(0.0);
	}
	return Real(std::atan2(y.dval(), x.dval()));
}


bool
GPlatesMaths::Real::is_nan() const
{
	return GPlatesMaths::is_nan(_dval);
}


bool
GPlatesMaths::Real::is_infinity() const
{
	return GPlatesMaths::is_infinity(_dval);
}


bool
GPlatesMaths::Real::is_positive_infinity() const
{
	return GPlatesMaths::is_positive_infinity(_dval);
}


bool
GPlatesMaths::Real::is_negative_infinity() const
{
	return GPlatesMaths::is_negative_infinity(_dval);
}


bool
GPlatesMaths::Real::is_finite() const
{
	return GPlatesMaths::is_finite(_dval);
}


GPlatesMaths::Real
GPlatesMaths::Real::quiet_nan()
{
	return GPlatesMaths::quiet_nan<double>();
}


GPlatesMaths::Real
GPlatesMaths::Real::positive_infinity()
{
	return std::numeric_limits<double>::infinity();
}


GPlatesMaths::Real
GPlatesMaths::Real::negative_infinity()
{
	return -std::numeric_limits<double>::infinity();
}


std::ostream &
GPlatesMaths::operator<<(std::ostream &os, const Real &r)
{
	os << r._dval;
	return os;
}


std::istream &
GPlatesMaths::operator>>(std::istream &is, Real &r)
{
	is >> r._dval;
	return is;
}

