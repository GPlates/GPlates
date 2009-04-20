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
#include <boost/bind.hpp>
#include <boost/cast.hpp>
#include <QColor>

#include "LatLonCoordinatesTable.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "gui/Colour.h"
#include "maths/InvalidLatLonException.h"
#include "maths/InvalidLatLonCoordinateException.h"
#include "maths/LatLonPointConversions.h"
#include "maths/GeometryOnSphere.h"
#include "maths/Real.h"
#include "utils/GeometryCreationUtils.h"
#include "utils/StringFormattingUtils.h"
#include "view-operations/ActiveGeometryOperation.h"
#include "view-operations/GeometryOperation.h"
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
	GPlatesGui::TreeWidgetBuilder::item_handle_type
	create_geometry_item(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			const QString &label = QString())
	{
		static const QBrush background(Qt::darkGray);
		static const QBrush foreground(Qt::white);

		const GPlatesGui::TreeWidgetBuilder::item_handle_type geom_item_handle =
				tree_widget_builder.create_item();

		QTreeWidgetItem *qtree_widget_item = tree_widget_builder.get_qtree_widget_item(
				geom_item_handle);

		qtree_widget_item->setText(0, label);
		qtree_widget_item->setBackground(0, background);
		qtree_widget_item->setForeground(0, foreground);

		// We cannot use the "Span Columns" trick unless the item is first added to the
		// QTreeWidget.
		// Call function later when QTreeWidgetItem is connected to QTreeWidget.
		tree_widget_builder.add_function(geom_item_handle,
			boost::bind(&QTreeWidgetItem::setFirstColumnSpanned, _1, true));

		// Call function later when QTreeWidgetItem is connected to QTreeWidget.
		tree_widget_builder.add_function(geom_item_handle,
			boost::bind(&QTreeWidgetItem::setExpanded, _1, true));

		return geom_item_handle;
	}


	/**
	 * Sets the QTreeWidgetItem's foreground/background colour to the highlight colour.
	 */
	void
	highlight_lat_lon(
			QTreeWidgetItem *coord_item,
			const GPlatesGui::Colour &highlight_colour)
	{
		QColor background_colour;
		background_colour.setRedF(highlight_colour.red());
		background_colour.setGreenF(highlight_colour.green());
		background_colour.setBlueF(highlight_colour.blue());
		background_colour.setAlphaF(highlight_colour.alpha());

		const QBrush background(background_colour);

		static const QBrush foreground(Qt::black);

		coord_item->setBackground(COLUMN_LAT, background);
		coord_item->setBackground(COLUMN_LON, background);

		coord_item->setForeground(COLUMN_LAT, foreground);
		coord_item->setForeground(COLUMN_LON, foreground);
	}


	/**
	 * Sets the QTreeWidgetItem's foreground/background colour to the unhighlight colour.
	 */
	void
	unhighlight_lat_lon(
			QTreeWidgetItem *coord_item)
	{
		// This should match the default colours.
		static const QBrush background(Qt::white);
		static const QBrush foreground(Qt::black);

		coord_item->setBackground(COLUMN_LAT, background);
		coord_item->setBackground(COLUMN_LON, background);

		coord_item->setForeground(COLUMN_LAT, foreground);
		coord_item->setForeground(COLUMN_LON, foreground);
	}


	/**
	* Modifies the lat/lon of an existing tree widget item.
	*/
	void
	set_lat_lon(
			QTreeWidgetItem *coord_item,
			double lat,
			double lon)
	{
		// Forgo locale printing of number so we can format the string using
		// "StringFormattingUtils.h".
		// FIXME: Do the same but supporting locale.

		// Format the lat/lon into a width of 9 chars with precision 4 digits.
		const unsigned int width = 9;
		const int precision = 4;
		const std::string formatted_lat_string = GPlatesUtils::formatted_double_to_string(
				lat, width, precision);
		const std::string formatted_lon_string = GPlatesUtils::formatted_double_to_string(
				lon, width, precision);

		// The text: What the item displays
		coord_item->setText(COLUMN_LAT, QString::fromStdString(formatted_lat_string));
		coord_item->setText(COLUMN_LON, QString::fromStdString(formatted_lon_string));
	}


	/**
	* Turns a lat,lon pair into a tree widget item ready for insertion
	* into the tree.
	*/
	GPlatesGui::TreeWidgetBuilder::item_handle_type
	create_lat_lon_item(
			GPlatesGui::TreeWidgetBuilder &tree_widget_builder,
			double lat,
			double lon)
	{
		const GPlatesGui::TreeWidgetBuilder::item_handle_type coord_item_handle =
				tree_widget_builder.create_item();

		QTreeWidgetItem *coord_item = tree_widget_builder.get_qtree_widget_item(
				coord_item_handle);

		coord_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		coord_item->setTextAlignment(COLUMN_LAT, Qt::AlignLeft);
		coord_item->setTextAlignment(COLUMN_LON, Qt::AlignLeft);

		unhighlight_lat_lon(coord_item);

		set_lat_lon(coord_item, lat, lon);

		return coord_item_handle;
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
		GPlatesViewOperations::GeometryBuilder *initial_geom_builder,
		GPlatesViewOperations::ActiveGeometryOperation *active_geometry_operation) :
