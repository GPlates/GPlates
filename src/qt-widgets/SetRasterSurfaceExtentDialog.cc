/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include <QRectF>

#include "GlobeCanvas.h"
#include "InformationDialog.h"
#include "SetRasterSurfaceExtentDialog.h"
#include "ViewportWindow.h"

#include "property-values/InMemoryRaster.h"

const float DEFAULT_LOWER_LEFT_LAT = -90.;
const float DEFAULT_LOWER_LEFT_LON = -180.;
const float DEFAULT_UPPER_RIGHT_LAT = 90.;
const float DEFAULT_UPPER_RIGHT_LON = 180.;

namespace{

	/**
	 * Checks that the lat-lon extent stored in the QRectF extent is valid.
	 * 
	 * A valid extent is one for which the latitude of the top of the QRectF is
	 * greater than the latitude of the bottom, and for which the longitude of the left
	 * of the box is less than the longitude of the right.
	 * 
	 * The dialog allows longitude values in the range -360 to 360, so the user
	 * can specify an extent which crosses the date line. 
	 */
	bool
	extent_is_valid(
		QRectF extent)
	{
		// Make sure the upper latitude is greater than the lower latitude.
		if (extent.top() <= extent.bottom()){
			return false;
		}
		// Make sure the left longitude is less than the right longitude. 
		if (extent.left() >= extent.right()){
			return false;
		}
		return true;
	}
}

const QString GPlatesQtWidgets::SetRasterSurfaceExtentDialog::s_help_dialog_title = QObject::tr(
		"Setting the raster extent");

// FIXME: How useful is a help dialog for us here? 
const QString GPlatesQtWidgets::SetRasterSurfaceExtentDialog::s_help_dialog_text = QObject::tr(
		"<html><body>\n"
		"\n"
		"Raster images are displayed on the globe over an area specified by a lat-lon bounding box." 
		"<ul>\n"
		"<li> The latitude values should be in the range [-90,90], and the upper latitude must be greater than the lower latitude. </li>\n"
		"<li> The longitude values should be in the range [-360,360], and the left longitude must be less than the right longitude. </li>\n"
		"</ul>\n"
		"</body></html>\n");

GPlatesQtWidgets::SetRasterSurfaceExtentDialog::SetRasterSurfaceExtentDialog(
		ViewportWindow &viewport_window, 
		QWidget *parent_):
	QDialog(parent_),
	d_viewport_window_ptr(&viewport_window),
	d_extent(DEFAULT_LOWER_LEFT_LON,
				DEFAULT_LOWER_LEFT_LAT,
				DEFAULT_UPPER_RIGHT_LON-DEFAULT_LOWER_LEFT_LON,
				DEFAULT_UPPER_RIGHT_LAT-DEFAULT_LOWER_LEFT_LAT),
	d_help_dialog(new InformationDialog(s_help_dialog_text, s_help_dialog_title, this))
{
	setupUi(this);

	QObject::connect(button_help,SIGNAL(clicked()),d_help_dialog,SLOT(show()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(handle_cancel()));
	QObject::connect(button_default_fields,SIGNAL(clicked()),this,SLOT(handle_reset_to_default_fields()));
}


void
GPlatesQtWidgets::SetRasterSurfaceExtentDialog::accept()
{
	d_extent.setLeft(spinbox_lower_left_longitude->value());
	d_extent.setTop(spinbox_upper_right_latitude->value());
	d_extent.setRight(spinbox_upper_right_longitude->value());
	d_extent.setBottom(spinbox_lower_left_latitude->value());

	if (extent_is_valid(d_extent))
	{	
		// FIXME:
		// Because we're now popping up this dialog on file-loading, it means
		// we'll be re-mapping an existing texture, even if we're going to replace
		// that texture immediately afterwards with another one. 
		// The time taken to re-map is small though, at least in comparison
		// with file input. 
		d_viewport_window_ptr->globe_canvas().globe().texture().set_extent(d_extent);
		done(QDialog::Accepted);
	}
	else {
		// Don't close the dialog, and do something to inform the user.	
	};
}


void
GPlatesQtWidgets::SetRasterSurfaceExtentDialog::handle_cancel()
{
	// Set the spin boxes back to their original values before we exit.
	spinbox_lower_left_longitude->setValue(d_extent.left());
	spinbox_lower_left_latitude->setValue(d_extent.bottom());
	spinbox_upper_right_longitude->setValue(d_extent.right());
	spinbox_upper_right_latitude->setValue(d_extent.top());

	reject();
}

void
GPlatesQtWidgets::SetRasterSurfaceExtentDialog::handle_reset_to_default_fields()
{
	// Reset the extent and the spin-boxes to (-90,-180),(90,180).
	d_extent.setLeft(DEFAULT_LOWER_LEFT_LON);
	d_extent.setTop(DEFAULT_UPPER_RIGHT_LAT);
	d_extent.setRight(DEFAULT_UPPER_RIGHT_LON);
	d_extent.setBottom(DEFAULT_LOWER_LEFT_LAT);

	spinbox_lower_left_longitude->setValue(d_extent.left());
	spinbox_lower_left_latitude->setValue(d_extent.bottom());
	spinbox_upper_right_longitude->setValue(d_extent.right());
	spinbox_upper_right_latitude->setValue(d_extent.top());

}
