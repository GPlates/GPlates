/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include "ReconstructionViewWidget.h"
#include "ViewportWindow.h"
#include "GlobeCanvas.h"
#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "utils/FloatingPointComparisons.h"


GPlatesQtWidgets::ReconstructionViewWidget::ReconstructionViewWidget(
		ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);

// ensures that this widget accepts keyEvents, so that the keyPressEvent method is processed from start-up,
// irrespective on which window (if any) the user has clicked. 
	setFocusPolicy(Qt::StrongFocus);

	d_canvas_ptr = new GlobeCanvas(view_state, this);
	gridLayout->addWidget(d_canvas_ptr, 1, 0);

	spinbox_reconstruction_time->setRange(
			ReconstructionViewWidget::min_reconstruction_time(),
			ReconstructionViewWidget::max_reconstruction_time());
	spinbox_reconstruction_time->setValue(0.0);
	QObject::connect(spinbox_reconstruction_time, SIGNAL(editingFinished()),
			this, SLOT(propagate_reconstruction_time()));
	QObject::connect(spinbox_reconstruction_time, SIGNAL(editingFinished()),
			d_canvas_ptr, SLOT(setFocus()));

	QObject::connect(button_reconstruction_increment, SIGNAL(clicked()),
			this, SLOT(increment_reconstruction_time()));
	QObject::connect(button_reconstruction_decrement, SIGNAL(clicked()),
			this, SLOT(decrement_reconstruction_time()));

	// Listen for zoom events, everything is now handled through ViewportZoom.
	GPlatesGui::ViewportZoom &vzoom = d_canvas_ptr->viewport_zoom();
	QObject::connect(&vzoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_change()));

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
			d_canvas_ptr, SLOT(setFocus()));

	// Set up the zoom slider to use appropriate range.
	slider_zoom->setRange(d_canvas_ptr->viewport_zoom().s_min_zoom_level,
			d_canvas_ptr->viewport_zoom().s_max_zoom_level);
	slider_zoom->setValue(d_canvas_ptr->viewport_zoom().s_initial_zoom_level);

	QObject::connect(slider_zoom, SIGNAL(sliderMoved(int)),
			&vzoom, SLOT(set_zoom_level(int)));

	QObject::connect(&(d_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			d_canvas_ptr, SLOT(notify_of_orientation_change()));
	QObject::connect(&(d_canvas_ptr->globe().orientation()), SIGNAL(orientation_changed()),
			this, SLOT(recalc_camera_position()));

	// Make sure the globe is expanding as much as possible!
	QSizePolicy globe_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	d_canvas_ptr->setSizePolicy(globe_size_policy);
	d_canvas_ptr->updateGeometry();

	recalc_camera_position();
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

	GPlatesMaths::PointOnSphere oriented_centre = d_canvas_ptr->globe().Orient(centre_of_canvas);
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
	d_canvas_ptr->viewport_zoom().set_zoom_percent(zoom_percent());
}


void
GPlatesQtWidgets::ReconstructionViewWidget::handle_zoom_change()
{
	spinbox_zoom_percent->setValue(d_canvas_ptr->viewport_zoom().zoom_percent());
	slider_zoom->setValue(d_canvas_ptr->viewport_zoom().zoom_level());
}


