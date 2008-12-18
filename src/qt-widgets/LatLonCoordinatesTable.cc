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

#include <memory>
#include <boost/cast.hpp>

#include "LatLonCoordinatesTable.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "maths/InvalidLatLonException.h"
#include "maths/InvalidLatLonCoordinateException.h"
#include "maths/LatLonPointConversions.h"
#include "maths/GeometryOnSphere.h"
#include "maths/Real.h"
#include "utils/GeometryCreationUtils.h"
#include "view-operations/GeometryType.h"


namespace
{
	/**
	* The order that coordinates are displayed in the tree widget.
	*/
	enum LatLonColumnLayout
	{
		COLUMN_LAT, COLUMN_LON
	};	


	/**
	* Creates a top-level QTreeWidgetItem used to distinguish
	* between parts of multi-geometries and polygon innards.
	*/
	std::auto_ptr<QTreeWidgetItem>
	create_geometry_item(
			const QString &label = QString())
	{
		static const QBrush background(Qt::darkGray);
		static const QBrush foreground(Qt::white);

		// We cannot use the "Span Columns" trick unless the item is first added to the
		// QTreeWidget.
		std::auto_ptr<QTreeWidgetItem> geom_item(new QTreeWidgetItem());
		geom_item->setText(0, label);
		geom_item->setBackground(0, background);
		geom_item->setForeground(0, foreground);
		geom_item->setFirstColumnSpanned(true);
		geom_item->setExpanded(true);

		return geom_item;
	}


	/**
	* Modifies the lat/lon of an existing tree widget item.
	*/
	void
	set_lat_lon(
			QTreeWidgetItem *item,
			double lat,
			double lon)
	{
		static QLocale locale;

		// The text: What the item displays
		item->setText(COLUMN_LAT, locale.toString(lat));
		item->setText(COLUMN_LON, locale.toString(lon));
	}


	/**
	* Turns a lat,lon pair into a tree widget item ready for insertion
	* into the tree.
	*/
	std::auto_ptr<QTreeWidgetItem>
	create_lat_lon_item(
			double lat,
			double lon)
	{
		std::auto_ptr<QTreeWidgetItem> item(new QTreeWidgetItem());
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		set_lat_lon(item.get(), lat, lon);

		return item;
	}

	QString
	get_geometry_type_text(
			GPlatesViewOperations::GeometryType::Value geom_type)
	{
		QString label;

		switch (geom_type)
		{
		case GPlatesViewOperations::GeometryType::POINT:
			label = "gml:Point";
			break;

		case GPlatesViewOperations::GeometryType::MULTIPOINT:
			label = "gml:MultiPoint";
			break;

		case GPlatesViewOperations::GeometryType::POLYLINE:
			label = "gml:LineString";
			break;

		case GPlatesViewOperations::GeometryType::POLYGON:
			label = "gml:Polygon";
			break;

		default:
			label = QObject::tr("<Error: unknown GeometryType>");
		}

		return label;
	}
}

GPlatesQtWidgets::LatLonCoordinatesTable::LatLonCoordinatesTable(
		QTreeWidget *coordinates_table,
		GPlatesViewOperations::GeometryBuilder *initial_geom_builder) :
