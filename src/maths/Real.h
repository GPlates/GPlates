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

#ifndef _GPLATES_MATHS_REAL_H_
#define _GPLATES_MATHS_REAL_H_

#include <iostream>
#include <cmath>


namespace GPlatesMaths
{
	/**
	 * An instance of this class is a floating-point approximation
	 * to an element of the field of real numbers.  The difference
	 * between instances of this class and instances of the standard
	 * floating-point types is the way in which arithmetic comparisons
	 * are handled:  this class attempts to avoid the problems associated
	 * with standard floating-point comparisons by providing
	 * "almost exact" comparisons instead of the "exact" comparisons
	 * provided by the standard floating-point types.
	 */
	class Real
	{
			static const double Epsilon;
			static const double NegativeEpsilon;

			/*
			 * these functions are friends,
			 * to be able to access the above two members.
			 */
			friend bool operator==(Real, Real);
			friend bool operator!=(Real, Real);
			friend bool operator<=(Real, Real);
			friend bool operator>=(Real, Real);
			friend bool operator<(Real, Real);
			friend bool operator>(Real, Real);
			
			friend std::istream&
			operator>>(std::istream&, Real&);

			double _dval;

		public:
			Real() : _dval(0.0) {  }

			Real(double d) : _dval(d) {  }

			double dval() const { return _dval; }

			Real &
			operator+=(Real other) {

				_dval += other._dval;
				return *this;
			}

			Real &
			operator-=(Real other) {

				_dval -= other._dval;
				return *this;
			}

			Real &
			operator*=(Real other) {

				_dval *= other._dval;
				return *this;
			}

			Real &
			operator/=(Real other) {

				_dval /= other._dval;
				return *this;
			}
	};


	inline bool
	operator==(Real r1, Real r2) {

		/*
		 * Allow difference between r1 and r2 to fall into a range
		 * instead of insisting upon an exact value.
		 * That range will be [-e, e].
		 */
		double d = r1.dval() - r2.dval();
		return (Real::NegativeEpsilon <= d && d <= Real::Epsilon);
	}


	/**
	 * On a Pentium IV processor, this should cost about
	 * (5 [FSUB] + (2 + 1) [2*FCOM] + 1 [OR]) = 9 clock cycles.
	 */
	inline bool
	operator!=(Real r1, Real r2) {

		/*
		 * Difference between r1 and r2 must lie *outside* range.
		 * This is necessary to maintain the logical invariant
		 * (a == b) iff not (a != b)
		 */
		double d = r1.dval() - r2.dval();
		return (Real::NegativeEpsilon > d || d > Real::Epsilon);
	}


	/**
	 * On a Pentium IV processor, this should cost about
	 * (5 [FSUB] + 2 [FCOM]) = 7 clock cycles.
	 */
	inline bool
	operator<=(Real r1, Real r2) {

		/*
		 * According to the logical invariant
		 * (a == b) implies (a <= b), the set of pairs (r1, r2) which
		 * cause this boolean comparison to evaluate to true must be
		 * a superset of the set of pairs which cause the equality
		 * comparison to return true.
		 */
		double d = r1.dval() - r2.dval();
		return (d <= Real::Epsilon);
	}


	inline bool
	operator>=(Real r1, Real r2) {

		/*
		 * According to the logical invariant
		 * (a == b) implies (a >= b), the set of pairs (r1, r2) which
		 * cause this boolean comparison to evaluate to true must be
		 * a superset of the set of pairs which cause the equality
		 * comparison to return true.
		 */
		double d = r1.dval() - r2.dval();
		return (Real::NegativeEpsilon <= d);
	}


	inline bool
	operator>(Real r1, Real r2) {

		/*
		 * (a > b) must be the logical inverse of (a <= b).
		 */
		double d = r1.dval() - r2.dval();
		return (d > Real::Epsilon);
	}


	inline bool
	operator<(Real r1, Real r2) {

		/*
		 * (a < b) must be the logical inverse of (a >= b).
		 */
		double d = r1.dval() - r2.dval();
		return (Real::NegativeEpsilon > d);
	}


	/**
	 * Using the exact value of the Real, return whether it is positive
	 * (ie. greater than exact zero) or not.
	 */
	inline bool
	isPositive(Real r) {

		return (r.dval() > 0.0);
	}


	/**
	 * Using the exact value of the Real, return whether it is negative
	 * (ie. less than exact zero) or not.
	 */
	inline bool
	isNegative(Real r) {

		return (r.dval() < 0.0);
	}


	/**
	 * Using the exact value of the Real, return whether it is greater
	 * than exact one or not.
	 */
	inline bool
	isGreaterThanOne(Real r) {

		return (r.dval() > 1.0);
	}


	/**
	 * Using the exact value of the Real, return whether it is greater
	 * than exact minus-one or not.
	 */
	inline bool
	isLessThanMinusOne(Real r) {

		return (r.dval() < -1.0);
	}


	inline Real
	operator+(Real r1, Real r2) {

		return Real(r1.dval() + r2.dval());
	}


	inline Real
	operator-(Real r1, Real r2) {

		return Real(r1.dval() - r2.dval());
	}


	inline Real
	operator*(Real r1, Real r2) {

		return Real(r1.dval() * r2.dval());
	}


	inline Real
	operator/(Real r1, Real r2) {

		return Real(r1.dval() / r2.dval());
	}


	inline Real
	operator-(Real r) {

		return Real(-r.dval());
	}


	inline Real
	sin(Real r) {

		return Real(std::sin(r.dval()));
	}


	inline Real
	cos(Real r) {

		return Real(std::cos(r.dval()));
	}


	inline Real
	tan(Real r) {

		return Real(std::tan(r.dval()));
	}


	/*
	 * FIXME: This is often defined in the C Standard Library -- but is it
	 * *always* defined *everywhere*, and to the *same precision*?
	 */
	const double MATHVALUE_PI = 3.14159265358979323846;


	inline Real
	degreesToRadians(Real rdeg) {

		return Real((MATHVALUE_PI / 180.0) * rdeg);
	}


	inline std::ostream &
	operator<<(std::ostream &os, Real r) {

		os << r.dval();
		return os;
	}


	inline std::istream &
	operator>>(std::istream &is, Real& r) {

		is >> r._dval;
		return is;
	}


	/**
	 * If the argument to this function is greater-than or equal-to 0.0
	 * (and thus, no exceptions are thrown), on a Pentium IV processor,
	 * this should cost about (5 [FSUB] + 2 [FCOM] + 2 [FCOM]) = 9 clock
	 * cycles plus the cost of the invocation of the 'std::sqrt' function
	 * (presumably at least 38 cycles for the P-IV FSQRT insn + the cost
	 * of a function call).
	 *
	 * Thus, the execution of the body of this function should cost at
	 * least 47 clock cycles + the cost of a function call.
	 */
	Real sqrt(Real r);

	Real asin(Real r);
	Real acos(Real r);
}

#endif  // _GPLATES_MATHS_REAL_H_
