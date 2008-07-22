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
#include <QMessageBox>
#include <boost/intrusive_ptr.hpp>
#include "DigitisationWidget.h"

#include "ExportCoordinatesDialog.h"
#include "maths/InvalidLatLonException.h"
#include "maths/LatLonPointConversions.h"
#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace
{
	typedef GPlatesMaths::PolylineOnSphere polyline_type;
	typedef GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr_type;
	typedef boost::intrusive_ptr<const GPlatesMaths::GeometryOnSphere> geometry_ptr_type;

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
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		// The text: What the item displays
		item->setText(COLUMN_LAT, locale.toString(lat));
		item->setText(COLUMN_LON, locale.toString(lon));
		// The data: A convenient means of storing the double so we won't have to
		// parse the text to get our latlongs back. Note we could have set this
		// as Qt::DisplayRole and got the same effect as setText; but we might want
		// to format the display differently later (e.g. precision, padding).
		item->setData(COLUMN_LAT, Qt::EditRole, QVariant(lat));
		item->setData(COLUMN_LON, Qt::EditRole, QVariant(lon));
		
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


	/**
	 * Goes through the children of the QTreeWidgetItem geometry-item (i.e. the
	 * points in the table) and attempts to build a vector of PointOnSphere.
	 * 
	 * Invalid points in the table will be skipped over, although due to the nature
	 * of the DigitisationWidget, there really shouldn't be any invalid points to begin
	 * with, since we're getting them from a PointOnSphere in the first place.
	 */
	std::vector<GPlatesMaths::PointOnSphere>
	build_points_from_table_item(
			QTreeWidgetItem *geom_item)
	{
		std::vector<GPlatesMaths::PointOnSphere> points;
		int children = geom_item->childCount();
		points.reserve(children);
		
		// Build a vector of points that we can pass to PolylineOnSphere's validity test.
		for (int i = 0; i < children; ++i) {
			QTreeWidgetItem *child = geom_item->child(i);
			double lat = 0.0;
			double lon = 0.0;
			
			// Pull the lat,lon out of the QTreeWidgetItem that we stored inside it
			// using the Qt::EditRole. This avoids unnecessary parsing of text.
			QVariant lat_var = child->data(COLUMN_LAT, Qt::EditRole);
			bool lat_ok = false;
			lat = lat_var.toDouble(&lat_ok);
			
			QVariant lon_var = child->data(COLUMN_LON, Qt::EditRole);
			bool lon_ok = false;
			lon = lon_var.toDouble(&lon_ok);
			
			// (Attempt to) create a LatLonPoint for the coordinates.
			if (lat_ok && lon_ok) {
				// At this point we have a valid lat,lon - valid as far as doubles are concerned.
				try {
					points.push_back(GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(lat,lon)));
				} catch (GPlatesMaths::InvalidLatLonException &) {
					// FIXME: We really shouldn't be encountering invalid latlongs. How did the
					// user click them?
					qDebug() << "build_points_from_table_item() caught InvalidLatLonException, what the hell?";
				}
			} else {
				// FIXME: If ! lat_ok || ! lon_ok, something is seriously wrong.
				// How did invalid data get in here?
				qDebug() << "build_points_from_table_item() found invalid doubles, what the hell?";
			}
		}
		return points;
	}


	/**
	 * Creates a single PointOnSphere (assuming >0 points are provided).
	 * If you supply more than one point, the others will get ignored.
	 *
	 * Note we are returning a possibly-null intrusive_ptr.
	 */
	geometry_ptr_type
	create_point_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points)
	{
		qDebug(Q_FUNC_INFO);
		
		if (points.size() > 0) {
			return points.front().clone_as_geometry().get();
		} else {
			return NULL;
		}
	}


	/**
	 * @a validity is a reference that should be created by the caller and will
	 * be populated by this function.
	 *
	 * Note we are returning a possibly-null intrusive_ptr.
	 */
	geometry_ptr_type
	create_polyline_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity& validity)
	{
		qDebug(Q_FUNC_INFO);

		// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
		std::pair<
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
				invalid_points;
		
		// Evaluate construction parameter validity, and create.
		validity = polyline_type::evaluate_construction_parameter_validity(
				points, invalid_points);

		// Create the polyline and return it - if we can.
		if (validity == polyline_type::VALID) {
			// Note that create_on_heap gives us a PolylineOnSphere::non_null_ptr_to_const_type,
			// we want to turn this into an intrusive_ptr<const GeometryOnSphere>!
			polyline_ptr_type polyline_ptr = polyline_type::create_on_heap(points);
			return polyline_ptr.get();
		} else {
			qDebug() << "polyline evaluate_construction_parameter_validity," << validity;
			return NULL;
		}
	}


	/**
	 * FIXME: As we have no MultiPoint available, this one is rather simplistic;
	 * it just returns a single PointOnSphere (assuming >0 points are provided).
	 *
	 * Note we are returning a possibly-null intrusive_ptr.
	 */
	geometry_ptr_type
	create_multipoint_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points)
	{
		qDebug(Q_FUNC_INFO);
		return create_point_on_sphere(points);
	}


	/**
	 * Does the work of examining QTreeWidgetItems and the user's intentions
	 * to call upon the appropriate anonymous namespace geometry creation function.
	 *
	 * Note we are returning a possibly-null intrusive_ptr.
	 */
	geometry_ptr_type
	create_geometry_from_table_items(
			QTreeWidgetItem *geom_item,
			GPlatesQtWidgets::DigitisationWidget::GeometryType target_geom_type)
	{
		geometry_ptr_type geometry_ptr = NULL;
		std::vector<GPlatesMaths::PointOnSphere> points;
		GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity validity;
		
		// FIXME: I think... we need some way to add data() to the 'header' QTWIs,
		// so that we can immediately discover which bits are supposed to be polygon exteriors etc.

		switch (target_geom_type)
		{
		default:
				break;
				
		case GPlatesQtWidgets::DigitisationWidget::POLYLINE:
				points = build_points_from_table_item(geom_item);
				qDebug() << "create_geometry_from_table_items():" << points.size() << "points.";

				// FIXME: I'd really like to wrap this up into a function pointer,
				// but to do so I need a way to abstract the ConstructionParameterValidity stuff.
				// Maybe make an exception for all of these 'unable to create geometry' cases?
				// Or a vector of GeometryConstructionProblems?
				// Note: Of course, PolygonOnSphere is going to need a different set of arguments...
				// so a function pointer may not be the right tool for the job here after all.
				if (points.size() == 0) {
					return NULL;
				} else if (points.size() == 1) {
					qDebug() << "create_geometry_from_table_items(): We want a polyline, but let's make a point instead!";
					geometry_ptr = create_point_on_sphere(points);
				} else {
					qDebug() << "create_geometry_from_table_items(): Let's make a polyline!";
					qDebug() << " I have a list of" << points.size() << "points.";
					geometry_ptr = create_polyline_on_sphere(points, validity);
				}
				break;
				
		case GPlatesQtWidgets::DigitisationWidget::MULTIPOINT:
				points = build_points_from_table_item(geom_item);
				qDebug() << "create_geometry_from_table_items():" << points.size() << "points.";

				// FIXME: I'd really like to wrap this up into a function pointer,
				if (points.size() == 0) {
					return NULL;
				} else if (points.size() == 1) {
					qDebug() << "create_geometry_from_table_items(): We want a multipoint, but a point is fine too.";
					geometry_ptr = create_point_on_sphere(points);
				} else {
					qDebug() << "create_geometry_from_table_items(): Let's make a multipoint!";
					geometry_ptr = create_multipoint_on_sphere(points);
				}
				break;
				
		case GPlatesQtWidgets::DigitisationWidget::POLYGON:
				break;
		}
		return geometry_ptr;
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
				QTreeWidgetItem *geometry_item):
			d_digitisation_widget(digitisation_widget),
			d_lat(lat),
			d_lon(lon),
			d_geometry_item(geometry_item)
		{
			setText(QObject::tr("add point"));
		}
		
		virtual
		void
		redo()
		{
			qDebug(Q_FUNC_INFO);
			QTreeWidgetItem *item = create_lat_lon_item(d_lat, d_lon);
			d_geometry_item->addChild(item);

			// Scroll to show the user the point they just added.
			d_digitisation_widget.coordinates_table()->scrollToItem(item);

			// Update labels and the displayed geometry.
			d_digitisation_widget.update_table_labels();
			d_digitisation_widget.update_geometry();
		}
		
		virtual
		void
		undo()
		{
			qDebug(Q_FUNC_INFO);
			if (d_geometry_item->childCount() > 0) {
				QTreeWidgetItem *item = d_geometry_item->takeChild(d_geometry_item->childCount() - 1);
				delete item;
			}
			// FIXME: Else, throw something - or Assert.
			// FIXME: If you clear the table and reset it, this obviously segfaults,
			// as d_geometry_item no longer exists (a brand new toplevel 'geometry' item has
			// been created).

			// Update labels and the displayed geometry.
			d_digitisation_widget.update_table_labels();
			d_digitisation_widget.update_geometry();
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
		QTreeWidgetItem *d_geometry_item;
	};
	
	
	class DigitisationCancelGeometry :
			public QUndoCommand
	{
	public:
		DigitisationCancelGeometry(
				GPlatesQtWidgets::DigitisationWidget &digitisation_widget,
				QTreeWidgetItem *geometry_item):
			d_digitisation_widget(digitisation_widget),
			d_geometry_item(geometry_item)
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
			d_removed_items = d_geometry_item->takeChildren();

			// Update labels and the displayed geometry.
			d_digitisation_widget.update_table_labels();
			d_digitisation_widget.clear_geometry();
		}
		
		virtual
		void
		undo()
		{
			qDebug(Q_FUNC_INFO);
			// Reassign the children to the tree.
			d_geometry_item->addChildren(d_removed_items);
			d_removed_items.clear();

			// Update labels and the displayed geometry.
			d_digitisation_widget.update_table_labels();
			d_digitisation_widget.update_geometry();
		}
		
	private:
		GPlatesQtWidgets::DigitisationWidget &d_digitisation_widget;
		QTreeWidgetItem *d_geometry_item;
		QList<QTreeWidgetItem *> d_removed_items;
	};
}



