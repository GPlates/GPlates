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

#include "ZoomControlWidget.h"
#include "gui/ViewportZoom.h"


GPlatesQtWidgets::ZoomControlWidget::ZoomControlWidget(
		GPlatesGui::ViewportZoom &vzoom,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_viewport_zoom_ptr(&vzoom)
{
	setupUi(this);
	show_buttons(false);
	show_label(true);

	// Zoom buttons.
	QObject::connect(button_zoom_in, SIGNAL(clicked()),
			&vzoom, SLOT(zoom_in()));
	QObject::connect(button_zoom_out, SIGNAL(clicked()),
			&vzoom, SLOT(zoom_out()));
	QObject::connect(button_zoom_reset, SIGNAL(clicked()),
			&vzoom, SLOT(reset_zoom()));

	// Zoom spinbox.
	QObject::connect(spinbox_zoom_percent, SIGNAL(editingFinished()),
			this, SLOT(handle_spinbox_changed()));

	// Listen for zoom events, everything is now handled through ViewportZoom.
	QObject::connect(&vzoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_changed()));
}


void
GPlatesQtWidgets::ZoomControlWidget::show_buttons(
		bool show_)
{
	button_zoom_in->setVisible(show_);
	button_zoom_out->setVisible(show_);
	button_zoom_reset->setVisible(show_);
}


void
GPlatesQtWidgets::ZoomControlWidget::show_label(
		bool show_)
{
	label_zoom->setVisible(show_);
}


void
GPlatesQtWidgets::ZoomControlWidget::activate_zoom_spinbox()
{
	spinbox_zoom_percent->setFocus();
	spinbox_zoom_percent->selectAll();
}


void
GPlatesQtWidgets::ZoomControlWidget::handle_zoom_changed()
{
	spinbox_zoom_percent->setValue(d_viewport_zoom_ptr->zoom_percent());
}


void
GPlatesQtWidgets::ZoomControlWidget::handle_spinbox_changed()
{
	d_viewport_zoom_ptr->set_zoom_percent(spinbox_zoom_percent->value());
	emit editing_finished();
}

