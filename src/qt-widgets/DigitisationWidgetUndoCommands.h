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

#ifndef GPLATES_QTWIDGETS_DIGITISATIONWIDGETUNDOCOMMANDS_H
#define GPLATES_QTWIDGETS_DIGITISATIONWIDGETUNDOCOMMANDS_H

#include <QtGlobal>
#include <QtAlgorithms>
#include <QDebug>
#include <QUndoCommand>
#include <QList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLocale>
#include "DigitisationWidget.h"
#include "global/GPlatesAssert.h"
#include "DigitisationUndoParadoxException.h"


namespace
{
	/**
	 * Turns a lat,lon pair into a tree widget item ready for insertion
	 * into the tree - note that you MUST insert it somewhere, so that Qt
	 * will manage the memory. If you wish to discard it before adding
	 * it to the tree, you'll have to delete the memory yourself.
	 */
	QTreeWidgetItem *
	create_lat_lon_item(
			double lat,
			double lon)
	{
		static QLocale locale;
		
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		// The text: What the item displays
		item->setText(GPlatesQtWidgets::DigitisationWidget::COLUMN_LAT, locale.toString(lat));
		item->setText(GPlatesQtWidgets::DigitisationWidget::COLUMN_LON, locale.toString(lon));
		// The data: A convenient means of storing the double so we won't have to
		// parse the text to get our latlongs back. Note we could have set this
		// as Qt::DisplayRole and got the same effect as setText; but we might want
		// to format the display differently later (e.g. precision, padding).
		item->setData(GPlatesQtWidgets::DigitisationWidget::COLUMN_LAT, Qt::EditRole, QVariant(lat));
		item->setData(GPlatesQtWidgets::DigitisationWidget::COLUMN_LON, Qt::EditRole, QVariant(lon));
		
		return item;
	}


	/**
	 * Creates the top-level QTreeWidgetItem used to distinguish
	 * between parts of multi-geometries and polygon innards.
	 */
	QTreeWidgetItem *
	add_geometry_item(
			QTreeWidget *widget,
			const QString &label = QString())
	{
		static const QBrush background(Qt::darkGray);
		static const QBrush foreground(Qt::white);
		
		// We cannot use the "Span Columns" trick unless the item is first added to the
		// QTreeWidget.
		QTreeWidgetItem *geom_item = new QTreeWidgetItem(widget);
		geom_item->setText(0, label);
		geom_item->setBackground(0, background);
		geom_item->setForeground(0, foreground);
		geom_item->setFirstColumnSpanned(true);
		geom_item->setExpanded(true);
		
		return geom_item;
	}
	
	
	/**
	 * Calls clone() on all the 'coordinate' QTreeWidgetItems found in the
	 * given top-level 'geometry' item. Appends them to the given list, which should
	 * be created by the caller.
	 * 
	 * The QTreeWidgetItems created will not belong to a parent, and you should
	 * ensure that their memory is managed by Qt by adding them to one, or
	 * deleting them all.
	 *
	 * This function is used when converting between geometry types in the widget.
	 */
	void
	clone_geometry_item_coordinates(
			QTreeWidgetItem *geom_item,
			QList<QTreeWidgetItem *> &list)
	{
		int children = geom_item->childCount();
		for (int i = 0; i < children; ++i) {
			list.append(geom_item->child(i)->clone());
		}
	}
}



