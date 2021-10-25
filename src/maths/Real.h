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

#ifndef GPLATES_MATHS_REAL_H
#define GPLATES_MATHS_REAL_H

#include <iosfwd>
#include <cmath>
#include <limits>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/operators.hpp>
#include <QDebug>
#include <QTextStream>

#include "MathsUtils.h"

#include "scribe/Transcribe.h"

#include "utils/QtStreamable.h"


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
	class Real :
			// NOTE: Base class chaining is used to avoid bloating sizeof(Real) due to multiple
			// inheritance from several empty base classes - this reduces sizeof(Real) from 16 to 8...
			public boost::less_than_comparable<Real,
					boost::equivalent<Real,
					boost::equality_comparable<Real> > >
			// NOTE: For same reason we are *not* inheriting from 'GPlatesUtils::QtStreamable<Real>'
			// and instead explicitly providing 'operator <<' overloads as non-member functions.
			//
			//public GPlatesUtils::QtStreamable<Real>
	{
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

		static
		Real
		quiet_nan();

		static
		Real
		positive_infinity();

		static
		Real
		negative_infinity();

	private:

		double _dval;

		friend bool are_almost_exactly_equal(const Real &, const Real &);
		friend bool are_slightly_more_strictly_equal(const Real &, const Real &);
		friend bool is_in_range(const Real &, const Real &, const Real &);
		friend bool operator<(const Real &, const Real &);
		friend std::ostream &operator<<(std::ostream &, const Real &);
		friend std::istream &operator>>(std::istream &, Real &);

	private: // Transcribing...

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);

		// Only the scribe system should be able to transcribe.
		friend class GPlatesScribe::Access;
	};


	std::ostream &
	operator<<(
			std::ostream &os,
			const Real &r);


	std::istream &
	operator>>(
			std::istream &,
			Real &r);


	/**
	 * Gives us:
	 *    qDebug() << r;
	 *    qWarning() << r;
	 *    qCritical() << r;
	 *    qFatal() << r;
	 */
	QDebug
	operator <<(
			QDebug dbg,
			const Real &r);


	/**
	 * Gives us:
	 *    QTextStream text_stream(device);
	 *    text_stream << r;
	 */
	QTextStream &
	operator <<(
			QTextStream &stream,
			const Real &r);


	/**
	 * Returns whether the two supplied real numbers @a r1 and @a r2 are
	 * equal to within the standard equality tolerance.
	 */
	inline
	bool
	are_almost_exactly_equal(
			const Real &r1,
			const Real &r2)
	{
		// Use the double version for consistency.
		return GPlatesMaths::are_almost_exactly_equal(r1._dval, r2._dval);
	}


	/**
	 * Returns whether the two supplied real numbers @a r1 and @a r2 are
	 * equal to within a slightly stricter tolerance than the standard
	 * equality tolerance, aka @a GPlatesMaths::EPSILON.
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
		// Use the double version for consistency.
		return GPlatesMaths::are_slightly_more_strictly_equal(r1._dval, r2._dval);
	}


	inline
	bool
	is_in_range(
			const Real &value,
			const Real &minimum,
			const Real &maximum)
	{
		// Use the double version for consistency.
		return GPlatesMaths::is_in_range(value._dval, minimum._dval, maximum._dval);
	}


	// All of the other operators are supplied by Boost operators.
	inline
	bool
	operator<(
			const Real &r1,
			const Real &r2)
	{
		double d = r2._dval - r1._dval;
		return d > GPlatesMaths::EPSILON;
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

	template<typename T>
	inline
	bool
	is_nan(
			T d)
	{
		return boost::math::isnan(d);
	}

	template<typename T>
	inline
	bool
	is_infinity(
			T d)
	{
		return boost::math::isinf(d);
	}

	template<typename T>
	inline
	bool
	is_positive_infinity(
			T d)
	{
		return boost::math::isinf(d) && d > 0;
	}

	template<typename T>
	inline
	bool
	is_negative_infinity(
			T d)
	{
		return boost::math::isinf(d) && d < 0;
	}

	template<typename T>
	inline
	bool
	is_finite(
			T d)
	{
		return boost::math::isfinite(d);
	}

	// The following assumes std::numeric_limits<double>::is_iec559 is true.
	// This is asserted for on application startup so it is something that we can
	// rely upon.

	template<typename T>
	inline
	T
	quiet_nan()
	{
		return std::numeric_limits<T>::quiet_NaN();
	}

	template<typename T>
	inline
	T
	positive_infinity()
	{
		return std::numeric_limits<T>::infinity();
	}

	template<typename T>
	inline
	T
	negative_infinity()
	{
		return -std::numeric_limits<T>::infinity();
	}
}

#endif  // GPLATES_MATHS_REAL_H