d_coordinates_table(coordinates_table),
d_tree_widget_builder(coordinates_table),
d_current_geometry_builder(NULL),
d_current_geometry_operation(NULL)
{
	if (active_geometry_operation)
	{
		connect_to_active_geometry_operation_signals(active_geometry_operation);
	}

	set_geometry_builder(initial_geom_builder);
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::set_geometry_builder(
		GPlatesViewOperations::GeometryBuilder *geom_builder)
{
	// If the new geometry builder is the same as current one then do nothing.
	if (geom_builder == d_current_geometry_builder)
	{
		return;
	}

	if (d_current_geometry_builder != NULL)
	{
		disconnect_from_current_geometry_builder();
	}

	d_current_geometry_builder = geom_builder;


	if (d_current_geometry_builder != NULL)
	{
		connect_to_current_geometry_builder();

		initialise_table_from_current_geometry_builder();
	}
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::connect_to_active_geometry_operation_signals(
		GPlatesViewOperations::ActiveGeometryOperation *active_geometry_operation)
{
	// Connect to the geometry operation's signals.

	// GeometryOperation has just highlighted a vertex.
	QObject::connect(
			active_geometry_operation,
			SIGNAL(switched_geometry_operation(
					GPlatesViewOperations::GeometryOperation *)),
			this,
			SLOT(switched_geometry_operation(
					GPlatesViewOperations::GeometryOperation *)));
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::connect_to_current_geometry_operation()
{
	// Highlighted point.
	QObject::connect(
			d_current_geometry_operation,
			SIGNAL(highlight_point_in_geometry(
					GPlatesViewOperations::GeometryBuilder *,
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesGui::Colour &)),
			this,
			SLOT(highlight_point_in_geometry(
					GPlatesViewOperations::GeometryBuilder *,
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesGui::Colour &)));

	// No highlighted point.
	QObject::connect(
			d_current_geometry_operation,
			SIGNAL(unhighlight_point_in_geometry(
					GPlatesViewOperations::GeometryBuilder *,
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryBuilder::PointIndex)),
			this,
			SLOT(unhighlight_point_in_geometry(
					GPlatesViewOperations::GeometryBuilder *,
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryBuilder::PointIndex)));
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::disconnect_from_current_geometry_operation()
{
	// Disconnect all signals from the current geometry operation.
	QObject::disconnect(d_current_geometry_operation, 0, this, 0);
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::connect_to_current_geometry_builder()
{
	// Change geometry type in our table.
	QObject::connect(
			d_current_geometry_builder,
			SIGNAL(changed_actual_geometry_type(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryType::Value)),
			this,
			SLOT(change_actual_geometry_type(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex,
					GPlatesViewOperations::GeometryType::Value)));

	// Insert geometry into our table.
	QObject::connect(
			d_current_geometry_builder,
			SIGNAL(inserted_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)),
			this,
			SLOT(insert_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)));

	// Remove geometry into our table.
	QObject::connect(
			d_current_geometry_builder,
			SIGNAL(removed_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)),
			this,
			SLOT(remove_geometry(
					GPlatesViewOperations::GeometryBuilder::GeometryIndex)));

	// Insert point into a geometry in our table.
	QObject::connect(
			d_current_geometry_builder,
			SIGNAL(inserted_point_into_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &)),
			this,
			SLOT(insert_point_into_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &)));

	// Remove point from a geometry in our table.
	QObject::connect(
			d_current_geometry_builder,
			SIGNAL(removed_point_from_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex)),
			this,
			SLOT(remove_point_from_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex)));

	// Moved point in a geometry in our table.
	QObject::connect(
			d_current_geometry_builder,
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
	QObject::disconnect(d_current_geometry_builder, 0, this, 0);
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::initialise_table_from_current_geometry_builder()
{
	//
	// First remove any items we've filled in so far.
	//

	destroy_top_level_items(d_tree_widget_builder);

	//
	// Then add an item for each internal geometry in the current GeometryBuilder.
	//

	GPlatesViewOperations::GeometryBuilder::GeometryIndex insert_child_index;
	for (insert_child_index = 0;
		insert_child_index < d_current_geometry_builder->get_num_geometries();
		++insert_child_index)
	{
		insert_geometry(insert_child_index);
	}
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::switched_geometry_operation(
		GPlatesViewOperations::GeometryOperation *geometry_operation)
{
	// If the new geometry operation is the same as current one then do nothing.
	if (geometry_operation == d_current_geometry_operation)
	{
		return;
	}

	if (d_current_geometry_operation != NULL)
	{
		disconnect_from_current_geometry_operation();
	}

	d_current_geometry_operation = geometry_operation;


	if (d_current_geometry_operation != NULL)
	{
		connect_to_current_geometry_operation();
	}
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::highlight_point_in_geometry(
		GPlatesViewOperations::GeometryBuilder *,
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesGui::Colour &highlight_colour)
{
	QTreeWidgetItem *coord_item = get_coord_item(geometry_index, point_index);

	highlight_lat_lon(coord_item, highlight_colour);

	// Scroll to show the user the highlighted point.
	// We can call this function now since we know the QTreeWidgetItem is currently
	// connected to the QTreeWidget.
	d_coordinates_table->scrollToItem(coord_item);
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::unhighlight_point_in_geometry(
		GPlatesViewOperations::GeometryBuilder *,
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index)
{
	QTreeWidgetItem *coord_item = get_coord_item(geometry_index, point_index);

	unhighlight_lat_lon(coord_item);
}


void
GPlatesQtWidgets::LatLonCoordinatesTable::change_actual_geometry_type(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryType::Value geometry_type)
{
	GPlatesGlobal::Assert(
		boost::numeric_cast<unsigned int>(geometry_index) <
				get_num_top_level_items(d_tree_widget_builder),
		GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	const QString label = get_geometry_type_text(geometry_type);

	QTreeWidgetItem *geom_item = get_child_qtree_widget_item(d_tree_widget_builder,
			d_tree_widget_builder.get_root_handle(), geometry_index);
	geom_item->setText(0, label);

#if 0
	// Update the QTreeWidget with our changes - this isn't really needed
	// since we only need to update if we've inserted/added an item.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
#endif
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::insert_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index)
{
	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(geometry_index) <=
					get_num_top_level_items(d_tree_widget_builder),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	// Get actual type of geometry.
	const GPlatesViewOperations::GeometryType::Value geom_type =
		d_current_geometry_builder->get_actual_type_of_geometry(geometry_index);

	// Get geometry type text.
	const QString geom_type_text = get_geometry_type_text(geom_type);

	// Create top-level tree widget item corresponding to inserted geometry.
	const GPlatesGui::TreeWidgetBuilder::item_handle_type geometry_item_handle =
			create_geometry_item(d_tree_widget_builder, geom_type_text);

	// Insert geometry into tree.
	insert_top_level_item(d_tree_widget_builder, geometry_item_handle, geometry_index);

	//
	// If inserted geometry contains any points then add them also.
	//
	const unsigned int num_points_in_geom =
		d_current_geometry_builder->get_num_points_in_geometry(geometry_index);

	// Iterate through all points in inserted geometry.
	for (unsigned int point_index = 0;
		point_index < num_points_in_geom;
		++point_index)
	{
		// Get point in inserted geometry.
		const GPlatesMaths::PointOnSphere &point =
			d_current_geometry_builder->get_geometry_point(geometry_index, point_index);

		// Insert point into our table.
		insert_point_into_geometry(geometry_index, point_index, point);
	}

	// Update the QTreeWidget with our changes.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::remove_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index)
{
	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(geometry_index) <
					get_num_top_level_items(d_tree_widget_builder),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	// Delete top-level tree widget item corresponding to removed geometry.
	d_tree_widget_builder.destroy_item(
			d_tree_widget_builder.get_child_item_handle(
					d_tree_widget_builder.get_root_handle(), geometry_index));

	// If removed geometry contains points then it doesn't matter since
	// deleting parent will also delete its children.

#if 0
	// Update the QTreeWidget with our changes - this isn't really needed
	// since we only need to update if we've inserted/added an item.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
#endif
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::insert_point_into_current_geometry(
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_globe)
{
	// Get index of current geometry.
	const GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index =
		d_current_geometry_builder->get_current_geometry_index();

	insert_point_into_geometry(geometry_index, point_index, oriented_pos_on_globe);

	// Update the QTreeWidget with our changes.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::insert_point_into_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_globe)
{
	// Figure out which 'geometry' QTreeWidgetItem is the one where we need to add
	// this coordinate.

	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(geometry_index) <
					get_num_top_level_items(d_tree_widget_builder),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	const GPlatesMaths::LatLonPoint& lat_lon_point =
		GPlatesMaths::make_lat_lon_point(oriented_pos_on_globe);

	// Create the 'coordinate' QTreeWidgetItem and add it.
	const GPlatesGui::TreeWidgetBuilder::item_handle_type coord_item_handle = create_lat_lon_item(
			d_tree_widget_builder, lat_lon_point.latitude(), lat_lon_point.longitude());

	const GPlatesGui::TreeWidgetBuilder::item_handle_type geom_item_handle =
			get_top_level_item_handle(d_tree_widget_builder, geometry_index);

	d_tree_widget_builder.insert_child(geom_item_handle, coord_item_handle, point_index);

	const GPlatesViewOperations::GeometryType::Value geom_type =
		d_current_geometry_builder->get_actual_type_of_current_geometry();
	const QString label = get_geometry_type_text(geom_type);

	QTreeWidgetItem *geom_item = d_tree_widget_builder.get_qtree_widget_item(geom_item_handle);
	geom_item->setText(0, label);

	// Scroll to show the user the point they just added.
	// Call function later when QTreeWidgetItem is connected to QTreeWidget.
	d_tree_widget_builder.add_function(coord_item_handle,
			boost::bind(&QTreeWidget::scrollToItem,
					_2, // The QTreeWidget - in this case 'd_coordinates_table'
					_1, // The QTreeWidgetItem - in this case 'coord_item'
					QAbstractItemView::EnsureVisible));
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::move_point_in_current_geometry(
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
		const GPlatesMaths::PointOnSphere &new_oriented_pos_on_globe)
{
	// Get index of current geometry.
	const GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index =
		d_current_geometry_builder->get_current_geometry_index();

	// Figure out which 'geometry' QTreeWidgetItem is the one where we need to
	// modify this coordinate.

	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(geometry_index) <
					get_num_top_level_items(d_tree_widget_builder),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	const GPlatesMaths::LatLonPoint& lat_lon_point =
		GPlatesMaths::make_lat_lon_point(new_oriented_pos_on_globe);

	const GPlatesGui::TreeWidgetBuilder::item_handle_type geom_item_handle =
			get_top_level_item_handle(d_tree_widget_builder, geometry_index);

	const GPlatesGui::TreeWidgetBuilder::item_handle_type coord_item_handle =
			d_tree_widget_builder.get_child_item_handle(geom_item_handle, point_index);
	QTreeWidgetItem *coord_item = d_tree_widget_builder.get_qtree_widget_item(coord_item_handle);

	// Change the latitude and longitude.
	set_lat_lon(coord_item, lat_lon_point.latitude(), lat_lon_point.longitude());

#if 0
	// Update the QTreeWidget with our changes - this isn't really needed
	// since we only need to update if we've inserted/added an item.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
#endif
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::remove_point_from_current_geometry(
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index)
{
	// Get index of current geometry.
	const GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index =
		d_current_geometry_builder->get_current_geometry_index();

	remove_point_from_geometry(geometry_index, point_index);

#if 0
	// Update the QTreeWidget with our changes - this isn't really needed
	// since we only need to update if we've inserted/added an item.
	d_tree_widget_builder.update_qtree_widget_with_added_or_inserted_items();
#endif
}

void
GPlatesQtWidgets::LatLonCoordinatesTable::remove_point_from_geometry(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index)
{
	// Figure out which 'geometry' QTreeWidgetItem is the one where we need to add
	// this coordinate.

	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(geometry_index) <
					get_num_top_level_items(d_tree_widget_builder),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	const GPlatesGui::TreeWidgetBuilder::item_handle_type geom_item_handle =
			get_top_level_item_handle(d_tree_widget_builder, geometry_index);

	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(point_index) <
					d_tree_widget_builder.get_num_children(geom_item_handle),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	const GPlatesGui::TreeWidgetBuilder::item_handle_type coord_item_handle =
			d_tree_widget_builder.get_child_item_handle(geom_item_handle, point_index);

	d_tree_widget_builder.destroy_item(coord_item_handle);
}


QTreeWidgetItem *
GPlatesQtWidgets::LatLonCoordinatesTable::get_coord_item(
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
		GPlatesViewOperations::GeometryBuilder::PointIndex point_index)
{
	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(geometry_index) <
					get_num_top_level_items(d_tree_widget_builder),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	const GPlatesGui::TreeWidgetBuilder::item_handle_type geom_item_handle =
			get_top_level_item_handle(d_tree_widget_builder, geometry_index);

	GPlatesGlobal::Assert(
			boost::numeric_cast<unsigned int>(point_index) <
					d_tree_widget_builder.get_num_children(geom_item_handle),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	const GPlatesGui::TreeWidgetBuilder::item_handle_type coord_item_handle =
			d_tree_widget_builder.get_child_item_handle(geom_item_handle, point_index);

	return d_tree_widget_builder.get_qtree_widget_item(coord_item_handle);
}
