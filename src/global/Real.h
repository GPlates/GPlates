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

#ifndef _GPLATES_GLOBAL_REAL_H_
#define _GPLATES_GLOBAL_REAL_H_

#include <cmath>


namespace GPlatesGlobal
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

			double _dval;

		public:
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

		return std::sin(r.dval());
	}


	inline Real
	cos(Real r) {

		return std::cos(r.dval());
	}


	inline Real
	tan(Real r) {

		return std::tan(r.dval());
	}


	inline std::ostream &
	operator<<(std::ostream &os, Real r) {

		os << r.dval();
		return os;
	}
}

#endif  // _GPLATES_GLOBAL_REAL_H_
