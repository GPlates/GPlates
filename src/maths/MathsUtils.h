/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_MATHUTILS_H
#define GPLATES_MATHS_MATHUTILS_H

#include <limits>
#include <cmath>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_integral.hpp>


namespace GPlatesMaths
{
	/*
	 * The "standard" epsilon used in GPlates for floating-point comparisons.
	 * Used by @a are_almost_exactly_equal.
	 *
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
	static const double EPSILON = 1.0e-12;

	/**
	 * A tighter epsilon: TIGHTER_EPSILON < EPSILON.
	 * Used by @a are_slightly_more_strictly_equal.
	 */
	static const double TIGHTER_EPSILON = 9.99e-13;

	/**
	 * An epsilon suitable for the comparison of geological times.
	 *
	 * For what it's worth, this represents a precision of about eight-and-three-quarter hours,
	 * which is not too bad for geological time.
	 */
	static const double GEO_TIMES_EPSILON = 1.0e-9;

	template<class T>
	inline
	bool
	are_almost_exactly_equal(
			T value1,
			T value2)
	{
		double d = value1 - value2;
		return -GPlatesMaths::EPSILON <= d && d <= GPlatesMaths::EPSILON;
	}
	
	template<class T>
	inline
	bool
	are_slightly_more_strictly_equal(
			T value1,
			T value2)
	{
		double d = value1 - value2;
		return -GPlatesMaths::TIGHTER_EPSILON <= d && d <= GPlatesMaths::TIGHTER_EPSILON;
	}

	/**
	 * JB's original commentary moved here from FloatingPointComparisons.h:
	 *
	 * Determine whether the two geological times @a geo_time1 and @a geo_time2 are
	 * equal (within a small epsilon).
	 *
	 * For an explanation of why this function is necessary, read the article "What
	 * Every Computer Scientist Should Know About Floating-Point Arithmetic" by David
	 * Goldberg: http://docs.sun.com/source/806-3568/ncg_goldberg.html
	 *
	 * I realise that using an epsilon for equality comparison is a little questionable
	 * -- the Goldberg paper even states this: 
	 * 	Incidentally, some people think that the solution to such anomalies is
	 * 	never to compare floating-point numbers for equality, but instead to
	 * 	consider them equal if they are within some error bound E.  This is hardly
	 * 	a cure-all because it raises as many questions as it answers.  What should
	 * 	the value of E be?  If x < 0 and y > 0 are within E, should they really be
	 * 	considered to be equal, even though they have different signs?
	 *
	 * My justification for using an epsilon in this case is that I'm expecting
	 * geological times to be confined to the range [0.001, 10000.000] and I believe
	 * I've chosen an epsilon which correctly and usefully covers this range.
	 */
	template<class T>
	inline
	bool
	are_geo_times_approximately_equal(
			T value1,
			T value2)
	{
		double d = value1 - value2;
		return -GPlatesMaths::GEO_TIMES_EPSILON <= d && d <= GPlatesMaths::GEO_TIMES_EPSILON;
	}

	template<class T>
	inline
	bool
	is_in_range(
			T value,
			T minimum,
			T maximum)
	{
		return (minimum - GPlatesMaths::EPSILON) <= value &&
			value <= (maximum + GPlatesMaths::EPSILON);
	}

	/**
	 * \f$ \pi \f$, the ratio of the circumference to the diameter of a circle.
	 */
	static const double PI = 3.14159265358979323846264338;

	/**
	 * \f$ \frac{\pi}{2} \f$.
	 */
	static const double HALF_PI = 1.57079632679489661923;


	namespace Implementation
	{
		template<typename T>
		inline
		const T
		convert_deg_to_rad(
				const T &value_in_degrees,
				boost::false_type)
		{
			return T((PI / 180.0) * value_in_degrees);
		}

		inline
		double
		convert_deg_to_rad(
				int value_in_degrees,
				boost::true_type)
		{
			// Avoid integer truncation.
			return (PI / 180.0) * value_in_degrees;
		}


		template<typename T>
		inline
		const T
		convert_rad_to_deg(
				const T &value_in_radians,
				boost::false_type)
		{
			return T((180.0 / PI) * value_in_radians);
		}

		inline
		double
		convert_rad_to_deg(
				int value_in_radians,
				boost::true_type)
		{
			// Avoid integer truncation.
			return (180.0 / PI) * value_in_radians;
		}
	}

	/**
	 * Converts degrees to radians.
	 *
	 * Supports type 'T' of integral, floating-point and 'real_t'.
	 */
	template<typename T>
	inline
	const typename boost::mpl::if_<boost::is_integral<T>, double, T>::type
	convert_deg_to_rad(
			const T &value_in_degrees)
	{
		// Note that we need to handle floating-point truncation if @a value_in_degrees
		// happens to be an integer literal.
		return Implementation::convert_deg_to_rad(value_in_degrees, typename boost::is_integral<T>::type());
	}


	/**
	 * Converts radians to degrees.
	 *
	 * Supports type 'T' of integral, floating-point and 'real_t'.
	 */
	template<typename T>
	inline
	const typename boost::mpl::if_<boost::is_integral<T>, double, T>::type
	convert_rad_to_deg(
			const T &value_in_radians)
	{
		// Note that we need to handle floating-point truncation if @a value_in_degrees
		// happens to be an integer literal.
		return Implementation::convert_rad_to_deg(value_in_radians, typename boost::is_integral<T>::type());
	}


	/**
	 * Terminates the application with an error message if the float and double
	 * built-in types do not have infinity and NaN.
	 */
	void
	assert_has_infinity_and_nan();
}

#endif  // GPLATES_MATHS_MATHUTILS_H
