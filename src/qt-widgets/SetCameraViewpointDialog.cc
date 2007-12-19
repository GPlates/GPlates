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
 
#include "SetCameraViewpointDialog.h"
#include "ViewportWindow.h"

GPlatesQtWidgets::SetCameraViewpointDialog::SetCameraViewpointDialog(
		ViewportWindow &viewport_window,
		QWidget *parent_):
	QDialog(parent_),
	d_viewport_window_ptr(&viewport_window)
{
	setupUi(this);
	set_lat_lon(0.0, 0.0);
}

void
GPlatesQtWidgets::SetCameraViewpointDialog::set_lat_lon(
		const double &lat,
		const double &lon)
{
	// Ensure no text is selected.
	spinbox_latitude->clear();
	spinbox_longitude->clear();
	// Update values to those of the actual camera.
	spinbox_latitude->setValue(lat);
	spinbox_longitude->setValue(lon);
	// Place user input in appropriate location.
	spinbox_latitude->setFocus();
	spinbox_latitude->selectAll();
}

