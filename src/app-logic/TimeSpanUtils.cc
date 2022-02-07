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
	//   begin_time = end_time + num_time_slots * time_increment
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


double
GPlatesAppLogic::TimeSpanUtils::TimeRange::get_time(
		unsigned int time_slot) const
{
	return d_begin_time - time_slot * d_time_increment;
}


boost::optional<unsigned int>
GPlatesAppLogic::TimeSpanUtils::TimeRange::get_time_slot(
		const double &time) const
{
	if (GPlatesMaths::are_geo_times_approximately_equal(time, d_begin_time))
	{
		return static_cast<unsigned int>(0);
	}

	if (GPlatesMaths::are_geo_times_approximately_equal(time, d_end_time))
	{
		// Note that there's always at least two time slots.
		return get_num_time_slots() - 1;
	}

	// We don't do this before epsilon testing equality with begin/end time because that would
	// discard times that are very close to begin/end time (yet outside exact time range).
	if (time > d_begin_time || time < d_end_time)
	{
		// Outside time span.
		return boost::none;
	}

	double delta_time_div_time_increment;
	const double delta_time_mod_time_increment =
			std::modf((d_begin_time - time) / d_time_increment, &delta_time_div_time_increment);

	// See if the specified time lies at an integer multiple of time increment.
	if (!GPlatesMaths::are_almost_exactly_equal(delta_time_mod_time_increment, 0.0))
	{
		return boost::none;
	}

	// Convert to integer (it's already integral - the 0.5 is just to avoid numerical precision issues).
	const unsigned int time_slot = static_cast<int>(delta_time_div_time_increment + 0.5);

	return time_slot;
}


boost::optional<unsigned int>
GPlatesAppLogic::TimeSpanUtils::TimeRange::get_nearest_time_slot(
		const double &time) const
{
	if (time > d_begin_time || time < d_end_time)
	{
		// Outside time span.
		return boost::none;
	}

	// The 0.5 is to round to the nearest integer.
	// Casting to 'int' is optimised on Visual Studio compared to 'unsigned int'.
	const unsigned int time_slot = static_cast<int>(0.5 + (d_begin_time - time) / d_time_increment);

	return time_slot;
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
	// bound the three time intervals [13.0, 12.0], [12.0, 11.0] and [11.0, 10.0].
	const unsigned int num_time_slots = 1 + static_cast<int>(
			(1 - round_threshold) + (begin_time - end_time) / time_increment);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_time_slots >= 2,
			GPLATES_ASSERTION_SOURCE);

	return num_time_slots;
}
