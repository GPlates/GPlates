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

#include "AnimationController.h"

#include "qt-widgets/ViewportWindow.h"		// for ViewState.
#include "maths/Real.h"
#include "utils/FloatingPointComparisons.h"
#include "utils/AnimationSequenceUtils.h"


GPlatesGui::AnimationController::AnimationController(
		GPlatesQtWidgets::ViewportWindow &view_state):
	d_view_state_ptr(&view_state),
	d_start_time(140.0),
	d_end_time(0.0),
	d_time_increment(-1.0),
	d_frames_per_second(5.0),
	d_finish_exactly_on_end_time(true),
	d_loop(false),
	d_adjust_bounds_to_contain_current_time(true)
{
	QObject::connect(&d_timer, SIGNAL(timeout()),
			this, SLOT(react_animation_playback_step()));
	QObject::connect(d_view_state_ptr, SIGNAL(reconstruction_time_changed(double)),
			this, SLOT(react_view_time_changed()));
}


const double &
GPlatesGui::AnimationController::view_time() const
{
	return d_view_state_ptr->reconstruction_time();
}


const double &
GPlatesGui::AnimationController::start_time() const
{
	return d_start_time;
}


const double &
GPlatesGui::AnimationController::end_time() const
{
	return d_end_time;
}


double
GPlatesGui::AnimationController::time_increment() const
{
	return std::fabs(d_time_increment);
}


double
GPlatesGui::AnimationController::raw_time_increment() const
{
	return d_time_increment;
}


bool
GPlatesGui::AnimationController::is_playing() const
{
	return d_timer.isActive();
}


const double &
GPlatesGui::AnimationController::frames_per_second() const
{
	return d_frames_per_second;
}


GPlatesGui::AnimationController::frame_index_type
GPlatesGui::AnimationController::duration_in_frames() const
{
	using namespace GPlatesUtils::AnimationSequence;
	SequenceInfo seq = calculate_sequence(start_time(), end_time(), time_increment(),
			should_finish_exactly_on_end_time());
	
	return seq.duration_in_frames;
#if 0
	using namespace GPlatesUtils::FloatingPointComparisons;

	// We always play the very first frame (@a start_time);
	static const frame_index_type first_frame = 1;
	double abs_time_increment = time_increment();
	
	// Find out how many steps we could go through the given time range.
	double available_range = std::fabs(end_time() - start_time());
	double available_steps = std::floor(available_range / abs_time_increment);

	// Okay, so if we were to step through that, how much non-animated 'slack space'
	// would be left at the end?
	double steppable_range = abs_time_increment * available_steps;
	double time_remainder = available_range - steppable_range;

	// Here's the tricky part, thanks to our friend the double.
	// If @a time_remainder is close to 0, that means that our @a available_range
	// (supplied by the user) probably neatly divides by the desired increment.
	//
	// On the other hand, if @a time_remainder is nowhere near 0, the user
	// is requesting a range which does not actually have an integer multiple of the
	// increment in there, and we may have to add an artificial extra frame on
	// the end (according to @a should_finish_exactly_on_end_time()).
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
	if (geo_times_are_approx_equal(time_remainder, 0.0)) {
		// Okay, requested range divides approximately by an integer multiple,
		// but we need to correct the @a available_steps calculation depending on
		// whether we were slightly over or slightly under.
		frame_index_type available_frame_steps = static_cast<frame_index_type>(available_steps);
		if (time_remainder > 0) {
			// Tiny extra leftover space at end, add one extra frame to @available_steps.
			available_frame_steps++;
		} else {
			// @a available_steps calculation overshot by a tiny amount, no need
			// for adjustment.
		}
		// Note that in this case, the value of @a should_finish_exactly_on_end_time()
		// is irrelevant.
		frame_index_type frames = first_frame + available_frame_steps;
		return frames;
	} else {
		// @a time_remainder is nowhere near 0, requested range does not divide
		// neatly by increment.
		// We don't need to worry about floating point error being accumulated,
		// but we do need to account for that last frame - if the user wants it
		// to be played.
		frame_index_type available_frame_steps = static_cast<frame_index_type>(available_steps);
		frame_index_type remainder_frame = 0;
		if (should_finish_exactly_on_end_time()) {
			remainder_frame = 1;
		}
		
		frame_index_type frames = first_frame + available_frame_steps + remainder_frame;
		return frames;
	}
#endif
}


