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

#include <QtGlobal>
#include <QDebug>
#include <QtAlgorithms>
#include <QLocale>
#include <QHeaderView>
#include <QTreeWidget>
#include <QUndoStack>
#include <QUndoCommand>
#include <QToolButton>
#include <QList>
#include "DigitisationWidget.h"

#include "ExportCoordinatesDialog.h"


namespace
{
	enum LatLonColumnLayout
	{
		COLUMN_LAT, COLUMN_LON
	};
	

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
		item->setText(COLUMN_LAT, locale.toString(lat));
		item->setText(COLUMN_LON, locale.toString(lon));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		
		return item;
	}
	
	/**
	 * Creates the (probably hidden) top-level item used to distinguish
	 * between parts of multi-geometries and polygon innards.
	 */
	QTreeWidgetItem *
	add_geometry_item(
			QTreeWidget *widget,
			const QString &label)
	{
		static const QBrush background(Qt::darkGray);
		static const QBrush foreground(Qt::white);
		
		// We cannot use the "Span Columns" trick unless the item is first added to the
		// QTreeWidget.
		QTreeWidgetItem *root = new QTreeWidgetItem(widget);
		root->setText(0, label);
		root->setBackground(0, background);
		root->setForeground(0, foreground);
		root->setFirstColumnSpanned(true);
		root->setExpanded(true);
		
		return root;
	}
	
	
	/**
	 * Determines what fragment of geometry the top-level tree widget item
	 * would become, given the current configuration of the DigitisationWidget
	 * and the position and number of children in this top-level item.
	 */
	QString
	calculate_label_for_item(
			GPlatesQtWidgets::DigitisationWidget::GeometryType target_geom_type,
			int position,
			QTreeWidgetItem *item)
	{
		QString label;
		// Pick a sensible default.
		switch (target_geom_type)
		{
		default:
				label = QObject::tr("<Error: unknown GeometryType>");
				break;
		case GPlatesQtWidgets::DigitisationWidget::POLYLINE:
				label = "gml:LineString";
				break;
		case GPlatesQtWidgets::DigitisationWidget::MULTIPOINT:
				label = "gml:MultiPoint";
				break;
		case GPlatesQtWidgets::DigitisationWidget::POLYGON:
				label = "gml:Polygon";
				break;
		}
		
		// Override that default for particular edge cases.
		int children = item->childCount();
		if (children == 0) {
			label = "";
		} else if (children == 1) {
			label = "gml:Point";
		} else if (children == 2 && target_geom_type == GPlatesQtWidgets::DigitisationWidget::POLYGON) {
			label = "gml:LineString";
		}
		
		// Digitising a Polygon gives special meaning to the first entry.
		if (target_geom_type == GPlatesQtWidgets::DigitisationWidget::POLYGON) {
			if (position == 0) {
				label = QObject::tr("exterior: %1").arg(label);
			} else {
				label = QObject::tr("interior: %1").arg(label);
			}
		}
		
		return label;
	}
}



