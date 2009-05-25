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


#include "SetProjectionDialog.h"
#include "MapCanvas.h"
#include "ViewportWindow.h"

GPlatesQtWidgets::SetProjectionDialog::SetProjectionDialog(
	ViewportWindow &viewport_window,
	QWidget *parent_):
	QDialog(parent_),
	d_viewport_window_ptr(&viewport_window)
{
	setupUi(this);

	// FIXME: Synchronise these with the definitions in the the MapProjection class. 
	combo_projection->addItem(QString(QObject::tr("3D Globe")));
	combo_projection->addItem(QString(QObject::tr("Rectangular")));
	combo_projection->addItem(QString(QObject::tr("Mercator")));
	combo_projection->addItem(QString(QObject::tr("Mollweide")));
	combo_projection->addItem(QString(QObject::tr("Robinson")));

	// The central_meridian spinbox should be disabled if we're in Orthographic mode. 
	update_central_meridian_status();
	QObject::connect(combo_projection,SIGNAL(currentIndexChanged(int)),this,SLOT(update_central_meridian_status()));

}

void
GPlatesQtWidgets::SetProjectionDialog::set_projection(
	int projection_type_)
{
	combo_projection->setCurrentIndex(projection_type_);
}

void
GPlatesQtWidgets::SetProjectionDialog::set_central_meridian(
	double central_meridian_)
{
	spin_central_meridian->setValue(central_meridian_);
}

void
GPlatesQtWidgets::SetProjectionDialog::setup()
{
	// Get the current projection. 
	int projection = d_viewport_window_ptr->reconstruction_view_widget().map_canvas().projection_type();

	combo_projection->setCurrentIndex(projection);

}

void
GPlatesQtWidgets::SetProjectionDialog::update_central_meridian_status()
{
	spin_central_meridian->setDisabled(
		combo_projection->currentIndex() == GPlatesGui::ORTHOGRAPHIC);
}

int
GPlatesQtWidgets::SetProjectionDialog::projection_type()
{
	return combo_projection->currentIndex();
}

double
GPlatesQtWidgets::SetProjectionDialog::central_meridian()
{
	return spin_central_meridian->value();
}