d_coordinates_table(coordinates_table),
d_current_geom_builder(NULL)
{
	set_geometry_builder(initial_geom_builder);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::set_geometry_builder(
		GPlatesViewOperations::GeometryBuilder *geom_builder)
{
	// If the new geometry builder is the same as current one then do nothing.
	if (geom_builder == d_current_geom_builder)
	{
		return;
	}

	if (d_current_geom_builder != NULL)
	{
		disconnect_from_current_geometry_builder();
	}

	d_current_geom_builder = geom_builder;


	if (d_current_geom_builder != NULL)
	{
		connect_to_current_geometry_builder();

		initialise_table_from_current_geometry_builder();
	}
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::connect_to_current_geometry_builder()
{
	// Change geometry type in our table.
	QObject::connect(
			d_current_geom_builder,
			SIGNAL(changed_actual_geometry_type(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryType::Value)),
			this,
			SLOT(change_actual_geometry_type(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryType::Value)));

	// Insert geometry into our table.
	QObject::connect(
			d_current_geom_builder,
			SIGNAL(inserted_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)),
			this,
			SLOT(insert_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)));

	// Remove geometry into our table.
	QObject::connect(
			d_current_geom_builder,
			SIGNAL(removed_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)),
			this,
			SLOT(remove_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)));

	// Insert point into a geometry in our table.
	QObject::connect(
			d_current_geom_builder,
			SIGNAL(inserted_point_into_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &)),
			this,
			SLOT(insert_point_into_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &)));

	// Remove point from a geometry in our table.
	QObject::connect(
			d_current_geom_builder,
			SIGNAL(removed_point_from_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex)),
			this,
			SLOT(remove_point_from_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex)));

	// Moved point in a geometry in our table.
	QObject::connect(
			d_current_geom_builder,
			SIGNAL(moved_point_in_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &,
					bool)),
			this,
			SLOT(move_point_in_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &)));
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::disconnect_from_current_geometry_builder()
{
	// Disconnect all signals from the current geometry builder.
	QObject::disconnect(d_current_geom_builder, 0, this, 0);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::initialise_table_from_current_geometry_builder()
{
	//
	// First remove any items we've filled in so far.
	//

	QTreeWidgetItem *root = d_coordinates_table->invisibleRootItem();
	int remove_child_index;
	for (remove_child_index = 0;
		remove_child_index < root->childCount();
		++remove_child_index)
	{
		remove_geometry(remove_child_index);
	}

	//
	// Then add an item for each internal geometry in the current GeometryBuilder.
	//

	GPlatesViewOperations::GeometryBuilder::GeometryIndex insert_child_index;
	for (insert_child_index = 0;
		insert_child_index < d_current_geom_builder->get_num_geometries();
		++insert_child_index)
	{
		insert_geometry(insert_child_index);
	}
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::change_actual_geometry_type(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryType::Value geometry_type)
{
	QTreeWidgetItem *root = d_coordinates_table->invisibleRootItem();

	GPlatesGlobal::Assert(
		boost::numeric_cast<int>(geometry_index) < root->childCount(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
	QTreeWidgetItem *geom_item = root->child(geometry_index);

	const QString label = get_geometry_type_text(geometry_type);
	geom_item->setText(0, label);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::insert_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index)
{
	QTreeWidgetItem *root = d_coordinates_table->invisibleRootItem();
	GPlatesGlobal::Assert(
		boost::numeric_cast<int>(geometry_index) <= root->childCount(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Get actual type of geometry.
	const GPlatesViewOperations::GeometryType::Value geom_type =
		d_current_geom_builder->get_actual_type_of_geometry(geometry_index);

	// Get geometry type text.
	const QString geom_type_text = get_geometry_type_text(geom_type);

	// Create top-level tree widget item corresponding to inserted geometry.
	std::auto_ptr<QTreeWidgetItem> geometry_item = create_geometry_item(geom_type_text);
	d_coordinates_table->insertTopLevelItem(geometry_index, geometry_item.release());

	//
	// If inserted geometry contains any points then add them also.
	//
	const unsigned int num_points_in_geom =
		d_current_geom_builder->get_num_points_in_geometry(geometry_index);

	// Iterate through all points in inserted geometry.
	for (unsigned int point_index = 0;
		point_index < num_points_in_geom;
		++point_index)
	{
		// Get point in inserted geometry.
		const GPlatesMaths::PointOnSphere &point =
			d_current_geom_builder->get_geometry_point(geometry_index, point_index);

		// Insert point into our table.
		insert_point_into_geometry(geometry_index, point_index, point);
	}
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::remove_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index)
{
	QTreeWidgetItem *root = d_coordinates_table->invisibleRootItem();
	GPlatesGlobal::Assert(
		boost::numeric_cast<int>(geometry_index) < root->childCount(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Delete top-level tree widget item corresponding to removed geometry.
	delete d_coordinates_table->takeTopLevelItem(geometry_index);

	// If removed geometry contains points then it doesn't matter since
	// deleting parent will also delete its children.
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::insert_point_into_current_geometry(
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_globe)
{
	// Get index of current geometry.
	const GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index =
		d_current_geom_builder->get_current_geometry_index();

	insert_point_into_geometry(geometry_index, point_index, oriented_pos_on_globe);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::insert_point_into_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_globe)
{
	QTreeWidgetItem *root = d_coordinates_table->invisibleRootItem();
	// Figure out which 'geometry' QTreeWidgetItem is the one where we need to add
	// this coordinate.

	GPlatesGlobal::Assert(
		boost::numeric_cast<int>(geometry_index) < root->childCount(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
	QTreeWidgetItem *geom_item = root->child(geometry_index);

	const GPlatesMaths::LatLonPoint& lat_lon_point =
		GPlatesMaths::make_lat_lon_point(oriented_pos_on_globe);

	// Create the 'coordinate' QTreeWidgetItem and add it.
	std::auto_ptr<QTreeWidgetItem> coord_item_auto_ptr = create_lat_lon_item(
		lat_lon_point.latitude(),
		lat_lon_point.longitude());
	QTreeWidgetItem* coord_item = coord_item_auto_ptr.get();

	geom_item->insertChild(point_index, coord_item_auto_ptr.release());

	const GPlatesViewOperations::GeometryType::Value geom_type =
		d_current_geom_builder->get_actual_type_of_current_geometry();
	const QString label = get_geometry_type_text(geom_type);
	geom_item->setText(0, label);

	// FIXME: Re-applying these properties has only become necessary with the
	// addition of the multi-geom aware DigitisationChangeGeometryType command.
	// Something needs to be done about this awkward situation.
	static const QBrush background(Qt::darkGray);
	static const QBrush foreground(Qt::white);
	geom_item->setBackground(0, background);
	geom_item->setForeground(0, foreground);
	geom_item->setFirstColumnSpanned(true);
	geom_item->setExpanded(true);

	// Scroll to show the user the point they just added.
	d_coordinates_table->scrollToItem(coord_item);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::move_point_in_current_geometry(
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesMaths::PointOnSphere &new_oriented_pos_on_globe)
{
	// Get index of current geometry.
	const GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index =
		d_current_geom_builder->get_current_geometry_index();

	QTreeWidgetItem *root = d_coordinates_table->invisibleRootItem();
	// Figure out which 'geometry' QTreeWidgetItem is the one where we need to
	// modify this coordinate.

	GPlatesGlobal::Assert(
		boost::numeric_cast<int>(geometry_index) < root->childCount(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
	QTreeWidgetItem *geom_item = root->child(geometry_index);

	const GPlatesMaths::LatLonPoint& lat_lon_point =
		GPlatesMaths::make_lat_lon_point(new_oriented_pos_on_globe);

	// Create the 'coordinate' QTreeWidgetItem and add it.
	QTreeWidgetItem* coord_item = geom_item->child(point_index);

	// Change the latitude and longitude.
	set_lat_lon(coord_item, lat_lon_point.latitude(), lat_lon_point.longitude());

	// Scroll to show the user the point they just added.
	d_coordinates_table->scrollToItem(coord_item);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::remove_point_from_current_geometry(
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index)
{
	// Get index of current geometry.
	const GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index =
		d_current_geom_builder->get_current_geometry_index();

	remove_point_from_geometry(geometry_index, point_index);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::remove_point_from_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index)
{
	QTreeWidgetItem *root = d_coordinates_table->invisibleRootItem();
	// Figure out which 'geometry' QTreeWidgetItem is the one where we need to add
	// this coordinate.

	GPlatesGlobal::Assert(
		boost::numeric_cast<int>(geometry_index) < root->childCount(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
	QTreeWidgetItem *geom_item = root->child(geometry_index);

	GPlatesGlobal::Assert(
		boost::numeric_cast<int>(point_index) < geom_item->childCount(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));
	delete geom_item->takeChild(point_index);
}