double
GPlatesGui::AnimationController::duration_in_ma() const
{
	using namespace GPlatesUtils::AnimationSequence;
	SequenceInfo seq = calculate_sequence(start_time(), end_time(), time_increment(),
			should_finish_exactly_on_end_time());
	
	return seq.duration_in_ma;
}


double
GPlatesGui::AnimationController::starting_frame_time() const
{
	return start_time();
}


double
GPlatesGui::AnimationController::ending_frame_time() const
{
	using namespace GPlatesUtils::AnimationSequence;
	SequenceInfo seq = calculate_sequence(start_time(), end_time(), time_increment(),
			should_finish_exactly_on_end_time());
	
	return seq.actual_end_time;
}


double
GPlatesGui::AnimationController::calculate_time_for_frame(
		GPlatesGui::AnimationController::frame_index_type frame) const
{
	GPlatesUtils::AnimationSequence::SequenceInfo seq =
			GPlatesUtils::AnimationSequence::calculate_sequence(
					start_time(), end_time(), time_increment(),
					should_finish_exactly_on_end_time());
	
	return GPlatesUtils::AnimationSequence::calculate_time_for_frame(
			seq, frame);
}


bool
GPlatesGui::AnimationController::should_finish_exactly_on_end_time() const
{
	return d_finish_exactly_on_end_time;
}

bool
GPlatesGui::AnimationController::should_loop() const
{
	return d_loop;
}


bool
GPlatesGui::AnimationController::should_adjust_bounds_to_contain_current_time() const
{
	return d_adjust_bounds_to_contain_current_time;
}



bool
GPlatesGui::AnimationController::is_valid_reconstruction_time(
		const double &time)
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	// Firstly, ensure that the time is not less than the minimum reconstruction time.
	if (time < AnimationController::min_reconstruction_time() &&
			! geo_times_are_approx_equal(time,
					AnimationController::min_reconstruction_time())) {
		return false;
	}

	// Secondly, ensure that the time is not greater than the maximum reconstruction time.
	if (time > AnimationController::max_reconstruction_time() &&
			! geo_times_are_approx_equal(time,
					AnimationController::min_reconstruction_time())) {
		return false;
	}

	// Otherwise, it's a valid time.
	return true;
}



void
GPlatesGui::AnimationController::play()
{
	if (is_playing()) {
		// The animation is already playing.
		return;
	}
	
	using namespace GPlatesUtils::FloatingPointComparisons;

	recalculate_increment();
	double abs_time_increment = std::fabs(d_time_increment);
	double abs_total_time_delta = std::fabs(d_end_time - d_start_time);

	// Firstly, let's handle the special case in which the time-increment is almost
	// exactly the same as the total time delta. The time-increment may even be a tiny
	// amount larger than the total time delta -- which is presumably not what the user
	// wanted (since the difference is smaller than any difference the user could
	// specify), and is the presumably the result of the floating-point representation.
	// In this case, we should allow one frame of animation after this current frame.
	if (geo_times_are_approx_equal(abs_time_increment - abs_total_time_delta, 0.0)) {

		double current_time = view_time();
		if (geo_times_are_approx_equal(d_start_time, current_time) ||
				geo_times_are_approx_equal(d_end_time, current_time)) {

			set_view_time(d_start_time);
			start_animation_timer();
			return;
		}
	}

	// That special case aside, see whether there's space (in the total time interval)
	// for more than a single frame (which is already being displayed).
	if (abs_time_increment > abs_total_time_delta) {
		// There's no space for more than the single frame which is already being
		// displayed.  So, there's nothing to animate.
		return;
	}

	// Otherwise, there's space for more than one frame between the start-time and
	// end-time, so we'll play an animation.

	// As a special case, let's see if we've already reached the end of the animation
	// (or rather, whether we're as close to the end of the animation as we can get
	// with this time-increment).  If we have, we should automatically rewind the time
	// to the start.
	double abs_remaining_time = std::fabs(d_end_time - view_time());
	if (abs_time_increment > abs_remaining_time) {
		// Yes, we've reached the end.  Let's rewind to the start.
		seek_beginning();
	}

	start_animation_timer();
}


void
GPlatesGui::AnimationController::pause()
{
	stop_animation_timer();
}


void
GPlatesGui::AnimationController::set_play_or_pause(
		bool lets_play)
{
	if (lets_play) {
		play();
	} else {
		pause();
	}
}


