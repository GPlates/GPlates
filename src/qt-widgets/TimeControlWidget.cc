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

#include "TimeControlWidget.h"

#include <QFont>
#include <QtGlobal>

#include "gui/AnimationController.h"


QDockWidget *
GPlatesQtWidgets::TimeControlWidget::create_as_qdockwidget(
		GPlatesGui::AnimationController &animation_controller)
{
	QDockWidget *dock = new QDockWidget(tr("Time Controls"));
	dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
	
	TimeControlWidget *controls = new TimeControlWidget(
			animation_controller, dock);
	dock->setWidget(controls);
	
	return dock;
}


GPlatesQtWidgets::TimeControlWidget::TimeControlWidget(
		GPlatesGui::AnimationController &animation_controller,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_animation_controller_ptr(&animation_controller)
{
	setupUi(this);
	show_step_buttons(false);
	show_label(true);

#if 0
	// FIXME: Adapt this snippet to this widget, once we have a better
	// place to store this min/max setting.
	spinbox_reconstruction_time->setRange(
			ReconstructionViewWidget::min_reconstruction_time(),
			ReconstructionViewWidget::max_reconstruction_time());
	spinbox_reconstruction_time->setValue(0.0);
#endif

	// Set up our buttons and spinbox.
	QObject::connect(button_reconstruction_increment, SIGNAL(clicked()),
			d_animation_controller_ptr, SLOT(step_back()));
	QObject::connect(button_reconstruction_decrement, SIGNAL(clicked()),
			d_animation_controller_ptr, SLOT(step_forward()));
	QObject::connect(spinbox_current_time, SIGNAL(editingFinished()),
			this, SLOT(handle_time_spinbox_editing_finished()));
	
	// React to time-change events and update our widgets accordingly.
	QObject::connect(d_animation_controller_ptr, SIGNAL(view_time_changed(double)),
			this, SLOT(handle_view_time_changed(double)));

	// Special: on OS X, the style stubbornly refuses to scale up the font size used
	// in the TimeControlWidget spinbox, despite it being allocated plenty of space.
	// However, we can manually bump the point size up a bit to compensate.
#ifdef Q_OS_MAC
	QFont time_font = spinbox_current_time->font();
	time_font.setPointSize(20);
	spinbox_current_time->setFont(time_font);
#endif
}


void
GPlatesQtWidgets::TimeControlWidget::show_step_buttons(
		bool show_)
{
	// But wait! We might want to try putting the fwd/rev buttons elsewhere.
	// So we'll hide these ones. Until it turns out they looked better here.
	button_reconstruction_increment->setVisible(show_);
	button_reconstruction_decrement->setVisible(show_);
}


void
GPlatesQtWidgets::TimeControlWidget::show_label(
		bool show_)
{
	label_time->setVisible(show_);
}


void
GPlatesQtWidgets::TimeControlWidget::activate_time_spinbox()
{
	spinbox_current_time->setFocus();
	spinbox_current_time->selectAll();
}


void
GPlatesQtWidgets::TimeControlWidget::handle_time_spinbox_editing_finished()
{
	d_animation_controller_ptr->set_view_time(spinbox_current_time->value());
	Q_EMIT editing_finished();
}


void
GPlatesQtWidgets::TimeControlWidget::handle_view_time_changed(
		double new_time)
{
	spinbox_current_time->setValue(new_time);
}