namespace GPlatesUndoCommands
{
	class DigitisationAddPoint :
			public QUndoCommand
	{
	public:
		DigitisationAddPoint(
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget,
				double lat,
				double lon,
				QTreeWidgetItem *geometry):
			d_digitisation_widget(digitisation_widget),
			d_lat(lat),
			d_lon(lon),
			d_geometry_ptr(geometry)
		{
			setText(QObject::tr("add point"));
		}
		
		virtual
		void
		redo()
		{
			qDebug(Q_FUNC_INFO);
			QTreeWidgetItem *item = create_lat_lon_item(d_lat, d_lon);
			d_geometry_ptr->addChild(item);

			// Scroll to show the user the point they just added.
			d_digitisation_widget.coordinates_table()->scrollToItem(item);
			// Update labels.
			d_digitisation_widget.update_table_labels();
		}
		
		virtual
		void
		undo()
		{
			qDebug(Q_FUNC_INFO);
			if (d_geometry_ptr->childCount() > 0) {
				QTreeWidgetItem *item = d_geometry_ptr->takeChild(d_geometry_ptr->childCount() - 1);
				delete item;
			}
			// FIXME: Else, throw something - or Assert.
			// FIXME: If you clear the table and reset it, this obviously segfaults,
			// as d_geometry_ptr no longer exists (a brand new toplevel 'geometry' item has
			// been created).

			// Update labels.
			d_digitisation_widget.update_table_labels();
		}
		
	private:
		GPlatesQtWidgets::DigitisationWidget &d_digitisation_widget;
		double d_lat;
		double d_lon;
		// FIXME: I'm worried about rewinding the stack so far that the 'Geometry' QTWI gets deleted,
		// and then forwarded so it gets recreated (but at a different address!!)
		// ... might be possible to structure the undocommand for that particular action
		// so that rather then deallocating the QTreeWidgetItem* on undo(), it just stores it.
		// Obviously, then you'll have to implement a smart destructor for the undocommand.
		QTreeWidgetItem *d_geometry_ptr;
	};
	
	
	class DigitisationCancelGeometry :
			public QUndoCommand
	{
	public:
		DigitisationCancelGeometry(
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget,
				QTreeWidgetItem *geometry):
			d_digitisation_widget(digitisation_widget),
			d_geometry_ptr(geometry)
		{
			setText(QObject::tr("clear geometry"));
		}
		
		virtual
		~DigitisationCancelGeometry()
		{
			// If the undo stack should delete this QUndoCommand, we must
			// make sure we delete any QTreeWidgetItems that have been temporarily
			// assigned to us.
			qDebug(Q_FUNC_INFO);
			qDeleteAll(d_removed_items);
		}
		
		virtual
		void
		redo()
		{
			qDebug(Q_FUNC_INFO);
			// When cancelling the geometry, the QTreeWidgetItems are not
			// immediately deleted - they are kept alive by the undo stack.
			d_removed_items = d_geometry_ptr->takeChildren();

			// Update labels.
			d_digitisation_widget.update_table_labels();
		}
		
		virtual
		void
		undo()
		{
			qDebug(Q_FUNC_INFO);
			// Reassign the children to the tree.
			d_geometry_ptr->addChildren(d_removed_items);
			d_removed_items.clear();

			// Update labels.
			d_digitisation_widget.update_table_labels();
		}
		
	private:
		GPlatesQtWidgets::DigitisationWidget &d_digitisation_widget;
		QTreeWidgetItem *d_geometry_ptr;
		QList<QTreeWidgetItem *> d_removed_items;
	};
}



GPlatesQtWidgets::DigitisationWidget::DigitisationWidget(
		QWidget *parent_):
	QWidget(parent_),
	d_export_coordinates_dialog(new ExportCoordinatesDialog(this))
{
	setupUi(this);
	
	// Set up the header of the coordinates widget.
	treewidget_coordinates->header()->setResizeMode(QHeaderView::Stretch);
	
	// Set up the geometry type combo box.
	combobox_geometry_type->addItem(tr("Polyline"), QVariant(POLYLINE));
	combobox_geometry_type->addItem(tr("Multi Point"), QVariant(MULTIPOINT));
	combobox_geometry_type->addItem(tr("Polygon"), QVariant(POLYGON));
	QObject::connect(combobox_geometry_type, SIGNAL(currentIndexChanged(int)),
			this, SLOT(change_geometry_type(int)));
	
	// The default OK and Cancel buttons are okay, but we want custom labels for buttons
	// which will give us accept() and reject() signals.
	buttonbox_create->addButton(tr("Create..."), QDialogButtonBox::AcceptRole);
	buttonbox_create->addButton(tr("Clear"), QDialogButtonBox::RejectRole);
	QObject::connect(buttonbox_create, SIGNAL(accepted()),
			this, SLOT(create_geometry()));
	QObject::connect(buttonbox_create, SIGNAL(rejected()),
			this, SLOT(cancel_geometry()));

	// Export dialog.
	QObject::connect(button_export_coordinates, SIGNAL(clicked()),
			this, SLOT(handle_export()));
	
	// Set up some (temporary) undo/redo buttons!
	QToolButton *tool1 = new QToolButton(this);
	tool1->setDefaultAction(d_undo_stack.createUndoAction(this));
	layout()->addWidget(tool1);
	QToolButton *tool2 = new QToolButton(this);
	tool2->setDefaultAction(d_undo_stack.createRedoAction(this));
	layout()->addWidget(tool2);
	
	// Get everything else ready that may need to be set up more than once.
	initialise_geometry(POLYLINE);
}