void
GPlatesGui::AnimationController::step_forward()
{
	// Step forward through the animation, towards the 'end' time.
	// Remember that the 'start' and 'end' times may be reversed,
	// and do not necessarily correspond to 'past' and 'future'.
	double new_time_value = view_time() + d_time_increment;

	// If the user attempts to use the step buttons to move past 0.0 (into the future!),
	// we should clamp the view time to 0.0.
	if (new_time_value < 0.0) {
		new_time_value = 0.0;
	}
	
	set_view_time(new_time_value);
}


void
GPlatesGui::AnimationController::step_back()
{
	// Step back through the animation, towards the 'start' time.
	// Remember that the 'start' and 'end' times may be reversed,
	// and do not necessarily correspond to 'past' and 'future'.
	double new_time_value = view_time() - d_time_increment;

	// If the user attempts to use the step buttons to move past 0.0 (into the future!),
	// we should clamp the view time to 0.0.
	if (new_time_value < 0.0) {
		new_time_value = 0.0;
	}
	
	set_view_time(new_time_value);
}


void
GPlatesGui::AnimationController::seek_beginning()
{
	set_view_time(d_start_time);
}


void
GPlatesGui::AnimationController::seek_end()
{
	set_view_time(d_end_time);
}


void
GPlatesGui::AnimationController::set_view_time(
		const double new_time)
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	// Ensure the new reconstruction time is valid.
	// FIXME: Move this function somewhere more appropriate and call the new version.
	if ( ! is_valid_reconstruction_time(new_time)) {
		return;
	}

	// Only modify the reconstruction time and emit signals if the time has
	// actually been changed.
	if ( ! geo_times_are_approx_equal(view_time(), new_time)) {
		d_view_state_ptr->reconstruct_to_time(new_time);
		
		emit view_time_changed(new_time);
	}
}


void
GPlatesGui::AnimationController::set_view_frame(
		GPlatesGui::AnimationController::frame_index_type frame)
{
	// Cap @a frame to bounds.
	frame_index_type duration = duration_in_frames();
	if (frame >= duration) {
		frame = duration - 1;
	}

	set_view_time(calculate_time_for_frame(frame));
}


void
GPlatesGui::AnimationController::set_start_time(
		const double new_time)
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	if ( ! geo_times_are_approx_equal(d_start_time, new_time)) {
		d_start_time = new_time;
		
		emit start_time_changed(new_time);
		recalculate_increment();
	}
}


void
GPlatesGui::AnimationController::set_end_time(
		const double new_time)
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	if ( ! geo_times_are_approx_equal(d_end_time, new_time)) {
		d_end_time = new_time;
		
		emit end_time_changed(new_time);
		recalculate_increment();
	}
}


void
GPlatesGui::AnimationController::set_time_increment(
		const double new_abs_increment)
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	// Translate the user-friendly absolute value new_increment into the
	// appropriate +/- increment to get from d_start_time to d_end_time.
	double new_increment;
	if (d_end_time > d_start_time) {
		new_increment = new_abs_increment;
	} else {
		new_increment = -new_abs_increment;
	}
	
	if ( ! geo_times_are_approx_equal(d_time_increment, new_increment)) {
		d_time_increment = new_increment;
		
		// Note that the signal emits the abs version for consistency.
		emit time_increment_changed(new_abs_increment);
	}
}


void
GPlatesGui::AnimationController::set_frames_per_second(
		const double fps)
{
	// Not dealing with geo-times here, but still want to compare two doubles.
	if (GPlatesMaths::Real(d_frames_per_second) != fps) {
		d_frames_per_second = fps;
		
		emit frames_per_second_changed(fps);
	}
}

void
GPlatesGui::AnimationController::set_should_finish_exactly_on_end_time(
		bool finish_exactly)
{
	if (d_finish_exactly_on_end_time != finish_exactly) {
		d_finish_exactly_on_end_time = finish_exactly;
		emit finish_exactly_on_end_time_changed(finish_exactly);
	}
}

void
GPlatesGui::AnimationController::set_should_loop(
		bool loop)
{
	if (d_loop != loop) {
		d_loop = loop;
		emit should_loop_changed(loop);
	}
}


void
GPlatesGui::AnimationController::set_should_adjust_bounds_to_contain_current_time(
		bool adjust_bounds)
{
	if (d_adjust_bounds_to_contain_current_time != adjust_bounds) {
		d_adjust_bounds_to_contain_current_time = adjust_bounds;
		emit should_adjust_bounds_to_contain_current_time_changed(adjust_bounds);
	}
}


