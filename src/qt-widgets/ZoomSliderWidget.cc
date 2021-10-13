/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2011 The University of Sydney, Australia
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

#include <QVBoxLayout>

#include "ZoomSliderWidget.h"

#include "gui/ViewportZoom.h"


namespace
{
	const int NUM_STEPS_PER_LEVEL = 100;
}


GPlatesQtWidgets::ZoomSliderWidget::ZoomSliderWidget(
		GPlatesGui::ViewportZoom &vzoom,
		QWidget *parent_):
	QWidget(parent_),
	d_viewport_zoom_ptr(&vzoom),
	d_slider_zoom(new ZoomSlider(this)),
	d_suppress_zoom_change_event(false)
{
	set_up_ui();
	set_up_signals_and_slots();
}


void
GPlatesQtWidgets::ZoomSliderWidget::set_up_ui()
{
	// Set our own properties.
	setObjectName("ZoomSlider");
	setFocusPolicy(Qt::NoFocus);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	// Set up the widgets, as though the Designer had created them.
	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setContentsMargins(0, 0, 0, 0);
	vbox->setSpacing(2);

	QLabel *label_zoom_max = new ZoomIcon(
			QPixmap(QString::fromUtf8(":/gnome_zoom_in_16.png")),
			d_slider_zoom,
			QAbstractSlider::SliderPageStepAdd,
			this);
	vbox->addWidget(label_zoom_max);

	d_slider_zoom->setOrientation(Qt::Vertical);
	d_slider_zoom->setTickPosition(QSlider::NoTicks);
	d_slider_zoom->setFocusPolicy(Qt::WheelFocus);
	vbox->addWidget(d_slider_zoom);

	QLabel *label_zoom_min = new ZoomIcon(
			QPixmap(QString::fromUtf8(":/gnome_zoom_out_16.png")),
			d_slider_zoom,
			QAbstractSlider::SliderPageStepSub,
			this);
	vbox->addWidget(label_zoom_min);

	// Set up the zoom slider to use appropriate range and current zoom level.
	d_slider_zoom->setSingleStep(1);
	d_slider_zoom->setRange(
			0,
			(d_viewport_zoom_ptr->s_max_zoom_level - d_viewport_zoom_ptr->s_min_zoom_level) *
				NUM_STEPS_PER_LEVEL);
	handle_zoom_changed();
}


void
GPlatesQtWidgets::ZoomSliderWidget::set_up_signals_and_slots()
{
	// When the user moves the slider, change the zoom level.
	QObject::connect(d_slider_zoom, SIGNAL(valueChanged(int)),
			this, SLOT(handle_slider_moved(int)));
	
	// When the zoom level changes, move the slider (ideally in a way that
	// does not emit more change events!)
	QObject::connect(d_viewport_zoom_ptr, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_changed()));
}


void
GPlatesQtWidgets::ZoomSliderWidget::handle_slider_moved(
		int slider_position)
{
	if ( ! d_suppress_zoom_change_event) {
		d_viewport_zoom_ptr->set_zoom_level(
			static_cast<double>(slider_position) / NUM_STEPS_PER_LEVEL +
				d_viewport_zoom_ptr->s_min_zoom_level);
	}
}


void
GPlatesQtWidgets::ZoomSliderWidget::handle_zoom_changed()
{
	// ViewportZoom has been changed.
	
	// We must ensure that in changing our slider, we do not accidentally
	// cause a further change to ViewportZoom.
	d_suppress_zoom_change_event = true;
	
	int new_zoom_level = static_cast<int>(d_viewport_zoom_ptr->zoom_level() * NUM_STEPS_PER_LEVEL + 0.5);
	d_slider_zoom->setValue(new_zoom_level);

	// Re-enable the modification of ViewportZoom from this slider by the user.
	d_suppress_zoom_change_event = false;
}

