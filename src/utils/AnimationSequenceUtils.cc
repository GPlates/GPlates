/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <cmath>
#include "AnimationSequenceUtils.h"

#include "maths/MathsUtils.h"


GPlatesUtils::AnimationSequence::SequenceInfo
GPlatesUtils::AnimationSequence::calculate_sequence(
		const double &start_time,
		const double &end_time,
		const double &abs_time_increment,
		bool should_finish_exactly_on_end_time)
{
	// Reconstruction time increment should not be zero.
	if (GPlatesMaths::are_geo_times_approximately_equal(abs_time_increment, 0.0))
	{
		throw AnimationSequence::TimeIncrementZero(GPLATES_EXCEPTION_SOURCE);
	}

	SequenceInfo seq;
	// Copy in the desired range params, etc, as potentially useful metadata.
	seq.desired_start_time = start_time;
	seq.desired_end_time = end_time;
	seq.abs_time_increment = std::fabs(abs_time_increment);
	seq.raw_time_increment = raw_time_increment(start_time, end_time, abs_time_increment);
	seq.should_finish_exactly_on_end_time = should_finish_exactly_on_end_time;

	// We always play the very first frame (@a start_time);
	static const size_type first_frame = 1;
	
	// Find out how many steps we could go through the given time range.
	double available_range = std::fabs(seq.desired_end_time - seq.desired_start_time);
	double available_steps = std::floor(available_range / seq.abs_time_increment);

	// Okay, so if we were to step through that, how much non-animated 'slack space'
	// would be left at the end?
	double steppable_range = seq.abs_time_increment * available_steps;
	double time_remainder = available_range - steppable_range;

	// Here's the tricky part, thanks to our friend the double.
	// If @a time_remainder is close to 0, that means that our @a available_range
	// (supplied by the user) probably neatly divides by the desired increment.
	//
	// On the other hand, if @a time_remainder is nowhere near 0, the user
	// is requesting a range which does not actually have an integer multiple of the
	// increment in there, and we may have to add an artificial extra frame on
	// the end (according to @a should_finish_exactly_on_end_time).
	//
	// The real mindfuck actually comes from the first case though:-
	// When @a time_remainder is close to 0 but >0, it means we had a little bit of
	// leftover space at the end (but nothing serious), and @a available_steps
	// was calculated with std::floor(some number like 19.99998). We need to add
	// 1 to our @a available_steps.
	// When @a time_remainder is close to 0 but <=0, which might just possibly happen,
	// it means our calculation of the @a steppable_range actually went over the 
	// original @a available_range by a tiny amount, thanks once again to doubles.
	// In this case, we have calculated @a available_steps with something like
	// std::floor(some number like 20.00002), and blindly adding an additional
	// @a end_time() step would be a fencepost error. Leave @a available_steps as-is.
	if (GPlatesMaths::are_geo_times_approximately_equal(time_remainder, 0.0)) {
		// Okay, requested range divides approximately by an integer multiple,
		// but we need to correct the @a available_steps calculation depending on
		// whether we were slightly over or slightly under.
		size_type available_frame_steps = static_cast<size_type>(available_steps);
		if (time_remainder > 0) {
			// Tiny extra leftover space at end, add one extra frame to @available_steps.
			available_frame_steps++;
		} else {
			// @a available_steps calculation overshot by a tiny amount, no need
			// for adjustment.
		}
		
		// Note that in this case, the value of @a should_finish_exactly_on_end_time
		// is irrelevant.
		seq.includes_remainder_frame = false;
		seq.remainder_frame_length = 0.0;
		
		// With all that taken care of, we can calculate the correct duration.
		seq.duration_in_frames = first_frame + available_frame_steps;
		seq.duration_in_ma = seq.abs_time_increment * available_frame_steps;
		
		// It is safe to assume that actual start/end times match the desired ones.
		seq.actual_start_time = seq.desired_start_time;
		seq.actual_end_time = seq.desired_end_time;
	} else {
		// @a time_remainder is nowhere near 0, requested range does not divide
		// neatly by increment.
		// We don't need to worry about floating point error being accumulated,
		// but we do need to account for that last frame - if the user wants it
		// to be played.
		size_type available_frame_steps = static_cast<size_type>(available_steps);
		
		// In this case, @a should_finish_exactly_on_end_time needs to be taken
		// into account.
		seq.includes_remainder_frame = false;
		seq.remainder_frame_length = 0.0;
		if (seq.should_finish_exactly_on_end_time) {
			seq.includes_remainder_frame = true;
			seq.remainder_frame_length = time_remainder;
		}
		
		// With all that taken care of, we can calculate the correct duration.
		seq.duration_in_frames = first_frame + available_frame_steps + 
				(seq.includes_remainder_frame? 1 : 0);
		seq.duration_in_ma = abs_time_increment * available_frame_steps +
				seq.remainder_frame_length;

		// With the duration calculated, knowing that there is possibly a
		// remainder frame of some sort, we can figure out what time the last
		// frame @em really lies on.
		seq.actual_start_time = seq.desired_start_time;
		if (seq.raw_time_increment > 0) {
			seq.actual_end_time = seq.desired_start_time + seq.duration_in_ma;
		} else {
			seq.actual_end_time = seq.desired_start_time - seq.duration_in_ma;
		}
	}
	
	return seq;
}


double
GPlatesUtils::AnimationSequence::raw_time_increment(
		const double &start_time,
		const double &end_time,
		const double &abs_time_increment)
{
	// Just because we ask them to supply an absolute increment,
	// doesn't mean we're going to get one.
	double time_increment = std::fabs(abs_time_increment);
	
	if (start_time < end_time) {
		return time_increment;
	} else {
		return -time_increment;
	}
}


double
GPlatesUtils::AnimationSequence::calculate_time_for_frame(
		const GPlatesUtils::AnimationSequence::SequenceInfo &seq,
		const GPlatesUtils::AnimationSequence::size_type &frame_index)
{
	// Frames up until the last frame are easy; the final frame is a special case.
	// Happily, that's already worked out for us.
	size_type last_frame_index = seq.duration_in_frames - 1;
	if (frame_index < last_frame_index) {
		return seq.actual_start_time + seq.raw_time_increment * frame_index;
	} else {
		return seq.actual_end_time;
	}
}