namespace GPlatesUndoCommands
{
	/**
	 * Undo Command for adding a point to the table in the Digitisation Widget.
	 * The inverse of this command is to remove the last item added (and delete
	 * the memory used by that QTreeWidgetItem).
	 *
	 * FIXME: This command currently assumes that we wish to append a point
	 * to the end of the last fragment of geometry available (i.e. the last
	 * bit of a Multi-PolylineOnSphere, or the most recently-added interior of
	 * a polygon.)
	 * If the DigitisationWidget is updated to allow users to append points
	 * to previous parts of geometry (not super-essential, admittedly), we'll
	 * have to use an index-based approach, or risk Undo/Redo problems.
	 */
	class DigitisationAddPoint :
			public QUndoCommand
	{
	public:
		DigitisationAddPoint(
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget,
				double lat,
				double lon):
			d_digitisation_widget(&digitisation_widget),
			d_lat(lat),
			d_lon(lon)
		{
			setText(QObject::tr("add point"));
		}
		
		virtual
		void
		redo()
		{
			QTreeWidgetItem *root = d_digitisation_widget->coordinates_table()->invisibleRootItem();
			// Figure out which 'geometry' QTreeWidgetItem is the one where we need to add
			// this coordinate.
			QTreeWidgetItem *geom_item = NULL;
			if (root->childCount() > 0) {
				// Pick out the last geometry item in the table.
				geom_item = root->child(root->childCount() - 1);
			} else {
				// No geometry items have been added to the table yet - make one.
				geom_item = add_geometry_item(d_digitisation_widget->coordinates_table());
			}
			
			// Create the 'coordinate' QTreeWidgetItem and add it.
			QTreeWidgetItem *coord_item = create_lat_lon_item(d_lat, d_lon);
			geom_item->addChild(coord_item);

			// Scroll to show the user the point they just added.
			d_digitisation_widget->coordinates_table()->scrollToItem(geom_item);

			// Update labels and the displayed geometry.
			d_digitisation_widget->update_table_labels();
			d_digitisation_widget->update_geometry();
		}
		
		virtual
		void
		undo()
		{
			QTreeWidgetItem *root = d_digitisation_widget->coordinates_table()->invisibleRootItem();

			// Figure out which 'geometry' QTreeWidgetItem is the one where we added
			// our coordinate previously.
			
			// Assertion: The table is not empty, there is a geometry item to be found.
			GPlatesGlobal::Assert(root->childCount() > 0,
					GPlatesQtWidgets::DigitisationUndoParadoxException(__FILE__, __LINE__));
			
			// Pick out the last geometry item in the table.
			QTreeWidgetItem *geom_item = root->child(root->childCount() - 1);

			// Locate the 'coordinate' QTreeWidgetItem that was added, and delete it.

			// Assertion: There is at least one coordinate item to be found. We put it
			// there just moments ago when you called our redo(). Remember?
			GPlatesGlobal::Assert(geom_item->childCount() > 0,
					GPlatesQtWidgets::DigitisationUndoParadoxException(__FILE__, __LINE__));

			// Kill the last child.
			QTreeWidgetItem *coord_item = geom_item->takeChild(geom_item->childCount() - 1);
			delete coord_item;
				
			// Special case; if this leaves us with an empty geometry item, we should
			// get rid of that one, too, just to keep things consistent. It doesn't harm
			// anything, but just doesn't quite look right if you undo/redo the right way.
			if (geom_item->childCount() == 0) {
				root->removeChild(geom_item);
				delete geom_item;
			}
				
			// Update labels and the displayed geometry.
			d_digitisation_widget->update_table_labels();
			d_digitisation_widget->update_geometry();
		}
		
	private:
		/**
		 * A const pointer to our DigitisationWidget, to emphasise that although we
		 * don't 'own' the DigitisationWidget, the pointer should never ever be reassigned
		 * in this QUndoCommand, or the space-time-undo continuum will get screwed up.
		 */
		GPlatesQtWidgets::DigitisationWidget *const d_digitisation_widget;

		double d_lat;
		double d_lon;
	};
	
	
	/**
	 * Undo Command for clearing the table of geometry being digitised and starting over.
	 * The inverse of this command restores the table items that were 'deleted'.
	 *
	 * For Undo/Redo to work properly, the QTreeWidgetItems are removed from the
	 * table but NOT deleted when this command activates. The command takes ownership
	 * of the QTreeWidgetItems that were removed. If it is undone, it gives them back
	 * to the QTreeWidget. If this QUndoCommand becomes unreachable, it will delete
	 * the memory used by any QTreeWidgetItems it was taking care of.
	 */
	class DigitisationClearGeometry :
			public QUndoCommand
	{
	public:
		DigitisationClearGeometry(
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget):
			d_digitisation_widget(&digitisation_widget)
		{
			setText(QObject::tr("clear geometry"));
		}
		
		/**
		 * If the undo stack should delete this QUndoCommand, we must
		 * make sure we delete any QTreeWidgetItems that have been temporarily
		 * assigned to us.
		 */
		virtual
		~DigitisationClearGeometry()
		{
			qDeleteAll(d_removed_items);
		}
		
		virtual
		void
		redo()
		{
			QTreeWidgetItem *root = d_digitisation_widget->coordinates_table()->invisibleRootItem();
			// When clearing the geometry, the QTreeWidgetItems are not
			// immediately deleted - they are kept alive by the undo stack.
			d_removed_items = root->takeChildren();

			// Update labels and the displayed geometry.
			d_digitisation_widget->update_table_labels();
			d_digitisation_widget->update_geometry();
		}
		
		virtual
		void
		undo()
		{
			QTreeWidgetItem *root = d_digitisation_widget->coordinates_table()->invisibleRootItem();
			// Reassign the children to the tree.
			root->addChildren(d_removed_items);
			d_removed_items.clear();

			// Update labels and the displayed geometry.
			d_digitisation_widget->update_table_labels();
			d_digitisation_widget->update_geometry();
		}
		
	private:
		/**
		 * A const pointer to our DigitisationWidget, to emphasise that although we
		 * don't 'own' the DigitisationWidget, the pointer should never ever be reassigned
		 * in this QUndoCommand, or the space-time-undo continuum will get screwed up.
		 */
		GPlatesQtWidgets::DigitisationWidget *const d_digitisation_widget;

		QList<QTreeWidgetItem *> d_removed_items;
	};


