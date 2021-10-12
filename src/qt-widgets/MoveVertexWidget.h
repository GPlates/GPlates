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

#ifndef GPLATES_QTWIDGETS_MOVEVERTEXWIDGET_H
#define GPLATES_QTWIDGETS_MOVEVERTEXWIDGET_H

#include <QDebug>
#include <QWidget>
#include <QTreeWidget>
#include <boost/scoped_ptr.hpp>
#include "MoveVertexWidgetUi.h"

#include "maths/GeometryOnSphere.h"


namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class GeometryBuilderToolTarget;
}

namespace GPlatesQtWidgets
{
	class LatLonCoordinatesTable;

	class MoveVertexWidget:
			public QWidget, 
			protected Ui_MoveVertexWidget
	{
		Q_OBJECT

	public:
		explicit
		MoveVertexWidget(
				GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target,
				QWidget *parent_ = NULL);

		~MoveVertexWidget();

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Listen for changes to the geometry builder targeted by
		 * the move vertex tool.
		 */
		void
		switched_move_vertex_geometry_builder(
				GPlatesViewOperations::GeometryBuilderToolTarget &,
				GPlatesViewOperations::GeometryBuilder *);

	private:
		void
		connect_to_geometry_builder_tool_target(
				GPlatesViewOperations::GeometryBuilderToolTarget &);

		QTreeWidget *
		coordinates_table()
		{
			return treewidget_coordinates;
		}

		/**
		 * A wrapper around coordinates table that listens to a GeometryBuilder
		 * and fills in the table accordingly.
		 */
		boost::scoped_ptr<LatLonCoordinatesTable> d_lat_lon_coordinates_table;
	};
}

#endif // GPLATES_QTWIDGETS_MOVEVERTEXWIDGET_H
