/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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
 
#include "AnimateDialog.h"
#include "gui/AnimationController.h"
#include "utils/FloatingPointComparisons.h"


GPlatesQtWidgets::AnimateDialog::AnimateDialog(
		GPlatesGui::AnimationController &animation_controller,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_animation_controller_ptr(&animation_controller)
{
	setupUi(this);

	// Handle my buttons and spinboxes:
	QObject::connect(button_Use_View_Time_start_time, SIGNAL(clicked()),
			this, SLOT(set_start_time_value_to_view_time()));
	QObject::connect(button_Use_View_Time_end_time, SIGNAL(clicked()),
			this, SLOT(set_end_time_value_to_view_time()));

	QObject::connect(widget_start_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_start_time_spinbox_changed(double)));
	QObject::connect(widget_end_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_end_time_spinbox_changed(double)));
	QObject::connect(widget_time_increment, SIGNAL(valueChanged(double)),
			this, SLOT(react_time_increment_spinbox_changed(double)));
	QObject::connect(widget_current_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_current_time_spinbox_changed(double)));

	QObject::connect(button_Reverse_the_Animation, SIGNAL(clicked()),
			d_animation_controller_ptr, SLOT(swap_start_and_end_times()));

	QObject::connect(slider_current_time, SIGNAL(valueChanged(int)),
			this, SLOT(set_current_time_from_slider(int)));
	QObject::connect(button_Start, SIGNAL(clicked()),
			this, SLOT(toggle_animation_playback_state()));
	QObject::connect(button_Rewind, SIGNAL(clicked()),
			this, SLOT(rewind()));

	QObject::connect(widget_Frames_per_second, SIGNAL(valueChanged(double)),
			d_animation_controller_ptr, SLOT(set_frames_per_second(double)));
	QObject::connect(checkbox_Finish_animation_on_end_time, SIGNAL(clicked(bool)),
			d_animation_controller_ptr, SLOT(set_should_finish_exactly_on_end_time(bool)));
	QObject::connect(checkbox_Loop, SIGNAL(clicked(bool)),
			d_animation_controller_ptr, SLOT(set_should_loop(bool)));

	// Initialise widgets to state matching AnimationController.
	widget_start_time->setValue(d_animation_controller_ptr->start_time());
	widget_end_time->setValue(d_animation_controller_ptr->end_time());
	widget_time_increment->setValue(d_animation_controller_ptr->time_increment());
	widget_current_time->setValue(d_animation_controller_ptr->view_time());

	recalculate_slider();
	set_start_button_state(d_animation_controller_ptr->is_playing());

	widget_Frames_per_second->setValue(d_animation_controller_ptr->frames_per_second());
	handle_options_changed();

	// Set up signal/slot connections to respond to AnimationController events.
	QObject::connect(d_animation_controller_ptr, SIGNAL(view_time_changed(double)),
			this, SLOT(handle_current_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(start_time_changed(double)),
			this, SLOT(handle_start_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(end_time_changed(double)),
			this, SLOT(handle_end_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(time_increment_changed(double)),
			this, SLOT(handle_time_increment_changed(double)));
	
	QObject::connect(d_animation_controller_ptr, SIGNAL(frames_per_second_changed(double)),
			widget_Frames_per_second, SLOT(setValue(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(finish_exactly_on_end_time_changed(bool)),
			this, SLOT(handle_options_changed()));
	QObject::connect(d_animation_controller_ptr, SIGNAL(should_loop_changed(bool)),
			this, SLOT(handle_options_changed()));
	
	QObject::connect(d_animation_controller_ptr, SIGNAL(animation_started()),
			this, SLOT(handle_animation_started()));
	QObject::connect(d_animation_controller_ptr, SIGNAL(animation_paused()),
			this, SLOT(handle_animation_paused()));
}


const double &
GPlatesQtWidgets::AnimateDialog::view_time() const
{
	return d_animation_controller_ptr->view_time();
}


void
GPlatesQtWidgets::AnimateDialog::set_start_time_value_to_view_time()
{
	d_animation_controller_ptr->set_start_time(view_time());
}

void
GPlatesQtWidgets::AnimateDialog::set_end_time_value_to_view_time()
{
	d_animation_controller_ptr->set_end_time(view_time());
}


void
GPlatesQtWidgets::AnimateDialog::toggle_animation_playback_state()
{
	if (d_animation_controller_ptr->is_playing()) {
		d_animation_controller_ptr->pause();
	} else {
		d_animation_controller_ptr->play();
	}
}


void
GPlatesQtWidgets::AnimateDialog::rewind()
{
	d_animation_controller_ptr->seek_beginning();
}


void
GPlatesQtWidgets::AnimateDialog::react_start_time_spinbox_changed(
		double new_val)
{
	d_animation_controller_ptr->set_start_time(new_val);
}


void
GPlatesQtWidgets::AnimateDialog::react_end_time_spinbox_changed(
		double new_val)
{
	d_animation_controller_ptr->set_end_time(new_val);
}


void
GPlatesQtWidgets::AnimateDialog::react_time_increment_spinbox_changed(
		double new_val)
{
	d_animation_controller_ptr->set_time_increment(new_val);
}


void
GPlatesQtWidgets::AnimateDialog::react_current_time_spinbox_changed(
		double new_val)
{
	d_animation_controller_ptr->set_view_time(new_val);
}


void
GPlatesQtWidgets::AnimateDialog::handle_start_time_changed(
		double new_val)
{
	widget_start_time->setValue(new_val);
	recalculate_slider();
}


void
GPlatesQtWidgets::AnimateDialog::handle_end_time_changed(
		double new_val)
{
	widget_end_time->setValue(new_val);
	recalculate_slider();
}


void
GPlatesQtWidgets::AnimateDialog::handle_time_increment_changed(
		double new_val)
{
	widget_time_increment->setValue(new_val);
}


void
GPlatesQtWidgets::AnimateDialog::handle_current_time_changed(
		double new_val)
{
	widget_current_time->setValue(new_val);
	recalculate_slider();
}


void
GPlatesQtWidgets::AnimateDialog::handle_options_changed()
{
	checkbox_Finish_animation_on_end_time->setChecked(
			d_animation_controller_ptr->should_finish_exactly_on_end_time());
	checkbox_Loop->setChecked(d_animation_controller_ptr->should_loop());
}


void
GPlatesQtWidgets::AnimateDialog::handle_animation_started()
{
	set_start_button_state(true);
	if (isVisible() && checkbox_Close_dialog_when_animation_starts->isChecked()) {
		setVisible(false);
	}
}


void
GPlatesQtWidgets::AnimateDialog::handle_animation_paused()
{
	set_start_button_state(false);
}


void
GPlatesQtWidgets::AnimateDialog::set_current_time_from_slider(
		int slider_pos)
{
	widget_current_time->setValue(slider_units_to_ma(slider_pos));
}


void
GPlatesQtWidgets::AnimateDialog::set_start_button_state(
		bool animation_is_playing)
{
	static const QIcon icon_play(":/gnome_media_playback_start_22.png");
	static const QIcon icon_pause(":/gnome_media_playback_pause_22.png");
	static const QIcon icon_stop(":/gnome_media_playback_stop_22.png");
	
	if (animation_is_playing) {
		button_Start->setText(tr("&Pause"));
		button_Start->setIcon(icon_pause);
	} else {
		button_Start->setText(tr("&Play"));
		button_Start->setIcon(icon_play);
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


