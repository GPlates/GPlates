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
#include <QHeaderView>
#include <QTreeWidget>
#include <QUndoStack>
#include <QToolButton>
#include <QMessageBox>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/cast.hpp>

#include "DigitisationWidget.h"
#include "LatLonCoordinatesTable.h"
#include "ExportCoordinatesDialog.h"
#include "CreateFeatureDialog.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "gui/ChooseCanvasTool.h"
#include "maths/InvalidLatLonException.h"
#include "maths/InvalidLatLonCoordinateException.h"
#include "maths/LatLonPoint.h"
#include "maths/GeometryOnSphere.h"
#include "maths/Real.h"
#include "utils/GeometryCreationUtils.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/GeometryBuilderUndoCommands.h"
#include "view-operations/UndoRedo.h"


GPlatesQtWidgets::DigitisationWidget::DigitisationWidget(
		GPlatesViewOperations::GeometryBuilder &new_geometry_builder,
		GPlatesPresentation::ViewState &view_state_,
		ViewportWindow &viewport_window_,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		QWidget *parent_):
	QWidget(parent_),
	d_export_coordinates_dialog(
			new ExportCoordinatesDialog(
				view_state_,
				this)),
	d_create_feature_dialog(
			new CreateFeatureDialog(
					view_state_,
					viewport_window_,
					GPlatesQtWidgets::CreateFeatureDialog::NORMAL, this)),
	d_new_geom_builder(&new_geometry_builder),
	d_choose_canvas_tool(&choose_canvas_tool)
{
	setupUi(this);
	
	// Set up the header of the coordinates widget.
	coordinates_table()->header()->setResizeMode(QHeaderView::Stretch);

	// Get a wrapper around coordinates table that listens to a GeometryBuilder
	// and fills in the table accordingly.
	d_lat_lon_coordinates_table.reset(new LatLonCoordinatesTable(
			coordinates_table(), &new_geometry_builder));
	
	make_signal_slot_connections();
}

GPlatesQtWidgets::DigitisationWidget::~DigitisationWidget()
{
	// boost::scoped_ptr destructor requires complete type.
}

void
GPlatesQtWidgets::DigitisationWidget::handle_create()
{
	GPlatesViewOperations::GeometryBuilder::geometry_opt_ptr_type geometry_opt_ptr =
		d_new_geom_builder->get_geometry_on_sphere();

	// Feed the Create dialog the GeometryOnSphere you've set up for the current
	// points. You -have- set up a GeometryOnSphere for the current points,
	// haven't you?
	if (geometry_opt_ptr) {
		// Give a GeometryOnSphere::non_null_ptr_to_const_type to the Create dialog.
		bool success = d_create_feature_dialog->set_geometry_and_display(*geometry_opt_ptr);
		if ( ! success) {
			// The user cancelled the creation process. Return early and do not reset
			// the digitisation widget.
			return;
		}
	} else {
		QMessageBox::warning(this, tr("No geometry for feature"),
				tr("There is no valid geometry to use for creating a feature."),
				QMessageBox::Ok);
		return;
		// FIXME: Should it be possible to make geometryless features from a digitisation tool?
	}
	
	// Then, when we're all done, reset the widget for new input.
	// No undo desired here.
	d_new_geom_builder->clear_all_geometries();

	// FIXME: Undo! but that ties into the main 'model' QUndoStack.
	// So the simplest thing to do at this stage is clear the undo stack.
	GPlatesViewOperations::UndoRedo::instance().get_active_undo_stack().clear();
}


void
GPlatesQtWidgets::DigitisationWidget::handle_clear()
{
	// Group two undo commands into one.
	std::auto_ptr<QUndoCommand> undo_command(
		new GPlatesViewOperations::UndoRedo::GroupUndoCommand(QObject::tr("clear geometry")));

	// Add child undo command for clearing all geometries.
	new GPlatesViewOperations::GeometryBuilderClearAllGeometries(
			d_new_geom_builder,
			undo_command.get());

	// Add child undo command for selecting the most recently selected digitise geometry tool.
	// When or if this undo/redo command gets called one of the digitisation tools may not
	// be active so make sure it gets activated so user can see what's being undone/redone.
	new GPlatesGui::ChooseCanvasToolUndoCommand(
			d_choose_canvas_tool,
			&GPlatesGui::ChooseCanvasTool::choose_most_recent_digitise_geometry_tool,
			undo_command.get());

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the point is initially added.
	GPlatesViewOperations::UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}


void
GPlatesQtWidgets::DigitisationWidget::handle_export()
{
	GPlatesViewOperations::GeometryBuilder::geometry_opt_ptr_type geometry_opt_ptr =
		d_new_geom_builder->get_geometry_on_sphere();

	// Feed the Export dialog the GeometryOnSphere you've set up for the current
	// points. You -have- set up a GeometryOnSphere for the current points,
	// haven't you?
	if (geometry_opt_ptr) {
		// Give a GeometryOnSphere::non_null_ptr_to_const_type to the Export dialog.
		d_export_coordinates_dialog->set_geometry_and_display(*geometry_opt_ptr);
	} else {
		QMessageBox::warning(this, tr("No geometry to export"),
				tr("There is no valid geometry to export."),
				QMessageBox::Ok);
	}
}

void
GPlatesQtWidgets::DigitisationWidget::make_signal_slot_connections()
{
	// Clear button to clear points from table and start over.
	QObject::connect(button_clear_coordinates, SIGNAL(clicked()),
			this, SLOT(handle_clear()));

	// Export... button to open the Export Coordinates dialog.
	QObject::connect(button_export_coordinates, SIGNAL(clicked()),
			this, SLOT(handle_export()));

	// Create... button to open the Create Feature dialog.
	QObject::connect(button_create_feature, SIGNAL(clicked()),
			this, SLOT(handle_create()));
}
