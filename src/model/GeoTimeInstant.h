/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_GEOTIMEINSTANT_H
#define GPLATES_MODEL_GEOTIMEINSTANT_H


namespace GPlatesModel {

	/**
	 * Instances of this class represent an instant in geological time, resolved and refined
	 * into a form which GPlates can efficiently process.
	 *
	 * This class is able to represent:
	 *  - time-instants with a specific time-position relative to the present-day;
	 *  - time-instants in the "distant past";
	 *  - time-instants in the "distant future".
	 */
	class GeoTimeInstant {

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
		value() const {
			return d_value;
		}

		/**
		 * Return true if this instance is a time-instant in the distant past; false
		 * otherwise.
		 */
		bool
		is_distant_past() const;

		/**
		 * Return true if this instance is a time-instant in the distant future; false
		 * otherwise.
		 */
		bool
		is_distant_future() const;

		/**
		 * Return true if this instance is a time-instant whose time-position may be
		 * expressed as a "real" floating-point number; false otherwise.
		 *
		 * The term "real" is used here to mean floating-point numbers which are meaningful
		 * for floating-point calculations (ie, not NaN) and are members of the set of Real
		 * numbers (ie, not positive-infinite or negative-infinite).
		 *
		 * If this function returns true, it implies that both @a is_distant_past and
		 * @a is_distant_future will return false.
		 */
		bool
		is_real() const;

		/**
		 * Return true if this instance is earlier than @a other; false otherwise.
		 */
		bool
		is_earlier_than(
				const GeoTimeInstant &other) const {
			return d_value > other.d_value;
		}

		/**
		 * Return true if this instance is later than @a other; false otherwise.
		 */
		bool
		is_later_than(
				const GeoTimeInstant &other) const {
			return d_value < other.d_value;
		}

		/**
		 * Return true if this instance is temporally-coincident with other; false
		 * otherwise.
		 */
		bool
		is_coincident_with(
				const GeoTimeInstant &other) const;

	private:

		double d_value;

	};

}

#endif  // GPLATES_MODEL_GEOTIMEINSTANT_H
