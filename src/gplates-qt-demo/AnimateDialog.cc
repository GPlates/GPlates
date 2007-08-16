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

		// First, see whether there's "space" (in the total time interval) for more than a
		// single frame (which is already being displayed).
		recalculate_increment();
		double total_time_delta = widget_end_time->value() - widget_start_time->value();
		if (std::fabs(d_time_increment) > std::fabs(total_time_delta)) {
			// There's no space for more than the single frame which is already being
			// displayed.
			return;
		}

		// Otherwise, let's see if we've already reached the end of the animation (or
		// rather, whether we're as close to the end as we can get with this increment).
		double remaining_time = widget_end_time->value() - widget_current_time->value();
		if (std::fabs(d_time_increment) > std::fabs(remaining_time)) {
			// Yes, we've reached the end.  Let's rewind to the start.
			widget_current_time->setValue(widget_start_time->value());
		}

		const double num_frames_per_sec = 5.0;
		double frame_duration_millisecs = (1000.0 / num_frames_per_sec);
		d_timer.start(static_cast<int>(frame_duration_millisecs));

		button_Start->setText(QObject::tr("Stop"));
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
	double remaining_time = widget_end_time->value() - widget_current_time->value();
	if (std::fabs(d_time_increment) > std::fabs(remaining_time)) {
		// There's no space for more than the single frame which is already being
		// displayed.
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
