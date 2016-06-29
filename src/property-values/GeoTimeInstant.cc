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

#include <ostream>
#include <cmath>

#include "GeoTimeInstant.h"

#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeDelegateProtocol.h"


const GPlatesPropertyValues::GeoTimeInstant
GPlatesPropertyValues::GeoTimeInstant::create_distant_past()
{
	return GeoTimeInstant(
			TimePositionTypes::DistantPast,
			GPlatesMaths::positive_infinity<double>());
}


const GPlatesPropertyValues::GeoTimeInstant
GPlatesPropertyValues::GeoTimeInstant::create_distant_future()
{
	return GeoTimeInstant(
			TimePositionTypes::DistantFuture,
			GPlatesMaths::negative_infinity<double>());
}


GPlatesPropertyValues::GeoTimeInstant::GeoTimeInstant(
		const double &value_) :
	d_value(value_)
{
	if (GPlatesMaths::is_finite(d_value))
	{
		d_type = TimePositionTypes::Real;
	}
	else if (GPlatesMaths::is_positive_infinity(d_value))
	{
		d_type = TimePositionTypes::DistantPast;
	}
	else if (GPlatesMaths::is_negative_infinity(d_value))
	{
		d_type = TimePositionTypes::DistantFuture;
	}
	else
	{
		// Encountered a NaN.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}


bool
GPlatesPropertyValues::GeoTimeInstant::is_strictly_earlier_than(
		const GeoTimeInstant &other) const
{
	if (d_type != other.d_type) {
		// We can compare the two geo-time instants purely by their time-position types.
		if (other.d_type == TimePositionTypes::DistantFuture) {
			// This geo-time must be either "distant past" or "real"; thus, 'this' is
			// earlier than 'other'.
			return true;
		}
		if (d_type == TimePositionTypes::DistantPast) {
			// The other geo-time must be either "real" or "distant future"; thus,
			// 'other' is later than 'this'.
			return true;
		}
		return false;
	} else if (d_type != TimePositionTypes::Real) {
		// The types are either both "distant past" or "distant future"; either way,
		// neither geo-time is either later or earlier than the other.
		return false;
	} else {
		// Both types are "real"; hence, the geo-times must be compared by "value".
		if (GPlatesMaths::are_geo_times_approximately_equal(d_value, other.d_value)) {
			return false;
		} else {
			// Don't forget that, since positive numbers indicate time instants in the
			// past, the larger the number, the further in the past.
			return (d_value > other.d_value);
		}
	}
}


bool
GPlatesPropertyValues::GeoTimeInstant::is_earlier_than_or_coincident_with(
		const GeoTimeInstant &other) const
{
	if (d_type != other.d_type) {
		// Since the types aren't the same, the geo-times can't be coincident, so in this
		// block we're comparing strictly for "earlier-than-ness".
		//
		// We can compare the two geo-time instants purely by their time-position types.
		if (other.d_type == TimePositionTypes::DistantFuture) {
			// This geo-time must be either "distant past" or "real"; thus, 'this' is
			// earlier than 'other'.
			return true;
		}
		if (d_type == TimePositionTypes::DistantPast) {
			// The other geo-time must be either "real" or "distant future"; thus,
			// 'other' is later than 'this'.
			return true;
		}
		return false;
	} else if (d_type != TimePositionTypes::Real) {
		// Both types are either "distant past" or "distant future"; either way, neither
		// geo-time is either later or earlier than the other; hence they are coincident.
		return true;
	} else {
		// Both types are "real"; hence, the geo-times must be compared by "value".
		if (GPlatesMaths::are_geo_times_approximately_equal(d_value, other.d_value)) {
			return true;
		} else {
			// Don't forget that, since positive numbers indicate time instants in the
			// past, the larger the number, the further in the past.
			return (d_value > other.d_value);
		}
	}
}


bool
GPlatesPropertyValues::GeoTimeInstant::is_coincident_with(
		const GeoTimeInstant &other) const
{
	if (d_type != other.d_type) {
		// At least one of the time-positions is "distant" (and if both are "distant",
		// they're not the same sort of "distant"), so there's no chance that these two
		// time-instants can be equal.
		return false;
	} else if (d_type != TimePositionTypes::Real) {
		// Both types are either "distant past" or "distant future".
		//
		// Note:  For now, if we encounter two time-instants which are both in the "distant past",
		// we're going to consider them equal.  (Even though we don't actually know what the times
		// in the "distant past" were, I think that it will be appropriate for the program to treat
		// the two time-instants in the same way, so we'll call them equal.)  Similarly for two
		// time-instants which are both in the "distant future".
		// Note: Comparing two "distant past" time instants (or two "distant future") as equal also
		// matches up with our current use of boost::equivalent to create "operator==" from
		// "operator<" and "operator>".
		return true;
	} else {
		// Both types are "real"; hence, the geo-times must be compared by "value".
		return GPlatesMaths::are_geo_times_approximately_equal(d_value, other.d_value);
	}
}


bool
GPlatesPropertyValues::GeoTimeInstant::operator<(
		const GeoTimeInstant &other) const
{
	if (d_type != other.d_type) {
		// Since the types aren't the same, the geo-times can't be coincident, so in this
		// block we're comparing strictly for "earlier-than-ness".
		//
		// We can compare the two geo-time instants purely by their time-position types.
		if (other.d_type == TimePositionTypes::DistantFuture) {
			// This geo-time must be either "distant past" or "real"; thus, 'this' is
			// earlier than 'other'.
			return true;
		}
		if (d_type == TimePositionTypes::DistantPast) {
			// The other geo-time must be either "real" or "distant future"; thus,
			// 'other' is later than 'this'.
			return true;
		}
		return false;
	} else if (d_type != TimePositionTypes::Real) {
		// Both types are either "distant past" or "distant future"; either way, neither
		// geo-time is either earlier than the other.
		return false;
	} else {
		// Both types are "real"; hence, the geo-times must be compared by "value".
		// Don't forget that, since positive numbers indicate time instants in the
		// past, the larger the number, the further in the past.
		// NOTE: By using the epsilon comparison we satisfy:
		//   !(x < y) && !(y < x)
		// ...for two values that are within epsilon of each other and hence satisfy the
		// equivalence relation and so can be used to find elements in a std::map for example.
		const double diff = d_value - other.d_value;
		return diff > GPlatesMaths::GEO_TIMES_EPSILON;
	}
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GeoTimeInstant::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GeoTimeInstant> &geo_time_instant)
{
	//
	// Using transcribe delegate protocol so that GeoTimeInstant, Real and double/float
	// can be used interchangeably (ie, are transcription compatible) except NaN is not
	// supported by GeoTimeInstant (+/- Infinity is OK though).
	//
	// So we only transcribe the value. The type can be inferred from the value.
	//
	// Note that +/- Infinity and NaN (float and double) are handled properly by the
	// scribe archive writers/readers.
	//
	if (scribe.is_saving())
	{
		transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, geo_time_instant->d_value);
	}
	else // loading...
	{
		double value;

		const GPlatesScribe::TranscribeResult transcribe_result =
				transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, value);
		if (transcribe_result != GPlatesScribe::TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		//
		// Set the 'type' to distant past/future if value is +/- Infinity, otherwise real.
		//
		TimePositionTypes::TimePositionType type;
		if (GPlatesMaths::is_finite(value))
		{
			type = TimePositionTypes::Real;
		}
		else if (GPlatesMaths::is_positive_infinity(value))
		{
			type = TimePositionTypes::DistantPast;
		}
		else if (GPlatesMaths::is_negative_infinity(value))
		{
			type = TimePositionTypes::DistantFuture;
		}
		else // NaN ...
		{
			// NaN is not supported for GeoTimeInstant.
			return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
		}

		geo_time_instant.construct_object(type, value);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GeoTimeInstant::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// If not already transcribed in 'transcribe_construct_data()'.
	if (!transcribed_construct_data)
	{
		//
		// Using transcribe delegate protocol so that GeoTimeInstant, Real and double/float
		// can be used interchangeably (ie, are transcription compatible) except NaN is not
		// supported by GeoTimeInstant (+/- Infinity is OK though).
		//
		// So we only transcribe the value. The type can be inferred from the value.
		//
		// Note that +/- Infinity and NaN (float and double) are handled properly by the
		// scribe archive writers/readers.
		//
		const GPlatesScribe::TranscribeResult transcribe_result =
				transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, d_value);
		if (transcribe_result != GPlatesScribe::TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		if (scribe.is_loading())
		{
			//
			// Set the 'type' to distant past/future if value is +/- Infinity, otherwise real.
			//
			if (GPlatesMaths::is_finite(d_value))
			{
				d_type = TimePositionTypes::Real;
			}
			else if (GPlatesMaths::is_positive_infinity(d_value))
			{
				d_type = TimePositionTypes::DistantPast;
			}
			else if (GPlatesMaths::is_negative_infinity(d_value))
			{
				d_type = TimePositionTypes::DistantFuture;
			}
			else // NaN ...
			{
				// NaN is not supported for GeoTimeInstant.
				return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
			}
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &o,
		const GeoTimeInstant &g)
{
	if (g.is_real()) {
		o << g.value();
	} else if (g.is_distant_past()) {
		o << "(distant past)";
	} else {
		o << "(distant future)";
	}

	return o;
}
