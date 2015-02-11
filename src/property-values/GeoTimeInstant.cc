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

#include "maths/MathsUtils.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::GeoTimeInstant
GPlatesPropertyValues::GeoTimeInstant::create_distant_past()
{
	return GeoTimeInstant(TimePositionTypes::DistantPast);
}


const GPlatesPropertyValues::GeoTimeInstant
GPlatesPropertyValues::GeoTimeInstant::create_distant_future()
{
	return GeoTimeInstant(TimePositionTypes::DistantFuture);
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
	}

	// Note:  For now, if we encounter two time-instants which are both in the "distant past",
	// we're going to consider them equal.  (Even though we don't actually know what the times
	// in the "distant past" were, I think that it will be appropriate for the program to treat
	// the two time-instants in the same way, so we'll call them equal.)  Similarly for two
	// time-instants which are both in the "distant future".
	//
	// Note that all geo-times in the distant past and the distant future have their "value"s
	// initialised to 0.0, so all geo-times in the distant past will compare equal by
	// comparison of "value", as will all geo-times in the distant future.
	return (GPlatesMaths::are_geo_times_approximately_equal(d_value, other.d_value));
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
GPlatesPropertyValues::GeoTimeInstant::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_type, "d_type") ||
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_value, "d_value"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GeoTimeInstant::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GeoTimeInstant> &geo_time_instant)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, geo_time_instant->d_type, "d_type");
		scribe.save(TRANSCRIBE_SOURCE, geo_time_instant->d_value, "d_value");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<TimePositionTypes::TimePositionType> type =
				scribe.load<TimePositionTypes::TimePositionType>(TRANSCRIBE_SOURCE, "d_type");
		if (!type.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<double> value = scribe.load<double>(TRANSCRIBE_SOURCE, "d_value");
		if (!value.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		if (type == TimePositionTypes::Real)
		{
			geo_time_instant.construct_object(value);
			geo_time_instant->d_type = type;
		}
		else
		{
			geo_time_instant.construct_object(type);
			geo_time_instant->d_value = value;
		}

		scribe.relocated(TRANSCRIBE_SOURCE, geo_time_instant->d_type, type);
		scribe.relocated(TRANSCRIBE_SOURCE, geo_time_instant->d_value, value);
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
