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

#include "view-operations/GeometryBuilder.h"


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
				GPlatesViewOperations::GeometryBuilder *initial_geom_builder = NULL);

		/**
		 * Disconnects from the previous @a GeometryBuilder, if any, and
		 * connects to the specified @a GeometryBuilder.
		 */
		void
		set_geometry_builder(
				GPlatesViewOperations::GeometryBuilder *geom_builder);

	public slots:
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

	private:
		/**
		 * The @a QTreeWidget that we fill in.
		 */
		QTreeWidget *d_coordinates_table;

		/**
		 * The @a GeometryBuilder we are listening to.
		 */
		GPlatesViewOperations::GeometryBuilder *d_current_geom_builder;


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
	};
}

#endif // GPLATES_QTWIDGETS_LATLONCOORDINATESTABLE_H