	/**
	 * Undo Command for readjusting the geometry being digitised to match
	 * the user's intention.
	 * The inverse of this command restores the table items that were 'deleted'.
	 *
	 * Note that this is a potentially 'lossy' operation - for example, converting
	 * from a PolygonOnSphere (with multiple interiors) to a MultiPointOnSphere and
	 * back again. I've decided against making the MultiPoint table items 'remember'
	 * that they once were separated in places (for the sake of multiple LineStrings
	 * or Polygons), because this would make the create_multipoint_on_sphere needlessly
	 * complex.
	 * 
	 * Instead, if the user feels he/she absolutely MUST toy with the
	 * Polyline/MultiPoint/Polygon digitisation canvas tool buttons, we will attempt
	 * to preserve the coordinates but no guarantees are made about the logical breaks
	 * between them. If this makes the user upset, they'll at least have the Undo
	 * command to save them and restore the original, non-mangled state of their
	 * digitised geometry.
	 *
	 * For Undo/Redo to work properly, the QTreeWidgetItems are removed from the
	 * table but NOT deleted when this command activates. The command takes ownership
	 * of the QTreeWidgetItems that were removed. If it is undone, it gives them back
	 * to the QTreeWidget. If this QUndoCommand becomes unreachable, it will delete
	 * the memory used by any QTreeWidgetItems it was taking care of.
	 */
	class DigitisationChangeGeometryType :
			public QUndoCommand
	{
	public:
		DigitisationChangeGeometryType(
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget,
				GPlatesQtWidgets::DigitisationWidget::GeometryType from_geom_type,
				GPlatesQtWidgets::DigitisationWidget::GeometryType to_geom_type):
			d_digitisation_widget(&digitisation_widget),
			d_from_geom_type(from_geom_type),
			d_to_geom_type(to_geom_type)
		{
			setText(QObject::tr("change geometry type"));
		}
		
		/**
		 * If the undo stack should delete this QUndoCommand, we must
		 * make sure we delete any QTreeWidgetItems that have been temporarily
		 * assigned to us.
		 */
		virtual
		~DigitisationChangeGeometryType()
		{
			qDeleteAll(d_removed_items);
		}
		
		virtual
		void
		redo()
		{
			QTreeWidgetItem *root = d_digitisation_widget->coordinates_table()->invisibleRootItem();

			// When changing the geometry type, the original QTreeWidgetItems are not
			// immediately deleted - they are kept alive by the undo stack.
			d_removed_items = root->takeChildren();
			
			// Clone the coordinate items of all parts of the former table.
			// This is a 'lossy' operation, but that's why we have an Undo command for it.
			QList<QTreeWidgetItem *> new_coordinate_items;
			QList<QTreeWidgetItem *>::const_iterator it = d_removed_items.begin();
			QList<QTreeWidgetItem *>::const_iterator end = d_removed_items.end();
			for ( ; it != end; ++it) {
				clone_geometry_item_coordinates(*it, new_coordinate_items);
			}
			
			if ( ! new_coordinate_items.empty()) {
				// Convert the old coordinates to something usable for the new geometry type.
				QTreeWidgetItem *new_geom_item = add_geometry_item(
						d_digitisation_widget->coordinates_table(), "");
				new_geom_item->addChildren(new_coordinate_items);
			}
			
			// Tell the DigitisationWidget what the new desired geometry type is.
			// FIXME: See the definition of set_geometry_type().
			d_digitisation_widget->set_geometry_type(d_to_geom_type);

			// Update labels and the displayed geometry.
			d_digitisation_widget->update_table_labels();
			d_digitisation_widget->update_geometry();
		}
		
		virtual
		void
		undo()
		{
			QTreeWidgetItem *root = d_digitisation_widget->coordinates_table()->invisibleRootItem();
			// FIXME: FOREACH GEOMETRY ITEM
			// Replace whatever might be in the table now with the original items.
			d_digitisation_widget->coordinates_table()->clear();
			// Reassign the children to the tree.
			root->addChildren(d_removed_items);
			d_removed_items.clear();

			// Tell the DigitisationWidget what the old desired geometry type was.
			// FIXME: See the definition of set_geometry_type().
			d_digitisation_widget->set_geometry_type(d_from_geom_type);

			// Update labels and the displayed geometry.
			d_digitisation_widget->update_table_labels();
			d_digitisation_widget->update_geometry();
		}
		
	private:
		/**
		 * A const pointer to our DigitisationWidget, to emphasise that although we
		 * don't 'own' the DigitisationWidget, the pointer should never ever be reassigned
		 * in this QUndoCommand, or the space-time-undo continuum will get screwed up.
		 */
		GPlatesQtWidgets::DigitisationWidget *const d_digitisation_widget;
		
		GPlatesQtWidgets::DigitisationWidget::GeometryType d_from_geom_type;
		GPlatesQtWidgets::DigitisationWidget::GeometryType d_to_geom_type;
		QList<QTreeWidgetItem *> d_removed_items;
	};
}


#endif	// GPLATES_QTWIDGETS_DIGITISATIONWIDGETUNDOCOMMANDS_H
