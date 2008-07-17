/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "EditGeometryWidget.h"
#include "EditGeometryActionWidget.h"


GPlatesQtWidgets::EditGeometryActionWidget::EditGeometryActionWidget(
		GPlatesQtWidgets::EditGeometryWidget &geometry_widget,
		QWidget *parent_):
	QWidget(parent_),
	d_geometry_widget_ptr(&geometry_widget)
{
	setupUi(this);

	// Set up slots for each button
	QObject::connect(button_insert_above, SIGNAL(clicked()), this, SLOT(insert_point_above()));
	QObject::connect(button_insert_below, SIGNAL(clicked()), this, SLOT(insert_point_below()));
	QObject::connect(button_delete, SIGNAL(clicked()), this, SLOT(delete_point()));

}


void
GPlatesQtWidgets::EditGeometryActionWidget::insert_point_above()
{
	d_geometry_widget_ptr->handle_insert_point_above(this);
}

void
GPlatesQtWidgets::EditGeometryActionWidget::insert_point_below()
{
	d_geometry_widget_ptr->handle_insert_point_below(this);
}

void
GPlatesQtWidgets::EditGeometryActionWidget::delete_point()
{
	d_geometry_widget_ptr->handle_delete_point(this);
}


