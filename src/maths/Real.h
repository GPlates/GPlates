/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_REAL_H
#define GPLATES_MATHS_REAL_H

#include <iostream>
#include <cmath>


namespace GPlatesMaths
{
	// Note: PI and HALF_PI are defined in MathsUtils.h.

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

		const double &
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
		is_precisely_greater_than(double d) const
		{
			return _dval > d;
		}

		bool
		is_precisely_less_than(double d) const
		{
			return _dval < d;
		}

		bool
		is_nan() const;

		bool
		is_infinity() const;

		bool
		is_positive_infinity() const;

		bool
		is_negative_infinity() const;

		bool
		is_finite() const;

		bool
		is_zero() const
		{
			return *this == Real(0.0);
		}

		static
		Real
		nan();

		static
		Real
		positive_infinity();

		static
		Real
		negative_infinity();

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
	const Real
	abs(
			const Real &r1)
	{
		return Real(fabs(r1.dval()));
	}


	/**
	 * Using the exact value of the Real, return whether it is positive
	 * (ie. greater than exact zero) or not.
	 */
	inline
	bool
	is_strictly_positive(
			const Real &r)
	{
		return r.dval() > 0.0;
	}


	/**
	 * Using the exact value of the Real, return whether it is negative
	 * (ie. less than exact zero) or not.
	 */
	inline
	bool
	is_strictly_negative(
			const Real &r)
	{
		return r.dval() < 0.0;
	}


	/**
	 * Using the exact value of the Real, return whether it is greater
	 * than exact one or not.
	 */
	inline
	bool
	is_strictly_greater_than_one(
			const Real &r)
	{
		return r.dval() > 1.0;
	}


	/**
	 * Using the exact value of the Real, return whether it is greater
	 * than exact minus-one or not.
	 */
	inline
	bool
	is_strictly_less_than_minus_one(
			const Real &r)
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
	 * Calculate the square-root of @a r.
	 *
	 * @a r must be non-negative.
	 *
	 * The return-value will be non-negative.
	 *
	 * @throw FunctionDomainException if @a r is less than 0.
	 */
	const Real
	sqrt(
			const Real &r);

	/**
	 * Calculate the arc sine of @a r.
	 *
	 * @r must lie in the valid domain of the arc sine function, the closed range [-1, 1].
	 *
	 * The return-value will lie in the closed range [-PI/2, PI/2].
	 *
	 * Don't forget: the arc sine will be returned in radians, not degrees!
	 *
	 * @throws FunctionDomainException if @a r < -1 or @a r > 1.
	 */
	const Real
	asin(
			const Real &r);

	/**
	 * Calculate the arc cosine of @a r.
	 *
	 * @r must lie in the valid domain of the arc cosine function, the closed range [-1, 1].
	 *
	 * The return-value will lie in the closed range [0, PI].
	 *
	 * Don't forget: the arc cosine will be returned in radians, not degrees!
	 *
	 * @throws FunctionDomainException if @a r < -1 or @a r > 1.
	 */
	const Real
	acos(
			const Real &r);

	/**
	 * Calculate the two-variable arc tangent of @a y and @a x.
	 *
	 * The return-value will lie in the open-closed range [-PI, PI].
	 *
	 * Don't forget: the arc tangent will be returned in radians, not degrees!
	 *
	 * Note that, unlike some implementations, this function defines atan2(0, 0) to be 0; thus,
	 * there are no invalid regions on the domain.
	 */
	const Real
	atan2(
			const Real &y,
			const Real &x);

	bool
	is_nan(double d);

	bool
	is_infinity(double d);

	bool
	is_positive_infinity(double d);

	bool
	is_negative_infinity(double d);

	bool
	is_finite(double d);

	bool
	is_zero(double d);

	double
	nan();

	double
	positive_infinity();

	double
	negative_infinity();

}

#endif  // GPLATES_MATHS_REAL_H