void
GPlatesQtWidgets::DigitisationWidget::clear_widget()
{
	treewidget_coordinates->clear();
}


void
GPlatesQtWidgets::DigitisationWidget::initialise_geometry(
		GeometryType geom_type)
{
	// Make a default top-level geometry item (for now)
	// FIXME: We will need to keep track of these, so that we can change them later.
	set_geometry_combobox(geom_type);
	// FIXME: should be a lookup table, probably.
	clear_widget();
	
	d_geometry_type = geom_type;
	
	add_geometry_item(treewidget_coordinates, "");
}


void
GPlatesQtWidgets::DigitisationWidget::change_geometry_type(
		GeometryType geom_type)
{
	qDebug(Q_FUNC_INFO);
	// FIXME: Dumb stub for now. This fn will need to be clever, depending on what
	// we're going from and what we're going to.
	// FIXME: In fact, this needs to be another QUndoCommand. Especially if we're going
	// from gml:Polygon to gml:LineString and back.
	initialise_geometry(geom_type);
}


void
GPlatesQtWidgets::DigitisationWidget::handle_export()
{
	// FIXME: Feed the Export dialog the GeometryOnSphere you've set up for the
	// current points. You -have- set up a GeometryOnSphere for the current points,
	// haven't you?
	
	d_export_coordinates_dialog->exec();
}


void
GPlatesQtWidgets::DigitisationWidget::create_geometry()
{
	// FIXME: Actually construct something!
	// FIXME: Undo!
	
	// Then, when we're all done, reset the widget for new input.
	initialise_geometry(POLYLINE);
}


void
GPlatesQtWidgets::DigitisationWidget::cancel_geometry()
{
	// Clear all points from the current geometry.
	// FIXME: And how do you suppose we deal with multi-geometries?
	QTreeWidgetItem *target_geometry = treewidget_coordinates->topLevelItem(0);
	d_undo_stack.push(new GPlatesUndoCommands::DigitisationCancelGeometry(
			*this, target_geometry));
}


void
GPlatesQtWidgets::DigitisationWidget::append_point_to_geometry(
		double lat,
		double lon,
		QTreeWidgetItem *target_geometry)
{
	// Make a QTreeWidgetItem for it,
	// Append it to a root item (which may be hidden)
	// AND DON'T FORGET ABOUT UNDO/REDO!!!!!!!!!!~~
	if (target_geometry == NULL) {
		target_geometry = treewidget_coordinates->topLevelItem(0);
	}
	d_undo_stack.push(new GPlatesUndoCommands::DigitisationAddPoint(
			*this, lat, lon, target_geometry));
}


void
GPlatesQtWidgets::DigitisationWidget::update_table_labels()
{
	// For each label (top-level QTreeWidgetItem) in the table,
	// determine what (piece of) geometry it will turn into when
	// the user hits Create.
	QTreeWidgetItem *root = treewidget_coordinates->invisibleRootItem();
	int num_children = root->childCount();
	for (int i = 0; i < num_children; ++i) {
		QTreeWidgetItem *item = root->child(i);
		QString label = calculate_label_for_item(d_geometry_type, i, item);
		item->setText(0, label);
	}
}


void
GPlatesQtWidgets::DigitisationWidget::change_geometry_type(
		int idx)
{
	QVariant var = combobox_geometry_type->itemData(idx);
	GeometryType geom_type = GeometryType(var.toInt());
	change_geometry_type(geom_type);
}


void
GPlatesQtWidgets::DigitisationWidget::set_geometry_combobox(
		GeometryType geom_type)
{
	int idx = combobox_geometry_type->findData(QVariant(geom_type));
	if (idx >= 0) {
		combobox_geometry_type->setCurrentIndex(idx);
	}
}


