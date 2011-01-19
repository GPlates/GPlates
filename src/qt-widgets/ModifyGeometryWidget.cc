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
#include "view-operations/GeometryOperationTarget.h"


GPlatesQtWidgets::ModifyGeometryWidget::ModifyGeometryWidget(
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		QWidget *parent_):
	TaskPanelWidget(parent_)
{
	setupUi(this);
	
	// Set up the header of the coordinates widget.
	coordinates_table()->header()->setResizeMode(QHeaderView::Stretch);

	// Get a wrapper around coordinates table that listens to a GeometryBuilder
	// and fills in the table accordingly.
	d_lat_lon_coordinates_table.reset(new LatLonCoordinatesTable(
			coordinates_table(),
			geometry_operation_target.get_current_geometry_builder(),
			&active_geometry_operation));

	connect_to_geometry_builder_tool_target(geometry_operation_target);
	
}


GPlatesQtWidgets::ModifyGeometryWidget::~ModifyGeometryWidget()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesQtWidgets::ModifyGeometryWidget::connect_to_geometry_builder_tool_target(
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target)
{
	// Change geometry type in our table.
	QObject::connect(
			&geometry_operation_target,
			SIGNAL(switched_geometry_builder(
					GPlatesViewOperations::GeometryOperationTarget &,
					GPlatesViewOperations::GeometryBuilder *)),
			this,
			SLOT(switched_geometry_builder(
					GPlatesViewOperations::GeometryOperationTarget &,
					GPlatesViewOperations::GeometryBuilder *)));
}


void
GPlatesQtWidgets::ModifyGeometryWidget::switched_geometry_builder(
		GPlatesViewOperations::GeometryOperationTarget &,
		GPlatesViewOperations::GeometryBuilder *new_geom_builder)
{
	d_lat_lon_coordinates_table->set_geometry_builder(new_geom_builder);
}


void
GPlatesQtWidgets::ModifyGeometryWidget::handle_activation()
{
	reload_coordinates_table_if_necessary();
}