GPlatesQtWidgets::DigitisationWidget::DigitisationWidget(
		QWidget *parent_):
	QWidget(parent_),
	d_export_coordinates_dialog(new ExportCoordinatesDialog(this)),
	d_geometry_type(GPlatesQtWidgets::DigitisationWidget::POLYLINE),
	d_geometry_ptr(NULL)
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
			this, SLOT(handle_create()));
	QObject::connect(buttonbox_create, SIGNAL(rejected()),
			this, SLOT(handle_cancel()));

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
GPlatesQtWidgets::DigitisationWidget::clear_geometry()
{
	qDebug() << "clear_geometry(): reset d_geometry_ptr to NULL.";
	d_geometry_ptr = NULL;
}

void
GPlatesQtWidgets::DigitisationWidget::update_geometry()
{
	// look at geom type
	// potentially, foreach geom treewidgetitem, possibly recursing for Multi*:
	QTreeWidgetItem *root = treewidget_coordinates->invisibleRootItem();
	int num_children = root->childCount();
	for (int i = 0; i < num_children; ++i) {
		//   build vector of pointonsphere from the lat,lon
				// std::vector<GPlatesMaths::PointOnSphere> build_points_from_table_item(*geom_item)
		//   feed that into a create_xxxx function
				// geometry_ptr_type create_polyline_on_sphere(std::vector<GPlatesMaths::PointOnSphere> &points,
				// GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity& validity)
		QTreeWidgetItem *item = root->child(i);
		geometry_ptr_type geometry_ptr = create_geometry_from_table_items(
				item, d_geometry_type);

		// FIXME: Move the above into a MultiGeometry case of create_geometry_from_table_items and recurse,
		// move the below to the bottom of this function. as it only handles single-linestring cases this way.
		
		// set that as d_geometry_ptr
		if (geometry_ptr != NULL) {
			// No problems, set the new geometry and render it on screen.
			qDebug() << "update_geometry(): Successfully made new geometry!";
			d_geometry_ptr = geometry_ptr;
		} else {
			qDebug() << "update_geometry(): Failed to make new geometry!";
		}
	}
}


