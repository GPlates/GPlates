/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include "AnimateDialog.h"
#include "Document.h"
#include "util/FloatingPointComparisons.h"


GPlatesGui::AnimateDialog::AnimateDialog(
		Document &viewport,
		QWidget *parent):
	QDialog(parent),
	d_viewport_ptr(&viewport),
	d_timer(this)
{
	setupUi(this);

	QObject::connect(button_Use_Viewport_Time_start_time, SIGNAL(clicked()),
			this, SLOT(set_start_time_value_to_viewport_time()));
	QObject::connect(button_Use_Viewport_Time_end_time, SIGNAL(clicked()),
			this, SLOT(set_end_time_value_to_viewport_time()));
	QObject::connect(button_Use_Viewport_Time_current_time, SIGNAL(clicked()),
			this, SLOT(set_current_time_value_to_viewport_time()));

	QObject::connect(widget_start_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_start_time_changed(double)));
	QObject::connect(widget_end_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_end_time_changed(double)));
	QObject::connect(widget_current_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_current_time_changed(double)));

	QObject::connect(button_Start, SIGNAL(clicked()),
			this, SLOT(toggle_animation_playback_state()));

	QObject::connect(&d_timer, SIGNAL(timeout()),
			this, SLOT(react_animation_playback_step()));
}


const double &
GPlatesGui::AnimateDialog::viewport_time() const
{
	return d_viewport_ptr->reconstruction_time();
}


void
GPlatesGui::AnimateDialog::toggle_animation_playback_state()
{
	if (d_timer.isActive()) {
		stop_animation_playback();
	} else {
		// Otherwise, the animation is not yet playing.

		using namespace GPlatesUtil::FloatingPointComparisons;

		double start_time = widget_start_time->value();
		double end_time = widget_end_time->value();

		recalculate_increment();
		double abs_time_increment = std::fabs(d_time_increment);
		double abs_total_time_delta = std::fabs(end_time - start_time);

		// Firstly, let's handle the special case in which the time-increment is almost
		// exactly the same as the total time delta. The time-increment may even be a tiny
		// amount larger than the total time delta -- which is presumably not what the user
		// wanted (since the difference is smaller than any difference the user could
		// specify), and is the presumably the result of the floating-point representation.
		// In this case, we should allow one frame of animation after this current frame.
		if (geo_times_are_approx_equal(abs_time_increment - abs_total_time_delta, 0.0)) {

			double current_time = widget_current_time->value();
			if (geo_times_are_approx_equal(start_time, current_time) ||
					geo_times_are_approx_equal(end_time, current_time)) {

				widget_current_time->setValue(start_time);
				start_animation_playback();
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
		double abs_remaining_time = std::fabs(end_time - widget_current_time->value());
		if (abs_time_increment > abs_remaining_time) {

			// Yes, we've reached the end.  Let's rewind to the start.
			widget_current_time->setValue(start_time);
		}

		start_animation_playback();
	}
}


void
GPlatesGui::AnimateDialog::react_start_time_changed(
		double new_val)
{
	ensure_current_time_lies_within_bounds();
	recalculate_slider();
}


void
GPlatesGui::AnimateDialog::react_end_time_changed(
		double new_val)
{
	ensure_current_time_lies_within_bounds();
	recalculate_slider();
}


void
GPlatesGui::AnimateDialog::react_current_time_changed(
		double new_val)
{
	ensure_bounds_contain_current_time();
	recalculate_slider();

	emit current_time_changed(widget_current_time->value());
}


void
GPlatesGui::AnimateDialog::react_animation_playback_step()
{
	using namespace GPlatesUtil::FloatingPointComparisons;

	double abs_time_increment = std::fabs(d_time_increment);
	double abs_remaining_time = std::fabs(widget_end_time->value() - widget_current_time->value());

	// Firstly, let's handle the special case in which the time-increment is almost exactly the
	// same as the total time delta. The time-increment may even be a tiny amount larger than
	// the remaining time -- which may have been caused by accumulated floating-point error. 
	// In this case, we should allow one more frame (after the current frame),  but rather than
	// adding the increment to the current-time, set the current-time directly to the end-time
	// (or else, the current-time would go past the end-time).
	if (geo_times_are_approx_equal(abs_time_increment - abs_remaining_time, 0.0)) {

		widget_current_time->setValue(widget_end_time->value());
		return;
	}

	// Now let's handle the more general case in which the time increment is larger than the
	// remaining time.
	if (abs_time_increment > abs_remaining_time) {
		// Another frame would take us past the end-time, so stop the animation here.

		stop_animation_playback();
		return;
	}

	widget_current_time->setValue(widget_current_time->value() + d_time_increment);
}


void
GPlatesGui::AnimateDialog::ensure_current_time_lies_within_bounds()
{
	double start_time = widget_start_time->value();
	double end_time = widget_end_time->value();
	double current_time = widget_current_time->value();

	if (current_time > start_time && current_time > end_time) {
		// The current-time is above the range of the boundary times.  It will need to be
		// adjusted back down to whichever is the upper bound.
		if (start_time > end_time) {
			widget_current_time->setValue(start_time);
		} else {
			widget_current_time->setValue(end_time);
		}
	} else if (current_time < start_time && current_time < end_time) {
		// The current-time is below the range of the boundary times.  It will need to be
		// adjusted back up to whichever is the lower bound.
		if (start_time < end_time) {
			widget_current_time->setValue(start_time);
		} else {
			widget_current_time->setValue(end_time);
		}
	}
}


void
GPlatesGui::AnimateDialog::ensure_bounds_contain_current_time()
{
	double start_time = widget_start_time->value();
	double end_time = widget_end_time->value();
	double current_time = widget_current_time->value();

	if (current_time > start_time && current_time > end_time) {
		// The current-time is above the range of the boundary times.  Whichever boundary
		// time is the upper bound will need to be adjusted.
		if (start_time > end_time) {
			widget_start_time->setValue(current_time);
		} else {
			widget_end_time->setValue(current_time);
		}
	} else if (current_time < start_time && current_time < end_time) {
		// The current-time is below the range of the boundary times.  Whichever boundary
		// time is the lower bound will need to be adjusted.
		if (start_time < end_time) {
			widget_start_time->setValue(current_time);
		} else {
			widget_end_time->setValue(current_time);
		}
	}
}


void
GPlatesGui::AnimateDialog::recalculate_slider()
{
	// The slider functionality is not yet implemented.
}


void
GPlatesGui::AnimateDialog::start_animation_playback()
{
	const double num_frames_per_sec = 5.0;
	double frame_duration_millisecs = (1000.0 / num_frames_per_sec);
	d_timer.start(static_cast<int>(frame_duration_millisecs));

	button_Start->setText(QObject::tr("Stop"));
}


void
GPlatesGui::AnimateDialog::stop_animation_playback()
{
	d_timer.stop();
	button_Start->setText(QObject::tr("Start"));
}


void
GPlatesGui::AnimateDialog::recalculate_increment()
{
	if (widget_start_time->value() < widget_end_time->value()) {
		d_time_increment = widget_time_increment->value();
	} else {
		d_time_increment = -widget_time_increment->value();
	}
}
