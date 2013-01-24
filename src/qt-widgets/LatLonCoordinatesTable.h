/* $Id$ */

/**
 * \file 
 * A wrapper around a QTreeView that listens to a @a GeometryBuilder and displays
 * geometry(s) as types and latitude/longitude coordinates.
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

#ifndef GPLATES_QTWIDGETS_LATLONCOORDINATESTABLE_H
#define GPLATES_QTWIDGETS_LATLONCOORDINATESTABLE_H

#include <QObject>
#include <QTreeWidget>

#include "gui/TreeWidgetBuilder.h"

#include "view-operations/GeometryBuilder.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
}

namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesViewOperations
{
	class GeometryOperation;
}

namespace GPlatesQtWidgets
{
	class LatLonCoordinatesTable :
		public QObject
	{
		Q_OBJECT

	public:
		explicit
		LatLonCoordinatesTable(
				QTreeWidget *coordinates_table,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state);

		void
		reload_if_necessary();

	private Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		change_actual_geometry_type(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryType::Value geometry_type);

		void
		insert_geometry(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index);

		void
		remove_geometry(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index);

		void
		insert_point_into_current_geometry(
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesMaths::PointOnSphere &oriented_pos_on_globe);

		void
		remove_point_from_current_geometry(
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index);

		void
		move_point_in_current_geometry(
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesMaths::PointOnSphere &new_oriented_pos_on_globe);

		/**
		 * The geometry operation emitting signals has changed.
		 * @a geometry_operation is NULL if no @a GeometryOperation is currently activated.
		 */
		void
		switched_geometry_operation(
				GPlatesViewOperations::GeometryOperation *geometry_operation);

		/**
		 * The geometry builder emitting signals has changed.
		 * @a geometry_builder is NULL if no @a GeometryBuilder is currently activated.
		 */
		void
		switched_geometry_builder(
				GPlatesViewOperations::GeometryBuilder *geometry_builder);

		/**
		 * The point at index @a point_index was in the geometry at
		 * index @a geometry_index in the geometry builder @a geometry_builder
		 * was highlighted by a geometry operation.
		 */
		void
		highlight_point_in_geometry(
				GPlatesViewOperations::GeometryBuilder *geometry_builder,
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesGui::Colour &highlight_colour);

		/**
		 * No points are highlighted by this geometry operation in the geometry
		 * builder @a geometry_builder.
		 */
		void
		unhighlight_point_in_geometry(
				GPlatesViewOperations::GeometryBuilder *geometry_builder,
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index);

	private:
		/**
		 * The @a QTreeWidget that we fill in.
		 */
		QTreeWidget *d_coordinates_table;

		/**
		 * Helps assemble our QTreeWidget.
		 */
		GPlatesGui::TreeWidgetBuilder d_tree_widget_builder;

		/**
		 * The @a GeometryBuilder we are listening to.
		 */
		GPlatesViewOperations::GeometryBuilder *d_current_geometry_builder;

		/**
		 * The @a GeometryOperation we are listening to.
		 */
		GPlatesViewOperations::GeometryOperation *d_current_geometry_operation;

		/**
		 * A flag to indicate whether we need to reload data.
		 */
		bool d_need_to_reload_data;

		void
		connect_to_geometry_operation_state_signals(
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state);

		void
		connect_to_current_geometry_operation();

		void
		disconnect_from_current_geometry_operation();

		void
		connect_to_current_geometry_builder();

		void
		disconnect_from_current_geometry_builder();

		/**
		 * Fill in @a QTreeWidget using the current @a GeometryBuilder object.
		 */
		void
		initialise_table_from_current_geometry_builder();

		void
		insert_point_into_geometry(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesMaths::PointOnSphere &oriented_pos_on_globe);

		void
		remove_point_from_geometry(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index);

		QTreeWidgetItem *
		get_coord_item(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index);
	};
}

#endif // GPLATES_QTWIDGETS_LATLONCOORDINATESTABLE_H
