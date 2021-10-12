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

#include "AnimateControlWidget.h"

#include <QDebug>
#include "gui/AnimationController.h"
#include "utils/FloatingPointComparisons.h"


QDockWidget *
GPlatesQtWidgets::AnimateControlWidget::create_as_qdockwidget(
		GPlatesGui::AnimationController &animation_controller)
{
	QDockWidget *dock = new QDockWidget(tr("Animation Controls"));
	dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	
	AnimateControlWidget *controls = new AnimateControlWidget(
			animation_controller, dock);
	dock->setWidget(controls);
	
	return dock;
}


GPlatesQtWidgets::AnimateControlWidget::AnimateControlWidget(
		GPlatesGui::AnimationController &animation_controller,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_animation_controller_ptr(&animation_controller)
{
	setupUi(this);
	use_combined_play_pause_button(true);
	show_step_buttons(true);
	
	// Set up signal/slot connections for our buttons to our private handler methods.
	QObject::connect(button_play, SIGNAL(clicked()),
			this, SLOT(handle_play_or_pause_clicked()));
	QObject::connect(button_pause, SIGNAL(clicked()),
			this, SLOT(handle_play_or_pause_clicked()));
	QObject::connect(button_play_or_pause, SIGNAL(clicked()),
			this, SLOT(handle_play_or_pause_clicked()));
	QObject::connect(button_seek_beginning, SIGNAL(clicked()),
			this, SLOT(handle_seek_beginning_clicked()));

	QObject::connect(button_step_backwards, SIGNAL(clicked()),
			d_animation_controller_ptr, SLOT(step_back()));
	QObject::connect(button_step_forwards, SIGNAL(clicked()),
			d_animation_controller_ptr, SLOT(step_forward()));

	QObject::connect(slider_current_time, SIGNAL(valueChanged(int)),
			this, SLOT(set_current_time_from_slider(int)));
	
	// Initialise widgets to state matching AnimationController.
	recalculate_slider();
	update_button_states();
	
	// Set up signal/slot connections to respond to AnimationController events.
	QObject::connect(d_animation_controller_ptr, SIGNAL(view_time_changed(double)),
			this, SLOT(handle_view_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(start_time_changed(double)),
			this, SLOT(handle_start_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(end_time_changed(double)),
			this, SLOT(handle_end_time_changed(double)));
	QObject::connect(d_animation_controller_ptr, SIGNAL(animation_started()),
			this, SLOT(handle_animation_started()));
	QObject::connect(d_animation_controller_ptr, SIGNAL(animation_paused()),
			this, SLOT(handle_animation_paused()));
}


void
GPlatesQtWidgets::AnimateControlWidget::show_step_buttons(
		bool show_)
{
	// But wait! We might want to try putting the fwd/rev buttons elsewhere.
	// So we'll hide these ones. Until it turns out they looked better here.
	button_step_backwards->setVisible(show_);
	button_step_forwards->setVisible(show_);
}


void
GPlatesQtWidgets::AnimateControlWidget::handle_play_or_pause_clicked()
{
	if (d_animation_controller_ptr->is_playing()) {
		// Animation playing. We want to pause.
		d_animation_controller_ptr->pause();
		
	} else {
		// Animation paused. We want to play.
		d_animation_controller_ptr->play();
	}
	update_button_states();
}


void
GPlatesQtWidgets::AnimateControlWidget::handle_seek_beginning_clicked()
{
	d_animation_controller_ptr->seek_beginning();
}

void
GPlatesQtWidgets::AnimateControlWidget::set_current_time_from_slider(
		int slider_pos)
{
	d_animation_controller_ptr->set_view_time(slider_units_to_ma(slider_pos));
}


void
GPlatesQtWidgets::AnimateControlWidget::handle_view_time_changed(
		double new_time)
{
	slider_current_time->setValue(ma_to_slider_units(new_time));
}


void
GPlatesQtWidgets::AnimateControlWidget::handle_start_time_changed(
		double new_time)
{
	recalculate_slider();
}

void
GPlatesQtWidgets::AnimateControlWidget::handle_end_time_changed(
		double new_time)
{
	recalculate_slider();
}


void
GPlatesQtWidgets::AnimateControlWidget::handle_animation_started()
{
	update_button_states();
}

void
GPlatesQtWidgets::AnimateControlWidget::handle_animation_paused()
{
	update_button_states();
}


int
GPlatesQtWidgets::AnimateControlWidget::ma_to_slider_units(
		const double &ma)
{
	// QSlider uses integers for it's min, max, and current values.
	// We convert from reconstruction times to 'slider units' by
	// multiplying by 100 and negating the value if necessary
	// (so that the slider always moves left-to-right)
	if (d_animation_controller_ptr->start_time() > d_animation_controller_ptr->end_time()) {
		// Left - Right on the slider corresponds to Past - Future (Large Ma. - Small Ma.)
		return 0 - static_cast<int>(ma * 100 + 0.5);
	} else {
		// Left - Right on the slider corresponds to Future - Past (Small Ma. - Large Ma.)
		return static_cast<int>(ma * 100 + 0.5);
	}
}


double
GPlatesQtWidgets::AnimateControlWidget::slider_units_to_ma(
		const int &slider_pos)
{
	// QSlider uses integers for it's min, max, and current values.
	// We convert from reconstruction times to 'slider units' by
	// multiplying by 100 and negating the value if necessary
	// (so that the slider always moves left-to-right)
	if (d_animation_controller_ptr->start_time() > d_animation_controller_ptr->end_time()) {
		// Left - Right on the slider corresponds to Past - Future (Large Ma. - Small Ma.)
		return (0 - slider_pos) / 100.0;
	} else {
		// Left - Right on the slider corresponds to Future - Past (Small Ma. - Large Ma.)
		return slider_pos / 100.0;
	}
}


void
GPlatesQtWidgets::AnimateControlWidget::recalculate_slider()
{
	double start_time = d_animation_controller_ptr->start_time();
	double end_time = d_animation_controller_ptr->end_time();
	double current_time = d_animation_controller_ptr->view_time();
	
	slider_current_time->setMinimum(ma_to_slider_units(start_time));
	slider_current_time->setMaximum(ma_to_slider_units(end_time));
	slider_current_time->setValue(ma_to_slider_units(current_time));
}


void
GPlatesQtWidgets::AnimateControlWidget::update_button_states()
{
	// Play and Pause buttons should get depressed according to play state.
	button_play->setChecked(d_animation_controller_ptr->is_playing());
	button_pause->setChecked( ! d_animation_controller_ptr->is_playing());
	
	// Magic "Play or Pause" button should change it's icon, tooltip etc.
	if (d_animation_controller_ptr->is_playing()) {
		// Playing. So we need to display 'pause'.
		button_play_or_pause->setIcon(button_pause->icon());
		button_play_or_pause->setToolTip(button_pause->toolTip());
	} else {
		// Paused. So we need to display 'play'.
		button_play_or_pause->setIcon(button_play->icon());
		button_play_or_pause->setToolTip(button_play->toolTip());
	}
}
