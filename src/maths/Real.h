/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_MATHS_REAL_H_
#define _GPLATES_MATHS_REAL_H_

#include <iostream>
#include <cmath>
#include <boost/python.hpp>

#include "global/types.h"


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	/// \f$ \pi \f$, the ratio of the circumference to the diameter of a circle
	static const double PI = 3.14159265358979323846264338;
	/// \f$ \frac{\pi}{2} \f$
	static const double PI_2 = 1.57079632679489661923;

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
	public:
		static const unsigned High_Precision;

	private:
		static const double Epsilon;
		static const double Negative_Epsilon;

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
		
		friend std::istream &operator>>(std::istream &, Real &);

		double _dval;

	public:
		Real()
			: _dval(0.0)
		{  }

		Real(double d)
			: _dval(d)
		{  }

		explicit
		Real(fpdata_t fpd)
			: _dval(fpd.dval())
		{  }

		double
		dval() const
		{
			return _dval;
		}


		Real &
		operator+=(Real other)
		{
			_dval += other._dval;
			return *this;
		}

		Real &
		operator-=(Real other)
		{
			_dval -= other._dval;
			return *this;
		}

		Real &
		operator*=(Real other)
		{
			_dval *= other._dval;
			return *this;
		}

		Real &
		operator/=(Real other)
		{
			_dval /= other._dval;
			return *this;
		}

		bool
		isPreciselyGreaterThan(double d) const
		{
			return _dval > d;
		}

		bool
		isPreciselyLessThan(double d) const
		{
			return _dval < d;
		}

		/**
		 * Convert a Real into a Python object
		 */
		static
		PyObject *
		convert(const Real &r)
		{
			return boost::python::incref(boost::python::object(r._dval).ptr());
		}
	};


	inline
	bool
	operator==(Real r1, Real r2)
	{
		/*
		 * Allow difference between r1 and r2 to fall into a range
		 * instead of insisting upon an exact value.
		 * That range will be [-e, e].
		 */
		double d = r1.dval() - r2.dval();
		return Real::Negative_Epsilon <= d && d <= Real::Epsilon;
	}


	/**
	 * Return whether the two supplied real numbers @a r1 and @a r2 are
	 * equal to within a slightly stricter tolerance than the standard
	 * equality tolerance, aka @a Real::Epsilon.
	 *
	 * This function is used by @a GPlatesMaths::FiniteRotation::operator*
	 * and will hopefully be the first in a new generation of comparison
	 * functions whose tolerances are tailored to the specific situation
	 * in which they will be used, thus providing more robust and more
	 * correct code.
	 *
	 * FIXME:  "Make it so."
	 */
	inline
	bool
	are_slightly_more_strictly_equal(
			const Real &r1,
			const Real &r2)
	{
		double d = r1.dval() - r2.dval();
		return -9.99e-13 <= d && d <= 9.99e-13;
	}


	/**
	 * On a Pentium IV processor, this should cost about
	 * (5 [FSUB] + (2 + 1) [2*FCOM] + 1 [OR]) = 9 clock cycles.
	 */
	inline
	bool
	operator!=(Real r1, Real r2)
	{
		/*
		 * Difference between r1 and r2 must lie *outside* range.
		 * This is necessary to maintain the logical invariant
		 * (a == b) iff not (a != b)
		 */
		double d = r1.dval() - r2.dval();
		return Real::Negative_Epsilon > d || d > Real::Epsilon;
	}


	/**
	 * On a Pentium IV processor, this should cost about
	 * (5 [FSUB] + 2 [FCOM]) = 7 clock cycles.
	 */
	inline
	bool
	operator<=(Real r1, Real r2)
	{
		/*
		 * According to the logical invariant
		 * (a == b) implies (a <= b), the set of pairs (r1, r2) which
		 * cause this boolean comparison to evaluate to true must be
		 * a superset of the set of pairs which cause the equality
		 * comparison to return true.
		 */
		double d = r1.dval() - r2.dval();
		return d <= Real::Epsilon;
	}


	inline
	bool
	operator>=(Real r1, Real r2)
	{
		/*
		 * According to the logical invariant
		 * (a == b) implies (a >= b), the set of pairs (r1, r2) which
		 * cause this boolean comparison to evaluate to true must be
		 * a superset of the set of pairs which cause the equality
		 * comparison to return true.
		 */
		double d = r1.dval() - r2.dval();
		return Real::Negative_Epsilon <= d;
	}


	inline
	bool
	operator>(Real r1, Real r2)
	{
		/*
		 * (a > b) must be the logical inverse of (a <= b).
		 */
		double d = r1.dval() - r2.dval();
		return d > Real::Epsilon;
	}


	inline
	bool
	operator<(Real r1, Real r2)
	{
		/*
		 * (a < b) must be the logical inverse of (a >= b).
		 */
		double d = r1.dval() - r2.dval();
		return Real::Negative_Epsilon > d;
	}

	inline
	Real
	abs(const Real &r1)
	{
		return Real (fabs (r1.dval ()));
	}


	/**
	 * Using the exact value of the Real, return whether it is positive
	 * (ie. greater than exact zero) or not.
	 */
	inline
	bool
	isPositive(Real r) {

		return r.dval() > 0.0;
	}


	/**
	 * Using the exact value of the Real, return whether it is negative
	 * (ie. less than exact zero) or not.
	 */
	inline
	bool
	isNegative(Real r)
	{
		return r.dval() < 0.0;
	}


	/**
	 * Using the exact value of the Real, return whether it is greater
	 * than exact one or not.
	 */
	inline
	bool
	isGreaterThanOne(Real r)
	{
		return r.dval() > 1.0;
	}


	/**
	 * Using the exact value of the Real, return whether it is greater
	 * than exact minus-one or not.
	 */
	inline
	bool
	isLessThanMinusOne(Real r)
	{
		return r.dval() < -1.0;
	}


	inline
	Real
	operator+(Real r1, Real r2)
	{
		return Real(r1.dval() + r2.dval());
	}


	inline
	Real
	operator-(Real r1, Real r2)
	{
		return Real(r1.dval() - r2.dval());
	}


	inline
	Real
	operator*(Real r1, Real r2)
	{
		return Real(r1.dval() * r2.dval());
	}


	inline
	Real
	operator/(Real r1, Real r2)
	{
		return Real(r1.dval() / r2.dval());
	}


	inline
	Real
	operator-(Real r)
	{
		return Real(-r.dval());
	}


	inline
	Real
	sin(Real r)
	{
		return Real(std::sin(r.dval()));
	}


	inline
	Real
	cos(Real r)
	{
		return Real(std::cos(r.dval()));
	}


	inline
	Real
	tan(Real r)
	{
		return Real(std::tan(r.dval()));
	}

	inline
	Real
	degreesToRadians(Real rdeg)
	{
		return Real((PI / 180.0) * rdeg);
	}

	inline
	Real
	radiansToDegrees(Real drad)
	{
		return Real((180.0 / PI) * drad);
	}

	inline
	std::ostream &
	operator<<(std::ostream &os, Real r)
	{
		os << r.dval();
		return os;
	}


	inline
	std::istream &
	operator>>(std::istream &is, Real& r)
	{
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
	 *
	 * @throw FunctionDomainException if @a r is less than 0.
	 */
	Real sqrt(Real r);

	/**
	 * Calculate the arc sine of @a r.
	 *
	 * @throws FunctionDomainException if @a r < -1 or @a r > 1.
	 */
	Real asin(Real r);

	/**
	 * Calculate the arc cosine of @a r.
	 *
	 * @throws FunctionDomainException if @a r < -1 or @a r > 1.
	 */
	Real acos(Real r);

	void export_Real();
}

#endif  // _GPLATES_MATHS_REAL_H_
