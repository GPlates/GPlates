/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <QLocale>
#include <QHBoxLayout>
#include <QSplitter>

#include "ReconstructionViewWidget.h"
#include "ViewportWindow.h"
#include "GlobeCanvas.h"

#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "utils/FloatingPointComparisons.h"


GPlatesQtWidgets::ReconstructionViewWidget::ReconstructionViewWidget(
		ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_splitter_widget(new QSplitter(this))
{
	setupUi(this);

	// ensures that this widget accepts keyEvents, so that the keyPressEvent method is processed from start-up,
	// irrespective on which window (if any) the user has clicked. 
	setFocusPolicy(Qt::StrongFocus);

#if 0
	// Create the GlobeCanvas,
	d_canvas_ptr = new GlobeCanvas(view_state, this);
	// and add it to the grid layout in the ReconstructionViewWidget.
	// Note this is a bit of a hack, relying on the QGridLayout set up in the Designer.

	gridLayout->addWidget(d_canvas_ptr, 0, 0);
#endif
	// Create the GlobeCanvas.
	d_globe_canvas_ptr = new GlobeCanvas(view_state, this);
	// Create the ZoomSliderWidget for the right-hand side.
	d_zoom_slider_widget = new ZoomSliderWidget(d_globe_canvas_ptr->viewport_zoom(), this);


	// Create a tiny invisible widget with a tiny invisible horizontal layout to
	// hold the "canvas" area (including the zoom slider). This layout will glue
	// the zoom slider to the right hand side of the canvas. We set a custom size
	// policy in an attempt to make sure that the GlobeCanvas+ZoomSlider eat as
	// much space as possible, leaving the TaskPanel to the default minimum.
	QWidget *canvas_widget = new QWidget(d_splitter_widget);
	QSizePolicy canvas_widget_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	canvas_widget_size_policy.setHorizontalStretch(255);
	canvas_widget->setSizePolicy(canvas_widget_size_policy);
	QHBoxLayout *canvas_widget_layout = new QHBoxLayout(canvas_widget);
	canvas_widget_layout->setSpacing(0);
	canvas_widget_layout->setContentsMargins(0, 0, 0, 0);
	// Add GlobeCanvas and ZoomSliderWidget to this hand-made widget.
	canvas_widget_layout->addWidget(d_globe_canvas_ptr);
	canvas_widget_layout->addWidget(d_zoom_slider_widget);
	// Then add that widget (globe + zoom slider) to the QSplitter.
	d_splitter_widget->addWidget(canvas_widget);
	
	// Add the QSplitter to the placeholder widget in the ReconstructionViewWidget.
	// Note this is a bit of a hack, relying on the canvas_taskpanel_place_holder widget
	// set up in the Designer.
	// We need to make an 'invisible' QLayout inside that widget first, with zero margins
	// and spacing. Just to hold one QSplitter widget. Trust me, without this, we end
	// up with seriously weird vertical stretching in the ReconstructionViewWidget.
	QHBoxLayout *splitter_layout = new QHBoxLayout(canvas_taskpanel_place_holder);
	splitter_layout->setSpacing(0);
	splitter_layout->setContentsMargins(0, 0, 0, 0);
	splitter_layout->addWidget(d_splitter_widget);
	

	// Set up the Reconstruction Time spinbox and buttons.
	spinbox_reconstruction_time->setRange(
			ReconstructionViewWidget::min_reconstruction_time(),
			ReconstructionViewWidget::max_reconstruction_time());
	spinbox_reconstruction_time->setValue(0.0);
	QObject::connect(spinbox_reconstruction_time, SIGNAL(editingFinished()),
			this, SLOT(propagate_reconstruction_time()));
	QObject::connect(spinbox_reconstruction_time, SIGNAL(editingFinished()),
			d_globe_canvas_ptr, SLOT(setFocus()));

	QObject::connect(button_reconstruction_increment, SIGNAL(clicked()),
			this, SLOT(increment_reconstruction_time()));
	QObject::connect(button_reconstruction_decrement, SIGNAL(clicked()),
			this, SLOT(decrement_reconstruction_time()));

	// Set up the Reconstruction Time slider 
	slider_reconstruction_time->setRange(0, 300); // FIXME : use the above values? 
	slider_reconstruction_time->setValue(0);
	slider_reconstruction_time->setInvertedAppearance( true );
	slider_reconstruction_time->setInvertedControls( true );

	// synchronize the slider and spinbox 
	QObject::connect(slider_reconstruction_time, SIGNAL(valueChanged(int)),
			this, SLOT(set_reconstruction_time_int(int)));


	// Listen for zoom events, everything is now handled through ViewportZoom.
	GPlatesGui::ViewportZoom &vzoom = d_globe_canvas_ptr->viewport_zoom();
	QObject::connect(&vzoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_change()));

	// Zoom buttons.
	QObject::connect(button_zoom_in, SIGNAL(clicked()),
			&vzoom, SLOT(zoom_in()));
	QObject::connect(button_zoom_out, SIGNAL(clicked()),
			&vzoom, SLOT(zoom_out()));
	QObject::connect(button_zoom_reset, SIGNAL(clicked()),
			&vzoom, SLOT(reset_zoom()));

	// Zoom spinbox.
	QObject::connect(spinbox_zoom_percent, SIGNAL(editingFinished()),
			this, SLOT(propagate_zoom_percent()));
	QObject::connect(spinbox_zoom_percent, SIGNAL(editingFinished()),
			d_globe_canvas_ptr, SLOT(setFocus()));

	// Zoom slider.
	QObject::connect(d_zoom_slider_widget, SIGNAL(slider_moved(int)),
			&vzoom, SLOT(set_zoom_level(int)));

	QObject::connect(&(d_globe_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			d_globe_canvas_ptr, SLOT(notify_of_orientation_change()));
	QObject::connect(&(d_globe_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			this, SLOT(recalc_camera_position()));
	QObject::connect(&(d_globe_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			d_globe_canvas_ptr, SLOT(force_mouse_pointer_pos_change()));

	recalc_camera_position();
}


void
GPlatesQtWidgets::ReconstructionViewWidget::insert_task_panel(
		GPlatesQtWidgets::TaskPanel *task_panel)
{
	// Add the Task Panel to the right-hand edge of the QSplitter in
	// the middle of the ReconstructionViewWidget.
	d_splitter_widget->addWidget(task_panel);
}


bool
GPlatesQtWidgets::ReconstructionViewWidget::is_valid_reconstruction_time(
		const double &time)
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	// Firstly, ensure that the time is not less than the minimum reconstruction time.
	if (time < ReconstructionViewWidget::min_reconstruction_time() &&
			! geo_times_are_approx_equal(time,
					ReconstructionViewWidget::min_reconstruction_time())) {
		return false;
	}

	// Secondly, ensure that the time is not greater than the maximum reconstruction time.
	if (time > ReconstructionViewWidget::max_reconstruction_time() &&
			! geo_times_are_approx_equal(time,
					ReconstructionViewWidget::min_reconstruction_time())) {
		return false;
	}

	// Otherwise, it's a valid time.
	return true;
}


void
GPlatesQtWidgets::ReconstructionViewWidget::set_reconstruction_time(
		double new_recon_time)
{
	using namespace GPlatesUtils::FloatingPointComparisons;

	// Ensure the new reconstruction time is valid.
	if ( ! ReconstructionViewWidget::is_valid_reconstruction_time(new_recon_time)) {
		return;
	}

	if ( ! geo_times_are_approx_equal(reconstruction_time(), new_recon_time)) {
		spinbox_reconstruction_time->setValue(new_recon_time);
		propagate_reconstruction_time();
	}
}


void
GPlatesQtWidgets::ReconstructionViewWidget::recalc_camera_position()
{
	static const GPlatesMaths::PointOnSphere centre_of_canvas =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	GPlatesMaths::PointOnSphere oriented_centre = d_globe_canvas_ptr->globe().Orient(centre_of_canvas);
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(oriented_centre);

	QLocale locale_;
	QString lat = locale_.toString(llp.latitude(), 'f', 2);
	QString lon = locale_.toString(llp.longitude(), 'f', 2);
	QString position_as_string(QObject::tr("(lat: "));
	position_as_string.append(lat);
	position_as_string.append(QObject::tr(" ; lon: "));
	position_as_string.append(lon);
	position_as_string.append(QObject::tr(")"));

	label_camera_coords->setText(position_as_string);
}


void
GPlatesQtWidgets::ReconstructionViewWidget::update_mouse_pointer_position(
		const GPlatesMaths::PointOnSphere &new_virtual_pos,
		bool is_on_globe)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(new_virtual_pos);

	QLocale locale_;
	QString lat = locale_.toString(llp.latitude(), 'f', 2);
	QString lon = locale_.toString(llp.longitude(), 'f', 2);
	QString position_as_string(QObject::tr("(lat: "));
	position_as_string.append(lat);
	position_as_string.append(QObject::tr(" ; lon: "));
	position_as_string.append(lon);
	position_as_string.append(QObject::tr(")"));
	if ( ! is_on_globe) {
		position_as_string.append(QObject::tr(" (off globe)"));
	}

	label_mouse_coords->setText(position_as_string);
}



void
GPlatesQtWidgets::ReconstructionViewWidget::propagate_zoom_percent()
{
	d_globe_canvas_ptr->viewport_zoom().set_zoom_percent(zoom_percent());
}


void
GPlatesQtWidgets::ReconstructionViewWidget::handle_zoom_change()
{
	spinbox_zoom_percent->setValue(d_globe_canvas_ptr->viewport_zoom().zoom_percent());
	d_zoom_slider_widget->set_zoom_value(d_globe_canvas_ptr->viewport_zoom().zoom_level());
}