void
GPlatesQtWidgets::DigitisationWidget::clear_widget()
{
	treewidget_coordinates->clear();
	d_geometry_ptr = NULL;
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
GPlatesQtWidgets::DigitisationWidget::handle_create()
{
	// FIXME: Actually construct something!
	// FIXME: Undo!
	
	// Then, when we're all done, reset the widget for new input.
	initialise_geometry(POLYLINE);
}


void
GPlatesQtWidgets::DigitisationWidget::handle_cancel()
{
	// Clear all points from the current geometry.
	// FIXME: Assuming single-part geometries for now.
	QTreeWidgetItem *target_geometry_item = treewidget_coordinates->topLevelItem(0);
	d_undo_stack.push(new GPlatesUndoCommands::DigitisationCancelGeometry(
			*this, target_geometry_item));
}


void
GPlatesQtWidgets::DigitisationWidget::handle_export()
{
	// Feed the Export dialog the GeometryOnSphere you've set up for the current
	// points. You -have- set up a GeometryOnSphere for the current points,
	// haven't you?
	if (d_geometry_ptr != NULL) {
		// Since the ExportCoordinatesDialog wants a guarentee of a non_null ptr,
		// we'll have to make one.
		// FIXME: it'd be nice if PolylineOnSphere::get_non_null_pointer() was virtual
		// and available in GeometryOnSphere.
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_non_null_ptr(
				d_geometry_ptr.get(), GPlatesUtils::NullIntrusivePointerHandler());
		d_export_coordinates_dialog->set_geometry_and_display(geometry_non_null_ptr);

	} else {
		QMessageBox::warning(this, tr("No geometry to export"),
				tr("There is no valid geometry to export."),
				QMessageBox::Ok);
	}
}


void
GPlatesQtWidgets::DigitisationWidget::append_point_to_geometry(
		double lat,
		double lon,
		QTreeWidgetItem *target_geometry_item)
{
	// Make a QTreeWidgetItem for the coordinates,
	// and append it to a root QTreeWidgetItem which we use for that
	// part of the geometry.
	// FIXME: Assuming single-part geometries for now.
	if (target_geometry_item == NULL) {
		target_geometry_item = treewidget_coordinates->topLevelItem(0);
	}
	d_undo_stack.push(new GPlatesUndoCommands::DigitisationAddPoint(
			*this, lat, lon, target_geometry_item));
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


