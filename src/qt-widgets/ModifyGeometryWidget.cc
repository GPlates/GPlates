/* $Id$ */

/**
 * \file Displays lat/lon points of geometry being modified by a canvas tool.
 * 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2009, 2011 The University of Sydney, Australia
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

#include <QHeaderView>
#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "ModifyGeometryWidget.h"
#include "LatLonCoordinatesTable.h"


GPlatesQtWidgets::ModifyGeometryWidget::ModifyGeometryWidget(
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		QWidget *parent_):
	TaskPanelWidget(parent_)
{
	setupUi(this);
	
	// Set up the header of the coordinates widget.
	coordinates_table()->header()->setSectionResizeMode(QHeaderView::Stretch);

	// Get a wrapper around coordinates table that listens to a GeometryBuilder
	// and fills in the table accordingly.
	d_lat_lon_coordinates_table.reset(
			new LatLonCoordinatesTable(coordinates_table(), geometry_operation_state));

}


GPlatesQtWidgets::ModifyGeometryWidget::~ModifyGeometryWidget()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesQtWidgets::ModifyGeometryWidget::handle_activation()
{
	reload_coordinates_table_if_necessary();
}
