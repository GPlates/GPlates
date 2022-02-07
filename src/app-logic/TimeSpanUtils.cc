/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "TimeSpanUtils.h"


GPlatesAppLogic::TimeSpanUtils::TimeRange::TimeRange(
		const double &begin_time,
		const double &end_time,
		const double &time_increment,
		Adjust adjust) :
	d_begin_time(begin_time),
	d_end_time(end_time),
	d_time_increment(time_increment),
	// Calculate the number of time slots rounded up to the nearest integer.
	d_num_time_slots(calc_num_time_slots(begin_time, end_time, time_increment))
{
	// Modify begin, end time or time increment to satisfy the constraints:
	//
	//   begin_time = end_time + (num_time_slots - 1) * time_increment
	//   num_time_slots >= 2
	//
	// Hence, for example, the begin time can be earlier in the past (or the end time later)
	// than the actual begin time (end time) passed by the caller.
	//
	// For example, for begin_time = 12.1 and end_time = 10.0 and time_increment = 1.0,
	// we get four time slots which are at times 13.0, 12.0, 11.0 and 10.0 and they bound the
	// three time intervals [13.0, 12.0], [12.0, 11.0] and [11.0, 10.0].
	// So 'd_begin_time' = 13.0 and 'd_end_time' = 10.0.
	//
	switch (adjust)
	{
	case ADJUST_BEGIN_TIME:
		d_begin_time = d_end_time + (d_num_time_slots - 1) * d_time_increment;
		break;

	case ADJUST_END_TIME:
		d_end_time = d_begin_time - (d_num_time_slots - 1) * d_time_increment;
		break;

	case ADJUST_TIME_INCREMENT:
		d_time_increment = (d_begin_time - d_end_time) / (d_num_time_slots - 1);
		break;

	default:
		// Shouldn't be able to get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


GPlatesAppLogic::TimeSpanUtils::TimeRange::TimeRange(
		const double &begin_time,
		const double &end_time,
		unsigned int num_time_slots) :
	d_begin_time(begin_time),
	d_end_time(end_time),
	d_num_time_slots(num_time_slots)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			begin_time > end_time &&
				num_time_slots >= 2,
			GPLATES_ASSERTION_SOURCE);

	d_time_increment = (d_begin_time - d_end_time) / (d_num_time_slots - 1);
}


boost::optional<unsigned int>
GPlatesAppLogic::TimeSpanUtils::TimeRange::get_time_slot(
		const double &time) const
{
	double interpolate_position;
	boost::optional< std::pair<unsigned int/*first_time_slot*/, unsigned int/*second_time_slot*/> >
			bounding_time_slots_opt = get_bounding_time_slots(time, interpolate_position);
	if (!bounding_time_slots_opt)
	{
		return boost::none;
	}

	const std::pair<unsigned int, unsigned int> &bounding_time_slots = bounding_time_slots_opt.get();

	// If the specified time does not lie on an integer multiple of time increment.
	if (bounding_time_slots.first != bounding_time_slots.second)
	{
		return boost::none;
	}

	return bounding_time_slots.first;
}


boost::optional<unsigned int>
GPlatesAppLogic::TimeSpanUtils::TimeRange::get_nearest_time_slot(
		const double &time) const
{
	double interpolate_position;
	boost::optional< std::pair<unsigned int/*first_time_slot*/, unsigned int/*second_time_slot*/> >
			bounding_time_slots_opt = get_bounding_time_slots(time, interpolate_position);
	if (!bounding_time_slots_opt)
	{
		return boost::none;
	}

	const std::pair<unsigned int, unsigned int> &bounding_time_slots = bounding_time_slots_opt.get();

	// This also works when both time slots are equal (and 'interpolate_position == 0').
	return interpolate_position < 0.5
			? bounding_time_slots.first
			: bounding_time_slots.second;
}