void
GPlatesGui::AnimationController::react_animation_playback_step()
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	double abs_time_increment = std::fabs(d_time_increment);
	double abs_remaining_time = std::fabs(d_end_time - view_time());

	// Firstly, let's handle the special case in which the time-increment is almost exactly the
	// same as the total time delta. The time-increment may even be a tiny amount larger than
	// the remaining time -- which may have been caused by accumulated floating-point error. 
	// In this case, we should allow one more frame (after the current frame),  but rather than
	// adding the increment to the current-time, set the current-time directly to the end-time
	// (or else, the current-time would go past the end-time).
	if (geo_times_are_approx_equal(abs_time_increment - abs_remaining_time, 0.0)) {
		set_view_time(d_end_time);
		return;
	}

	// Now let's handle the more general case in which the time increment is larger than the
	// remaining time.
	if (abs_time_increment > abs_remaining_time) {
		// Another frame would take us past the end-time. Decide what to do based on the
		// "Finish animation exactly at end time" and "Loop" options, as set from AnimateDialog.
		if (d_finish_exactly_on_end_time) {
			// We should finish at the exact end point.
			set_view_time(d_end_time);
		} else {
			// Else, the animation should stop where the last increment left us,
			// even if this does not exactly equal the specified end time.
		}

		if (d_loop) {
			// Return to the start of the animation and keep animating.
			set_view_time(d_start_time);
			return;
		} else {
			// We are not looping and should stop the animation here.
			stop_animation_timer();
			return;
		}
	}

	set_view_time(view_time() + d_time_increment);
}


void
GPlatesGui::AnimationController::react_view_time_changed()
{
	if (d_adjust_bounds_to_contain_current_time) {
		ensure_bounds_contain_current_time();
	}
}


void
GPlatesGui::AnimationController::swap_start_and_end_times()
{
	// FIXME: This needs to be smarter now it's in AnimationController.
	//        These are compile fixes only.

	// We first set both endpoints to equal the current time, in a clever hack
	// to preserve the current time (a simple swap would result in both start
	// and end times temporarily equal to the min or max time, firing an event
	// which would clamp the current time at one of those endpoints)
	double orig_start_time = start_time();
	double orig_end_time = end_time();
	
	set_start_time(view_time());
	set_end_time(view_time());

	set_start_time(orig_end_time);
	set_end_time(orig_start_time);
}


void
GPlatesGui::AnimationController::start_animation_timer()
{
	double frame_duration_millisecs = (1000.0 / d_frames_per_second);
	d_timer.start(static_cast<int>(frame_duration_millisecs));

	emit animation_started();
	emit animation_state_changed(true);
}


void
GPlatesGui::AnimationController::stop_animation_timer()
{
	d_timer.stop();

	emit animation_paused();
	emit animation_state_changed(false);
}


void
GPlatesGui::AnimationController::ensure_current_time_lies_within_bounds()
{
	double current_time = view_time();

	if (current_time > d_start_time && current_time > d_end_time) {
		// The current-time is above the range of the boundary times.  It will need to be
		// adjusted back down to whichever is the upper bound.
		if (d_start_time > d_end_time) {
			set_view_time(d_start_time);
		} else {
			set_view_time(d_end_time);
		}
	} else if (current_time < d_start_time && current_time < d_end_time) {
		// The current-time is below the range of the boundary times.  It will need to be
		// adjusted back up to whichever is the lower bound.
		if (d_start_time < d_end_time) {
			set_view_time(d_start_time);
		} else {
			set_view_time(d_end_time);
		}
	}
}


void
GPlatesGui::AnimationController::ensure_bounds_contain_current_time()
{
	double current_time = view_time();
	
	if (current_time > d_start_time && current_time > d_end_time) {
		// The current-time is above the range of the boundary times.  Whichever boundary
		// time is the upper bound will need to be adjusted.
		if (d_start_time > d_end_time) {
			set_start_time(current_time);
		} else {
			set_end_time(current_time);
		}
	} else if (current_time < d_start_time && current_time < d_end_time) {
		// The current-time is below the range of the boundary times.  Whichever boundary
		// time is the lower bound will need to be adjusted.
		if (d_start_time < d_end_time) {
			set_start_time(current_time);
		} else {
			set_end_time(current_time);
		}
	}
}



void
GPlatesGui::AnimationController::recalculate_increment()
{
	if (d_start_time < d_end_time) {
		d_time_increment = time_increment();
	} else {
		d_time_increment = -time_increment();
	}
	// This function will only ever swap the sign of the increment,
	// not the magnitude, and therefore does not need to emit
	// a signal back to the GUI.
}

