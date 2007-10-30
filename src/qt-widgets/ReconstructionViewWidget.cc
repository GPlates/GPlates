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

	d_canvas_ptr = new GlobeCanvas(view_state, this);
	gridLayout->addWidget(d_canvas_ptr, 1, 0);

	spinbox_reconstruction_time->setRange(
			ReconstructionViewWidget::min_reconstruction_time(),
			ReconstructionViewWidget::max_reconstruction_time());
	spinbox_reconstruction_time->setValue(0.0);
	QObject::connect(spinbox_reconstruction_time, SIGNAL(editingFinished()),
			this, SLOT(propagate_reconstruction_time()));

	// Make sure the globe is expanding as much as possible!
	QSizePolicy globe_size_policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	d_canvas_ptr->setSizePolicy(globe_size_policy);
	d_canvas_ptr->updateGeometry();
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
GPlatesQtWidgets::ReconstructionViewWidget::update_mouse_pointer_position(
		const GPlatesMaths::PointOnSphere &new_virtual_pos,
		bool is_on_globe)
{
	GPlatesMaths::LatLonPoint llp =
			GPlatesMaths::LatLonPointConversions::convertPointOnSphereToLatLonPoint(
					new_virtual_pos);

	QLocale locale_;
	QString lat = locale_.toString(llp.latitude().dval(), 'f', 2);
	QString lon = locale_.toString(llp.longitude().dval(), 'f', 2);
	QString position_as_string(QObject::tr("(lat: "));
	position_as_string.append(lat);
	position_as_string.append(QObject::tr(" ; lon: "));
	position_as_string.append(lon);
	position_as_string.append(QObject::tr(")"));
	if ( ! is_on_globe) {
		position_as_string.append(QObject::tr(" (off globe)"));
	}

	label_cursor_coordinates->setText(position_as_string);
}
