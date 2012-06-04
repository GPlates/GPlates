/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2011 The University of Sydney, Australia
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

#include "ActionButtonBox.h"
#include "ConnectWFSDialog.h"
#include "CreateFeatureDialog.h"
#include "ExportCoordinatesDialog.h"
#include "LatLonCoordinatesTable.h"
#include "QtWidgetUtils.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "gui/ChooseCanvasToolUndoCommand.h"
#include "gui/Dialogs.h"

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
		GPlatesViewOperations::GeometryBuilder &digitise_geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesPresentation::ViewState &view_state_,
		ViewportWindow &viewport_window_,
		QAction *clear_action,
		QAction *undo_action,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_viewport_window( &viewport_window_ ),
	d_export_coordinates_dialog(
			new ExportCoordinatesDialog(
				view_state_,
				this)),
	d_create_feature_dialog(
			new CreateFeatureDialog(
				view_state_,
				viewport_window_,
				GPlatesQtWidgets::CreateFeatureDialog::NORMAL, this)),
	d_new_geom_builder(&digitise_geometry_builder),
	d_canvas_tool_workflows(&canvas_tool_workflows)
{
	setupUi(this);

	// Add the action button box.
	ActionButtonBox *action_button_box = new ActionButtonBox(2, 16, this);
	action_button_box->add_action(clear_action);
	action_button_box->add_action(undo_action);
#ifndef Q_WS_MAC
	int desired_height = button_create_feature->sizeHint().height();
	action_button_box->setFixedHeight(desired_height);
	button_export_coordinates->setFixedHeight(desired_height);
	button_use_in_wfs->setFixedHeight(desired_height);
#endif
	QtWidgetUtils::add_widget_to_placeholder(
			action_button_box,
			action_button_box_placeholder_widget);
	
	// Set up the header of the coordinates widget.
	coordinates_table()->header()->setResizeMode(QHeaderView::Stretch);

	// Get a wrapper around coordinates table that listens to a GeometryBuilder
	// and fills in the table accordingly.
	d_lat_lon_coordinates_table.reset(
			new LatLonCoordinatesTable(coordinates_table(), geometry_operation_state));
	
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
GPlatesQtWidgets::DigitisationWidget::handle_use_in_wfs()
{
	GPlatesViewOperations::GeometryBuilder::geometry_opt_ptr_type geometry_opt_ptr =
		d_new_geom_builder->get_geometry_on_sphere();

	// Feed the dialog the GeometryOnSphere you've set up for the current points. 
	if (geometry_opt_ptr) {

		// Give a GeometryOnSphere::non_null_ptr_to_const_type to the dialog.
		d_viewport_window->dialogs().pop_up_connect_wfs_dialog();
		d_viewport_window->dialogs().connect_wfs_dialog().set_request_geometry( *geometry_opt_ptr );
		
	} else {
		QMessageBox::warning(this, tr("No geometry to export"),
				tr("There is no valid geometry to export."),
				QMessageBox::Ok);
	}
}

void
GPlatesQtWidgets::DigitisationWidget::make_signal_slot_connections()
{
	// Export... button to open the Export Coordinates dialog.
	QObject::connect(
			button_export_coordinates,
			SIGNAL(clicked()),
			this,
			SLOT(handle_export()));

	// Use in WFS button to open the WFS dialog.
	QObject::connect(
			button_use_in_wfs,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_in_wfs()));

	// Create... button to open the Create Feature dialog.
	QObject::connect(
			button_create_feature,
			SIGNAL(clicked()),
			this,
			SLOT(handle_create()));

	QObject::connect(
			d_new_geom_builder,
			SIGNAL(stopped_updating_geometry()),
			this,
			SLOT(handle_geometry_changed()));
}


void
GPlatesQtWidgets::DigitisationWidget::handle_activation()
{
	reload_coordinates_table_if_necessary();
}


QString
GPlatesQtWidgets::DigitisationWidget::get_clear_action_text() const
{
	return tr("C&lear Geometry");
}


bool
GPlatesQtWidgets::DigitisationWidget::clear_action_enabled() const
{
	return d_new_geom_builder->has_geometry();
}


void
GPlatesQtWidgets::DigitisationWidget::handle_clear_action_triggered()
{
	// Group two undo commands into one.
	std::auto_ptr<QUndoCommand> undo_command(
		new GPlatesViewOperations::UndoRedo::GroupUndoCommand(QObject::tr("clear geometry")));

	// Add child undo command for clearing all geometries.
	new GPlatesViewOperations::GeometryBuilderClearAllGeometries(
			*d_new_geom_builder,
			undo_command.get());

	// Add child undo command for selecting the most recently selected digitise geometry tool.
	// When or if this undo/redo command gets called one of the digitisation tools may not
	// be active so make sure it gets activated so user can see what's being undone/redone.
	new GPlatesGui::ChooseCanvasToolUndoCommand(*d_canvas_tool_workflows, undo_command.get());

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the point is initially added.
	GPlatesViewOperations::UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}


void
GPlatesQtWidgets::DigitisationWidget::handle_geometry_changed()
{
	bool has_geometry = d_new_geom_builder->has_geometry();
	emit_clear_action_enabled_changed(has_geometry);
	button_export_coordinates->setEnabled(has_geometry);
	button_use_in_wfs->setEnabled(has_geometry);
}

