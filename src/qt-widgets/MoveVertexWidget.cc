/* $Id$ */

/**
 * \file 
 * Uses the 
 * 
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

#include <QHeaderView>
#include <QWidget>
#include <QTreeWidget>

#include "MoveVertexWidget.h"
#include "LatLonCoordinatesTable.h"
#include "view-operations/GeometryBuilderToolTarget.h"

GPlatesQtWidgets::MoveVertexWidget::MoveVertexWidget(
		GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
	
	// Set up the header of the coordinates widget.
	coordinates_table()->header()->setResizeMode(QHeaderView::Stretch);

	// Get a wrapper around coordinates table that listens to a GeometryBuilder
	// and fills in the table accordingly.
	d_lat_lon_coordinates_table.reset(new LatLonCoordinatesTable(
			coordinates_table(),
			geom_builder_tool_target.get_geometry_builder_for_active_tool()));

	connect_to_geometry_builder_tool_target(geom_builder_tool_target);
}

GPlatesQtWidgets::MoveVertexWidget::~MoveVertexWidget()
{
	// boost::scoped_ptr destructor needs complete type.
}

void
GPlatesQtWidgets::MoveVertexWidget::connect_to_geometry_builder_tool_target(
		GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target)
{
	// Change geometry type in our table.
	QObject::connect(
			&geom_builder_tool_target,
			SIGNAL(switched_move_vertex_geometry_builder(
					GPlatesViewOperations::GeometryBuilderToolTarget &,
					GPlatesViewOperations::GeometryBuilder *)),
			this,
			SLOT(switched_move_vertex_geometry_builder(
					GPlatesViewOperations::GeometryBuilderToolTarget &,
					GPlatesViewOperations::GeometryBuilder *)));
}

void
GPlatesQtWidgets::MoveVertexWidget::switched_move_vertex_geometry_builder(
		GPlatesViewOperations::GeometryBuilderToolTarget &,
		GPlatesViewOperations::GeometryBuilder *new_geom_builder)
{
	d_lat_lon_coordinates_table->set_geometry_builder(new_geom_builder);
}
