/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GEOTIMEINSTANT_H
#define GPLATES_PROPERTYVALUES_GEOTIMEINSTANT_H


namespace GPlatesPropertyValues
{
	/**
	 * Instances of this class represent an instant in geological time, resolved and refined
	 * into a form which GPlates can efficiently process.
	 *
	 * This class is able to represent:
	 *  - time-instants with a specific time-position relative to the present-day;
	 *  - time-instants in the "distant past";
	 *  - time-instants in the "distant future".
	 */
	class GeoTimeInstant
	{
		/*
		 * Implementation note:
		 *
		 * Previously, class GeoTimeInstant contained only a single member datum of type
		 * 'double'.  The time-positions for the distant past and the distant future were
		 * represented using the floating-point values for infinity and minus-infinity
		 * (respectively).
		 * 
		 * However, when Glen was porting to Windows (specifically, the Microsoft Visual
		 * Studio C++ compiler), he pointed out that:
		 *
		 * > 1. INFINITY, isinf(), and std::isfinite() are all non-standard: and MSVC
		 * > does not have them.
		 * >
		 * > It does have _finite() and _isnan(), unless we can find some portable way
		 * > of doing things instead.
		 * >
		 * > 2. My understand was that you are not intended to store positive infinity,
		 * > negative infinity, or NaN in double or float variables - only test
		 * > variables for them with functions like isnan, isinf (which unfortunately
		 * > are non-standard).
		 * >
		 * > This is because on a particular implementation it is not required that
		 * > double/float can store these values.
		 * >
		 * > Is this correct? If so, the use of INFINITY and -INFINITY in GeoTimeInstant
		 * > would need to change.
		 *
		 * I looked into this issue, and determined that Glen was correct.  Specifically,
		 *  1. The macros and functions 'INFINITY', 'isinf' and 'isfinite' are part of
		 *     C99, which of course is not part of the C++ standard.
		 *  2. While the IEEE 754 floating-point standard does indeed define infinity,
		 *     etc., the C++ standard doesn't require that a C++ implementation use IEEE
		 *     754 for floating-point numbers.
		 *
		 * For more info, take a look at these links which Google found for me:
		 *  http://groups.google.com/group/comp.lang.c++/browse_thread/thread/e1cea36d29c4b708/0149a9483297f8e8
		 *  http://www.thescripts.com/forum/thread165462.html
		 *  http://gcc.gnu.org/ml/libstdc++/2002-09/msg00038.html
		 *
		 * Hence, I modified class GeoTimeInstant to use the TimePositionType enumeration
		 * instead.
		 */
		struct TimePositionTypes
		{
			enum TimePositionType
			{
				Real,
				DistantPast,
				DistantFuture
			};
		};

	public:

		/**
		 * Create a GeoTimeInstant instance for the distant past.
		 *
		 * This is basically creating an instance for a time-instant which is infinitely
		 * far in the past, as if we'd created a GeoTimeInstant with a time-position value
		 * of infinity.
		 *
		 * All distant-past time-instants will compare earlier than all non-distant-past
		 * time-instants.
		 */
		static
		const GeoTimeInstant
		create_distant_past();

		/**
		 * Create a GeoTimeInstant instance for the distant future.
		 *
		 * This is basically creating an instance for a time-instant which is infinitely
		 * far in the future, as if we'd created a GeoTimeInstant with a time-position
		 * value of minus-infinity.
		 *
		 * All distant-future time-instants will compare later than all non-distant-future
		 * time-instants.
		 */
		static
		const GeoTimeInstant
		create_distant_future();

		/**
		 * Create a GeoTimeInstant instance for a time-position of @a value million
		 * years ago.
		 *
		 * Note that positive values represent times in the past; negative values represent
		 * times in the future.
		 */
		explicit
		GeoTimeInstant(
				const double &value_):
			d_type(TimePositionTypes::Real),
			d_value(value_)
		{  }

		/**
		 * Access the floating-point representation of the time-position of this instance.
		 *
		 * Note that positive values represent times in the past; negative values represent
		 * times in the future.
		 *
		 * Note that this value may not be meaningful if @a is_real returns false.
		 */
		const double &
		value() const
		{
			return d_value;
		}

		/**
		 * Return true if this instance is a time-instant in the distant past; false
		 * otherwise.
		 */
		bool
		is_distant_past() const
		{
			return (d_type == TimePositionTypes::DistantPast);
		}

		/**
		 * Return true if this instance is a time-instant in the distant future; false
		 * otherwise.
		 */
		bool
		is_distant_future() const
		{
			return (d_type == TimePositionTypes::DistantFuture);
		}

		/**
		 * Return true if this instance is a time-instant whose time-position may be
		 * expressed as a "real" floating-point number; false otherwise.
		 *
		 * The term "real" is used here to mean floating-point numbers which are meaningful
		 * for floating-point calculations (ie, not NaN) and are members of the set of Real
		 * numbers (ie, not positive-infinity or negative-infinity, which are members of
		 * the set of Extended Real numbers).
		 *
		 * If this function returns true, it implies that both @a is_distant_past and
		 * @a is_distant_future will return false.
		 */
		bool
		is_real() const
		{
			return (d_type == TimePositionTypes::Real);
		}

		/**
		 * Return true if this instance is strictly earlier than @a other; false otherwise.
		 */
		bool
		is_strictly_earlier_than(
				const GeoTimeInstant &other) const;

		/**
		 * Return true if this instance is either earlier than @a other or
		 * temporally-coincident with @a other; false otherwise.
		 */
		bool
		is_earlier_than_or_coincident_with(
				const GeoTimeInstant &other) const;

		/**
		 * Return true if this instance is later than @a other; false otherwise.
		 */
		bool
		is_strictly_later_than(
				const GeoTimeInstant &other) const
		{
			return ! is_earlier_than_or_coincident_with(other);
		}

		/**
		 * Return true if this instance is either later than @a other or
		 * temporally-coincident with @a other; false otherwise.
		 */
		bool
		is_later_than_or_coincident_with(
				const GeoTimeInstant &other) const
		{
			return ! is_strictly_earlier_than(other);
		}

		/**
		 * Return true if this instance is temporally-coincident with @a other; false
		 * otherwise.
		 */
		bool
		is_coincident_with(
				const GeoTimeInstant &other) const;

	private:

		TimePositionTypes::TimePositionType d_type;
		double d_value;

		/**
		 * Create a GeoTimeInstant instance for a time-position type @a type_.
		 *
		 * This constructor should only be used to instantiate GeoTimeInstant instances in
		 * which the time-position type is @em not "Real".  Hence, this constructor is
		 * private:  The static member functions @a create_distant_past and
		 * @a create_distant_future are used to invoke this constructor with the
		 * appropriate value.
		 */
		explicit
		GeoTimeInstant(
				TimePositionTypes::TimePositionType type_):
			d_type(type_),
			d_value(0.0)
		{  }

	};


	inline
	bool
	operator==(
			const GeoTimeInstant &g1,
			const GeoTimeInstant &g2)
	{
		return g1.is_coincident_with(g2);
	}
}

#endif  // GPLATES_PROPERTYVALUES_GEOTIMEINSTANT_H
