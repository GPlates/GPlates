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

#ifndef GPLATES_QTWIDGETS_MODIFYGEOMETRYWIDGET_H
#define GPLATES_QTWIDGETS_MODIFYGEOMETRYWIDGET_H

#include <QDebug>
#include <QWidget>
#include <QTreeWidget>
#include <boost/scoped_ptr.hpp>

#include "ui_ModifyGeometryWidgetUi.h"
#include "LatLonCoordinatesTable.h"
#include "TaskPanelWidget.h"

#include "maths/GeometryOnSphere.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
}

namespace GPlatesQtWidgets
{
	class LatLonCoordinatesTable;

	class ModifyGeometryWidget :
			public TaskPanelWidget, 
			protected Ui_ModifyGeometryWidget
	{
		Q_OBJECT

	public:

		explicit
		ModifyGeometryWidget(
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				QWidget *parent_ = NULL);

		~ModifyGeometryWidget();

		void
		reload_coordinates_table_if_necessary()
		{
			d_lat_lon_coordinates_table->reload_if_necessary();
		}

		virtual
		void
		handle_activation();

	private:

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

#endif // GPLATES_QTWIDGETS_MODIFYGEOMETRYWIDGET_H