boost::optional< std::pair<unsigned int/*begin_time_slot*/, unsigned int/*end_time_slot*/> >
GPlatesAppLogic::TimeSpanUtils::TimeRange::get_bounding_time_slots(
		const double &time,
		double &interpolate_position) const
{
	// Detect whether the time is outside the time range or at the begin/end time (within epsilon).
	if (time > d_begin_time - GPlatesMaths::GEO_TIMES_EPSILON)
	{
		if (time > d_begin_time + GPlatesMaths::GEO_TIMES_EPSILON)
		{
			// Outside time range.
			return boost::none;
		}

		// It's very close to zero - let's make it exactly zero.
		interpolate_position = 0.0;

		// Return two identical time slots to indicate we're on the first time slot and
		// hence no interpolation is necessary.
		const unsigned int begin_time_slot = 0;
		return std::make_pair(begin_time_slot, begin_time_slot);
	}
	if (time < d_end_time + GPlatesMaths::GEO_TIMES_EPSILON)
	{
		if (time < d_end_time - GPlatesMaths::GEO_TIMES_EPSILON)
		{
			// Outside time range.
			return boost::none;
		}

		// It's very close to zero - let's make it exactly zero.
		interpolate_position = 0.0;

		// Return two identical time slots to indicate we're on the last time slot and
		// hence no interpolation is necessary.
		const unsigned int end_time_slot = d_num_time_slots - 1;
		return std::make_pair(end_time_slot, end_time_slot);
	}

	double first_time_slot_integer;
	interpolate_position = std::modf((d_begin_time - time) / d_time_increment, &first_time_slot_integer);

	// Convert to integer (it's already integral - the 0.5 is just to avoid numerical precision issues).
	const unsigned int first_time_slot = static_cast<int>(first_time_slot_integer + 0.5);

	// See if we're very close to being exactly on a time slot.
	if (interpolate_position < GPlatesMaths::GEO_TIMES_EPSILON)
	{
		// It's very close to zero - let's make it exactly zero.
		interpolate_position = 0.0;

		// Return two identical time slots to indicate we're on a time slot and
		// hence no interpolation is necessary.
		return std::make_pair(first_time_slot, first_time_slot);
	}
	else if (interpolate_position > 1 - GPlatesMaths::GEO_TIMES_EPSILON)
	{
		// It's very close to one - let's make it exactly zero (and increment the integer time slot).
		interpolate_position = 0.0;

		// Return two identical time slots to indicate we're on a time slot and
		// hence no interpolation is necessary.
		return std::make_pair(first_time_slot + 1, first_time_slot + 1);
	}

	return std::make_pair(first_time_slot, first_time_slot + 1);
}


unsigned int
GPlatesAppLogic::TimeSpanUtils::TimeRange::calc_num_time_slots(
		const double &begin_time,
		const double &end_time,
		const double &time_increment)
{
	// Rounds up to the nearest integer.
	const double round_threshold = 1e-6;

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			begin_time > end_time &&
				time_increment > 0 &&
				// Ensure we get at least two time slots (which is one time interval)...
				begin_time - end_time > round_threshold * time_increment,
			GPLATES_ASSERTION_SOURCE);

	// Casting to 'int' is optimised on Visual Studio compared to 'unsigned int'.
	// The '1' converts intervals to slots (eg, two intervals is bounded by three fence posts or slots).
	// The '1-1e-6' rounds up to the nearest integer.
	//
	// For example, for begin_time = 12.1 and end_time = 10.0 and time_increment = 1.0,
	// we get four time slots which are at times 13.0, 12.0, 11.0 and 10.0 and they are
	// bound by the three time intervals [13.0, 12.0], [12.0, 11.0] and [11.0, 10.0].
	const unsigned int num_time_slots = 1 + static_cast<int>(
			(1 - round_threshold) + (begin_time - end_time) / time_increment);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_time_slots >= 2,
			GPLATES_ASSERTION_SOURCE);

	return num_time_slots;
}
