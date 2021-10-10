/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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
#include "ViewportWindow.h"
#include "utils/FloatingPointComparisons.h"


GPlatesQtWidgets::AnimateDialog::AnimateDialog(
		ViewportWindow &viewport,
		QWidget *parent_):
	QDialog(parent_),
	d_viewport_ptr(&viewport),
	d_timer(this)
{
	setupUi(this);

	QObject::connect(button_Use_View_Time_start_time, SIGNAL(clicked()),
			this, SLOT(set_start_time_value_to_view_time()));
	QObject::connect(button_Use_View_Time_end_time, SIGNAL(clicked()),
			this, SLOT(set_end_time_value_to_view_time()));
	QObject::connect(button_Use_View_Time_current_time, SIGNAL(clicked()),
			this, SLOT(set_current_time_value_to_view_time()));

	QObject::connect(widget_start_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_start_time_changed(double)));
	QObject::connect(widget_end_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_end_time_changed(double)));
	QObject::connect(widget_current_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_current_time_changed(double)));

	QObject::connect(button_Reverse_the_Animation, SIGNAL(clicked()),
			this, SLOT(swap_start_and_end_times()));

	QObject::connect(slider_current_time, SIGNAL(valueChanged(int)),
			this, SLOT(set_current_time_from_slider(int)));
	QObject::connect(button_Start, SIGNAL(clicked()),
			this, SLOT(toggle_animation_playback_state()));
	QObject::connect(button_Rewind, SIGNAL(clicked()),
			this, SLOT(rewind()));

	QObject::connect(&d_timer, SIGNAL(timeout()),
			this, SLOT(react_animation_playback_step()));
}


const double &
GPlatesQtWidgets::AnimateDialog::view_time() const
{
	return d_viewport_ptr->reconstruction_time();
}


void
GPlatesQtWidgets::AnimateDialog::toggle_animation_playback_state()
{
	if (d_timer.isActive()) {
		stop_animation_playback();
	} else {
		// Otherwise, the animation is not yet playing.

		using namespace GPlatesUtils::FloatingPointComparisons;

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
GPlatesQtWidgets::AnimateDialog::rewind()
{
	widget_current_time->setValue(widget_start_time->value());
}


void
GPlatesQtWidgets::AnimateDialog::react_start_time_changed(
		double new_val)
{
	ensure_current_time_lies_within_bounds();
	recalculate_slider();
}


void
GPlatesQtWidgets::AnimateDialog::react_end_time_changed(
		double new_val)
{
	ensure_current_time_lies_within_bounds();
	recalculate_slider();
}


void
GPlatesQtWidgets::AnimateDialog::react_current_time_changed(
		double new_val)
{
	ensure_bounds_contain_current_time();
	recalculate_slider();

	emit current_time_changed(widget_current_time->value());
}


void
GPlatesQtWidgets::AnimateDialog::react_animation_playback_step()
{
	using namespace GPlatesUtils::FloatingPointComparisons;

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
		// Another frame would take us past the end-time. Decide what to do based on the
		// "Finish animation exactly at end time" and "Loop" checkboxes.
		if (checkbox_Finish_animation_on_end_time->checkState() == Qt::Checked) {
			// We should finish at the exact end point.
			widget_current_time->setValue(widget_end_time->value());
		} else {
			// Else, the animation should stop where the last increment left us,
			// even if this does not exactly equal the specified end time.
		}

		if (checkbox_Loop->checkState() == Qt::Checked) {
			// Return to the start of the animation and keep animating.
			widget_current_time->setValue(widget_start_time->value());
			return;
		} else {
			// We are not looping and should stop the animation here.
			stop_animation_playback();
			return;
		}
	}

	widget_current_time->setValue(widget_current_time->value() + d_time_increment);
}


void
GPlatesQtWidgets::AnimateDialog::swap_start_and_end_times()
{
	double current_time = widget_current_time->value();
	double start_time = widget_start_time->value();
	double end_time = widget_end_time->value();

	// We first set both endpoints to equal the current time, in a clever hack
	// to preserve the current time (a simple swap would result in both start
	// and end times temporarily equal to the min or max time, firing an event
	// which would clamp the current time at one of those endpoints)
	widget_start_time->setValue(current_time);
	widget_end_time->setValue(current_time);

	widget_start_time->setValue(end_time);
	widget_end_time->setValue(start_time);
}


void
GPlatesQtWidgets::AnimateDialog::set_current_time_from_slider(
		int slider_pos)
{
	widget_current_time->setValue(slider_units_to_ma(slider_pos));
}


void
GPlatesQtWidgets::AnimateDialog::ensure_current_time_lies_within_bounds()
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
GPlatesQtWidgets::AnimateDialog::ensure_bounds_contain_current_time()
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


int
GPlatesQtWidgets::AnimateDialog::ma_to_slider_units(
		const double &ma)
{
	// QSlider uses integers for it's min, max, and current values.
	// We convert from reconstruction times to 'slider units' by
	// multiplying by 100 and negating the value if necessary
	// (so that the slider always moves left-to-right)
	if (widget_start_time->value() > widget_end_time->value()) {
		// Left - Right on the slider corresponds to Past - Future (Large Ma. - Small Ma.)
		return 0 - static_cast<int>(ma * 100 + 0.5);
	} else {
		// Left - Right on the slider corresponds to Future - Past (Small Ma. - Large Ma.)
		return static_cast<int>(ma * 100 + 0.5);
	}
}


double
GPlatesQtWidgets::AnimateDialog::slider_units_to_ma(
		const int &slider_pos)
{
	// QSlider uses integers for it's min, max, and current values.
	// We convert from reconstruction times to 'slider units' by
	// multiplying by 100 and negating the value if necessary
	// (so that the slider always moves left-to-right)
	if (widget_start_time->value() > widget_end_time->value()) {
		// Left - Right on the slider corresponds to Past - Future (Large Ma. - Small Ma.)
		return (0 - slider_pos) / 100.0;
	} else {
		// Left - Right on the slider corresponds to Future - Past (Small Ma. - Large Ma.)
		return slider_pos / 100.0;
	}
}


void
GPlatesQtWidgets::AnimateDialog::recalculate_slider()
{
	double start_time = widget_start_time->value();
	double end_time = widget_end_time->value();
	double current_time = widget_current_time->value();
	
	slider_current_time->setMinimum(ma_to_slider_units(start_time));
	slider_current_time->setMaximum(ma_to_slider_units(end_time));
	slider_current_time->setValue(ma_to_slider_units(current_time));
}


void
GPlatesQtWidgets::AnimateDialog::start_animation_playback()
{
	double num_frames_per_sec = widget_Frames_per_second->value();
	double frame_duration_millisecs = (1000.0 / num_frames_per_sec);
	d_timer.start(static_cast<int>(frame_duration_millisecs));

	button_Start->setText(QObject::tr("Stop"));
}


void
GPlatesQtWidgets::AnimateDialog::stop_animation_playback()
{
	d_timer.stop();
	button_Start->setText(QObject::tr("Start"));
}


void
GPlatesQtWidgets::AnimateDialog::recalculate_increment()
{
	if (widget_start_time->value() < widget_end_time->value()) {
		d_time_increment = widget_time_increment->value();
	} else {
		d_time_increment = -widget_time_increment->value();
	}
}


