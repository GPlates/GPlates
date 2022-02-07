/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <iosfwd>
#include <boost/operators.hpp>

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"

#include "utils/QtStreamable.h"


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
	class GeoTimeInstant :
			public boost::less_than_comparable<GeoTimeInstant>,
			public boost::equality_comparable<GeoTimeInstant>,
			public boost::equivalent<GeoTimeInstant>,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<GeoTimeInstant>
	{
		struct TimePositionTypes
		{
			enum TimePositionType
			{
				Real,         // Finite
				DistantPast,  // +Infinity
				DistantFuture // -Infinity
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
		 *
		 * Note that the specified value can be positive or negative infinity
		 * (or you could use @a create_distant_past and @a create_distant_future instead).
		 *
		 * Note that the value cannot be NaN.
		 */
		explicit
		GeoTimeInstant(
				const double &value_);

		/**
		 * Access the floating-point representation of the time-position of this instance.
		 *
		 * Note that positive values represent times in the past; negative values represent
		 * times in the future.
		 *
		 * If @a is_real is false then the value returned is positive infinity if
		 * @a is_distant_past is true or negative infinity if @a is_distant_future is true.
		 */
		double
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
		 *
		 * This is essentially the same as 'operator==' (provided by Boost).
		 */
		bool
		is_coincident_with(
				const GeoTimeInstant &other) const;


		/**
		 * Less than comparison operator - all other operators supplied by Boost.
		 *
		 * Note: This is used (by boost::equivalent) to implement the equivalence relation:
		 *   !(x < y) && !(y < x)
		 * ...which holds for two values that are within epsilon of each other.
		 * This is also used, for example, by std::map to find elements (using above equivalence relation).
		 */
		bool
		operator<(
				const GeoTimeInstant &rhs) const;

	private:

		TimePositionTypes::TimePositionType d_type;
		double d_value;

		/**
		 * Create a GeoTimeInstant instance for a time-position type @a type_.
		 */
		explicit
		GeoTimeInstant(
				TimePositionTypes::TimePositionType type_,
				const double &value_) :
			d_type(type_),
			d_value(value_)
		{  }

	private: // Transcribe for sessions/projects...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<GeoTimeInstant> &geo_time_instant);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	std::ostream &
	operator<<(
			std::ostream &o,
			const GeoTimeInstant &g);
}

#endif  // GPLATES_PROPERTYVALUES_GEOTIMEINSTANT_H
