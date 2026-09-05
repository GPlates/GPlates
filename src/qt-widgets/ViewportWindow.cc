/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 The University of Sydney, Australia
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Geological Survey of Norway
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

#if defined(_MSC_VER) && _MSC_VER <= 1400
////Visual C++ 2005
#pragma warning( disable : 4005 )
#endif 

#include <iostream>
#include <iterator>
#include <memory>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/bind/bind.hpp>

#include <QActionGroup>
#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QProgressBar>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <QToolBar>
#include <QWidget>

#include "ViewportWindow.h"

#include "ActionButtonBox.h"
#include "CanvasToolBarDockWidget.h"
#include "ChooseFeatureCollectionDialog.h"
#include "CreateFeatureDialog.h"
#include "DigitisationWidget.h"
#include "DockWidget.h"
#include "FeaturePropertiesDialog.h"
#include "GlobeCanvas.h"
#include "GlobeAndMapWidget.h"
#include "ImportRasterDialog.h"
#include "ImportScalarField3DDialog.h"
#include "MapView.h"
#include "PythonConsoleDialog.h"
#include "QtWidgetUtils.h"
#include "ReadErrorAccumulationDialog.h"
#include "ReconstructionViewWidget.h"
#include "SaveFileDialog.h"
#include "SearchResultsDockWidget.h"
#include "TaskPanel.h"
#include "VisualLayersDialog.h"

#include "api/DeferredApiCall.h"
#include "api/PythonInterpreterLocker.h"
#include "api/PythonUtils.h"
#include "api/Sleeper.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "canvas-tools/GeometryOperationState.h"
#include "canvas-tools/MeasureDistanceState.h"
#include "canvas-tools/ModifyGeometryState.h"

#include "file-io/ReadErrorAccumulation.h"
#include "file-io/SymbolFileReader.h"

#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"
#include "global/Version.h"
#include "global/config.h"
#include "global/python.h"

#include "gui/AnimationController.h"
#include "gui/CanvasToolWorkflows.h"
#include "gui/ColourSchemeDelegator.h"
#include "gui/DockState.h"
#include "gui/Dialogs.h"
#include "gui/FeatureFocus.h"
#include "gui/FileIOFeedback.h"
#include "gui/FullScreenMode.h"
#include "gui/GuiDebug.h"
#include "gui/ImportMenu.h"
#include "gui/PythonManager.h"
#include "gui/RenderSettings.h"
#include "gui/SessionMenu.h"
#include "gui/TrinketArea.h"
#include "gui/UnsavedChangesTracker.h"
#include "gui/UtilitiesMenu.h"

#include "model/Model.h"
#include "model/types.h"

#include "presentation/SessionManagement.h"
#include "presentation/ViewState.h"

#include "utils/ComponentManager.h"
#include "utils/DeferredCallEvent.h"
#include "utils/Profile.h"

#include "view-operations/CloneOperation.h"
#include "view-operations/DeleteFeatureOperation.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryParameters.h"
#include "view-operations/UndoRedo.h"

namespace GPlatesQtWidgets
{
	namespace
	{
		const char *STATUS_MESSAGE_SUFFIX_FOR_GLOBE = QT_TR_NOOP("Ctrl+drag to re-orient the globe.");
		const char *STATUS_MESSAGE_SUFFIX_FOR_MAP = QT_TR_NOOP("Ctrl+drag to pan the map.");

		void
		canvas_tool_status_message(
				GPlatesQtWidgets::ViewportWindow &viewport_window,
				const char *message)
		{
			viewport_window.status_message(
					GPlatesQtWidgets::ViewportWindow::tr(message) + " " +
					GPlatesQtWidgets::ViewportWindow::tr(
						viewport_window.reconstruction_view_widget().globe_is_active() ?
							STATUS_MESSAGE_SUFFIX_FOR_GLOBE : STATUS_MESSAGE_SUFFIX_FOR_MAP));
		}

		void
		add_shortcut_to_tooltip(
				QAction *action)
		{
			if (!action->shortcut().isEmpty())
			{
				action->setToolTip(QString()); // Reset to default.
				action->setToolTip(action->toolTip() + "  " +
						action->shortcut().toString(QKeySequence::NativeText));
			}
		}
	}
}


// ViewportWindow constructor
GPlatesQtWidgets::ViewportWindow::ViewportWindow(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state) :
	d_application_state(application_state),
	d_view_state(view_state),
	d_geometry_operation_state_ptr(
			new GPlatesCanvasTools::GeometryOperationState()),
	d_modify_geometry_state(
			new GPlatesCanvasTools::ModifyGeometryState()),
	d_measure_distance_state_ptr(
			new GPlatesCanvasTools::MeasureDistanceState(
				get_view_state().get_rendered_geometry_collection(),
				*d_geometry_operation_state_ptr)),
	d_canvas_tool_workflows(
			new GPlatesGui::CanvasToolWorkflows()),
	d_clone_operation_ptr(
			new GPlatesViewOperations::CloneOperation(
				*d_canvas_tool_workflows,
				get_view_state().get_digitise_geometry_builder(),
				get_view_state().get_focused_feature_geometry_builder(),
				get_view_state())),
	d_delete_feature_operation_ptr(
			new GPlatesViewOperations::DeleteFeatureOperation(
				get_view_state().get_feature_focus(),
				get_application_state())),
	d_dialogs_ptr(
			new GPlatesGui::Dialogs(
				get_application_state(),
				get_view_state(),
				*this,
				this)),
	d_full_screen_mode(
			new GPlatesGui::FullScreenMode(*this)),
	d_trinket_area_ptr(
			new GPlatesGui::TrinketArea(*d_dialogs_ptr, *this)),
	d_unsaved_changes_tracker_ptr(
			new GPlatesGui::UnsavedChangesTracker(
				*this,
				get_application_state().get_feature_collection_file_state(),
				get_application_state().get_feature_collection_file_io(),
				get_view_state().get_session_management(),
				this)),
	d_file_io_feedback_ptr(
			new GPlatesGui::FileIOFeedback(
				get_application_state(),
				get_view_state(),
				*this,
				get_view_state().get_feature_focus(),
				this)),
	d_session_menu_ptr(
			new GPlatesGui::SessionMenu(
				get_application_state(),
				get_view_state(),
				*d_file_io_feedback_ptr,
				this)),
	d_import_menu_ptr(NULL), // Needs to be set up after call to setupUi.
	d_utilities_menu_ptr(NULL), // Needs to be set up after call to setupUi.
	d_dock_state_ptr(
			new GPlatesGui::DockState(
				*this,
				this)),
	d_search_results_dock_ptr(NULL),
	d_canvas_tools_dock_ptr(NULL),
	d_reconstruction_view_widget_ptr(
			new ReconstructionViewWidget(
				*this,
				get_view_state(),
				this)),
	d_task_panel_ptr(NULL),
	d_undo_action_ptr(
			GPlatesViewOperations::UndoRedo::instance().get_undo_group().createUndoAction(this, tr("&Undo"))),
	d_redo_action_ptr(
			GPlatesViewOperations::UndoRedo::instance().get_undo_group().createRedoAction(this, tr("Re&do"))),
	d_inside_update_undo_action_tooltip(false),
	d_inside_update_redo_action_tooltip(false)
{
	setupUi(this);

	// FIXME: remove this when all non Qt widget state has been moved into ViewState.
	// This is a temporary solution to avoiding passing ViewportWindow references around
	// when only non Qt widget related view state is needed - currently ViewportWindow contains
	// some of this state so just get the ViewportWindow reference from ViewState -
	// the method will eventually get removed when the state has been moved over.
	get_view_state().set_other_view_state(*this);

	//
	// Currently the order of initialisation of canvas tools dock, canvas tool workflows and task panel
	// is precarious due to references to each other via ViewportWindow.
	// So the order that currently works is:
	//  (1) canvas tools dock,
	//  (2) task panel,
	//  (3) canvas tool workflows.
	//

	// Create the tabbed search results dock widget.
	d_search_results_dock_ptr = new SearchResultsDockWidget(
			*d_dock_state_ptr,
			get_view_state().get_feature_table_model(),
			*this);
	d_search_results_dock_ptr->dock_at_bottom();

	// Create the tabbed canvas toolbar dock widget.
	d_canvas_tools_dock_ptr = new CanvasToolBarDockWidget(
			*d_dock_state_ptr,
			canvas_tool_workflows(),
			*this);
	d_canvas_tools_dock_ptr->dock_at_left();

	// Specify which dock widget area should occupy the bottom left corner of the main window.
	//
	// Previously the canvas tools were in a QToolBar which is a primary window around the central
	// widget (so it wasn't affected by the bottom 'search results' dock window).
	// See the layout diagram at http://doc.trolltech.com/4.4/qdockwidget.html#details
	// Now the canvas tools are in a QDockWindow (which is a secondary window) so we have to specify
	// which dock window gets the corner areas.
	//
	// Handle cases where user docks canvas toolbar to the left or right while search results dock is at *bottom*.
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	// Handle cases where user docks canvas toolbar to the left or right while search results dock is at *top*.
	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);

	d_task_panel_ptr = new TaskPanel(
			get_view_state().get_digitise_geometry_builder(),
			*d_geometry_operation_state_ptr,
			*d_modify_geometry_state,
			*d_measure_distance_state_ptr,
			d_undo_action_ptr,
			d_redo_action_ptr,
			*d_canvas_tool_workflows,
			get_view_state(),
			*this,
			this);

	// Switch to the appropriate task panel tab when a canvas tool is activated.
	QObject::connect(
			&canvas_tool_workflows(),
			SIGNAL(canvas_tool_activated(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)),
			this,
			SLOT(handle_canvas_tool_activated(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)));

	// Now that the canvas tools dock and task panel have been setup we can create the
	// canvas tool workflows/tools and activate the default canvas workflow/tool.
	// This will also activate the default canvas tool to start things going.
	canvas_tool_workflows().initialise(
			*d_geometry_operation_state_ptr,
			*d_modify_geometry_state,
			*d_measure_distance_state_ptr,
			boost::bind(&canvas_tool_status_message, boost::ref(*this), boost::placeholders::_1),
			get_view_state(),
			*this);

	// Connect all the Signal/Slot relationships of ViewportWindow's
	// toolbar buttons and menu items.
	connect_menu_actions();

	// Duplicate the menu structure for the full-screen-mode GMenu.
	populate_gmenu_from_menubar();

	// Initialise various elements for full-screen-mode that must wait until after setupUi().
	d_full_screen_mode->init();

	// Initialise the Recent Session menu (that must wait until after setupUi()).
	d_session_menu_ptr->init(*menu_Open_Recent_Session);
	
	// FIXME: Set up the Task Panel in a more detailed fashion here.
#if 1
	d_reconstruction_view_widget_ptr->insert_task_panel(d_task_panel_ptr);
#else
	// Stretchable Task Panel hack for testing: Make the Task Panel
	// into a QDockWidget, undocked by default.
	QDockWidget *task_panel_dock = new QDockWidget(tr("Task Panel"), this);
	task_panel_dock->setObjectName("TaskPanelDock");
	task_panel_dock->setWidget(d_task_panel_ptr);
	task_panel_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, task_panel_dock);
	task_panel_dock->setFloating(true);
#endif
	set_up_task_panel_actions();

	// Disable the feature-specific Actions as there is no currently focused feature to act on.
	enable_or_disable_feature_actions(get_view_state().get_feature_focus());
	QObject::connect(
			&get_view_state().get_feature_focus(),
			SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(enable_or_disable_feature_actions(GPlatesGui::FeatureFocus &)));

	// Set up the Reconstruction View widget.
	setCentralWidget(d_reconstruction_view_widget_ptr);

	// Listen for file read errors so can update/display read errors dialog.
	QObject::connect(
			&get_application_state().get_feature_collection_file_io(),
			SIGNAL(handle_read_errors(
					const GPlatesFileIO::ReadErrorAccumulation &)),
			this,
			SLOT(handle_read_errors(
					const GPlatesFileIO::ReadErrorAccumulation &)));


	// After modifying the move-nearby-vertices widget, this will reset the focus to the globe/map.
	// FIXME: Find a more general way to do this for changes made by the user in the task panel tabs.
	QObject::connect(
			d_modify_geometry_state.get(),
			SIGNAL(snap_vertices_setup_changed(bool,double,bool,GPlatesModel::integer_plate_id_type)),
			d_reconstruction_view_widget_ptr,
			SLOT(setFocus()));

	// Initialise the "Trinket Area", a class which manages the various icons present in the
	// status bar. This must occur after ViewportWindow::setupUi().
	d_trinket_area_ptr->init();

	// Initialise the "Unsaved Changes" tracking aspect of the GUI, now that setupUi() has
	// been called and all the widgets that are used to notify the user are in place.
	d_unsaved_changes_tracker_ptr->init();

	// Synchronise the "Show X Features" menu items with RenderSettings.
	GPlatesGui::RenderSettings &render_settings = get_view_state().get_render_settings();
	action_Show_Static_Points->setChecked(render_settings.show_static_points());
	action_Show_Static_Lines->setChecked(render_settings.show_static_lines());
	action_Show_Static_Polygons->setChecked(render_settings.show_static_polygons());
	action_Show_Static_Multipoints->setChecked(render_settings.show_static_multipoints());
	action_Show_Velocity_Arrows->setChecked(render_settings.show_velocity_arrows());
	action_Show_Topological_Sections->setChecked(render_settings.show_topological_sections());
	action_Show_Topological_Lines->setChecked(render_settings.show_topological_lines());
	action_Show_Topological_Polygons->setChecked(render_settings.show_topological_polygons());
	action_Show_Topological_Networks->setChecked(render_settings.show_topological_networks());
	action_Show_Rasters->setChecked(render_settings.show_rasters());
	action_Show_3D_Scalar_Fields->setChecked(render_settings.show_3d_scalar_fields());
	action_Show_Scalar_Coverages->setChecked(render_settings.show_scalar_coverages());

	// Synchronise "Show Stars" with what's in ViewState.
	action_Show_Stars->setChecked(get_view_state().get_show_stars());

	// Repaint the globe/map when the colour scheme delegator's target changes.
	QObject::connect(
			get_view_state().get_colour_scheme_delegator().get(),
			SIGNAL(changed()),
			this,
			SLOT(handle_colour_scheme_delegator_changed()));

	// Get notified about visual layers being added so we can open the layers dialog.
	QObject::connect(
			&(get_view_state().get_visual_layers()),
			SIGNAL(layer_added(size_t)),
			this,
			SLOT(handle_visual_layer_added(size_t)));

	// Get notified about project filename changes so we can change the project filename in
	// the window title.
	QObject::connect(
			&(get_view_state().get_session_management()),
			SIGNAL(changed_project_filename(boost::optional<QString>)),
			this,
			SLOT(handle_changed_project_filename(boost::optional<QString>)));

	set_window_title();

    // Create the visual layers dialog (but don't show it yet).
    // This is so it can listen for signals before it first pops up
    // (which is when the first layer is added).
	dialogs().visual_layers_dialog();
}


GPlatesQtWidgets::ViewportWindow::~ViewportWindow()
{
	// boost::scoped_ptr destructors need complete type.
}


void
GPlatesQtWidgets::ViewportWindow::load_project(
		const QString &project_filename)
{
	d_file_io_feedback_ptr->open_project(project_filename);
}


void
GPlatesQtWidgets::ViewportWindow::load_feature_collections(
		const QStringList &filenames)
{
	d_file_io_feedback_ptr->open_files(filenames);
}


void
GPlatesQtWidgets::ViewportWindow::display()
{
	//
	// First show the main window so that everything is visible.
	//

	show();

	//
	// Then call functions that rely on the main window being visible.
	//

	// Re-position the visual layers dialog relative to the main window.
	//
	// We do this here because the visual layers dialog now gets created in the ViewportWindow
	// constructor (so it can receive signals earlier) and hence parent (ViewportWindow) geometry
	// queries are not valid (because ViewportWindow is not yet visible at that stage).
	GPlatesQtWidgets::QtWidgetUtils::reposition_to_side_of_parent(&dialogs().visual_layers_dialog());

	// Activate the default canvas tool which checks for visibility of the main view canvas.
	// In particular this needs to be done after the globe canvas is visible otherwise the default
	// canvas tool will not get activated.
	canvas_tool_workflows().activate();
}


void
GPlatesQtWidgets::ViewportWindow::handle_read_errors(
		const GPlatesFileIO::ReadErrorAccumulation &new_read_errors)
{
	// Pop up errors only if we have new read errors.
	if (new_read_errors.is_empty())
	{
		return;
	}

	// Populate the read errors dialog.
	ReadErrorAccumulationDialog &read_errors_dialog = dialogs().read_error_accumulation_dialog();
	read_errors_dialog.clear();
	read_errors_dialog.read_errors().accumulate(new_read_errors);
	read_errors_dialog.update();
	
	// At this point we can either throw the dialog in the user's face,
	// or pop up a small icon in the status bar which they can click to see the errors.
	// How do we decide? Well, until we get UserPreferences, let's just pop up the icon
	// on warnings, and show the whole dialog on any kind of real error.
	GPlatesFileIO::ReadErrors::Severity severity = new_read_errors.most_severe_error_type();
	if (severity > GPlatesFileIO::ReadErrors::Warning)
	{
		read_errors_dialog.show();
	}
	else
	{
		d_trinket_area_ptr->read_errors_trinket().setVisible(true);
	}
}


void	
GPlatesQtWidgets::ViewportWindow::connect_menu_actions()
{
	// If you want to add a new menu action, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	// 3. Drag this action to a menu.
	// 4. If your shortcut key uses 'Ctrl', it is most likely an Application shortcut
	//    that should be usable from within any window or non-modal dialog of GPlates.
	//    It's not immediately obvious how to set this via Designer.
	//    First, select the QAction, either in the Action Editor or the Object inspector.
	//    This will adjust the Property Editor window - find the "shortcutContext" property,
	//    and set it to "ApplicationShortcut".
	// 5. Add code for the triggered() signal your action generates here.
	//    Please keep this function sorted in the same order as menu items appear.

	connect_file_menu_actions();
	connect_edit_menu_actions();
	connect_view_menu_actions();
	connect_features_menu_actions();
	connect_reconstruction_menu_actions();
	connect_utilities_menu_actions();
	connect_tools_menu_actions();
	connect_window_menu_actions();
	connect_help_menu_actions();
}


void
GPlatesQtWidgets::ViewportWindow::connect_file_menu_actions()
{
	QObject::connect(action_Open_Feature_Collection, SIGNAL(triggered()),
			d_file_io_feedback_ptr, SLOT(open_files()));

	QObject::connect(action_Open_Project, SIGNAL(triggered()),
			d_file_io_feedback_ptr, SLOT(open_project()));

	QObject::connect(action_Save_Project, SIGNAL(triggered()),
			d_file_io_feedback_ptr, SLOT(save_project()));

	QObject::connect(action_Save_Project_As, SIGNAL(triggered()),
			d_file_io_feedback_ptr, SLOT(save_project_as()));

	QObject::connect(action_Clear_Session, SIGNAL(triggered()),
			d_file_io_feedback_ptr, SLOT(clear_session()));

	// Show the status tips related to projects/sessions in the status bar.
	// This is needed since the status tips don't appear to show on some platforms.
	QObject::connect(
			action_Open_Project, SIGNAL(hovered()),
			this, SLOT(show_menu_item_status_tip_in_status_bar()));
	QObject::connect(
			action_Save_Project, SIGNAL(hovered()),
			this, SLOT(show_menu_item_status_tip_in_status_bar()));
	QObject::connect(
			action_Save_Project_As, SIGNAL(hovered()),
			this, SLOT(show_menu_item_status_tip_in_status_bar()));
	QObject::connect(
			action_Clear_Session, SIGNAL(hovered()),
			this, SLOT(show_menu_item_status_tip_in_status_bar()));
	QObject::connect(
			menu_Open_Recent_Session, SIGNAL(aboutToShow()),
			this, SLOT(show_menu_item_status_tip_in_status_bar()));

	// ----
	// Import submenu logic is handled by the ImportMenu class.
	// Note: items to the Import submenu should be added programmatically, through
	// @a d_import_menu_ptr, instead of via the designer.
	d_import_menu_ptr = new GPlatesGui::ImportMenu(
			menu_Import,
			menu_File,
			this);
	// Import raster...
	d_import_menu_ptr->add_import(
			GPlatesGui::ImportMenu::RASTER,
			"Import &Raster...",
			boost::bind(&ViewportWindow::pop_up_import_raster_dialog, boost::ref(*this)));
	// Import time-dependent raster...
	d_import_menu_ptr->add_import(
			GPlatesGui::ImportMenu::RASTER,
			"Import &Time-Dependent Raster...",
			boost::bind(&ViewportWindow::pop_up_import_time_dependent_raster_dialog, boost::ref(*this)));
	// Import 3D scalar field...
	d_import_menu_ptr->add_import(
			GPlatesGui::ImportMenu::SCALAR_FIELD_3D,
			"Import 3D &Scalar Field...",
			boost::bind(&ViewportWindow::pop_up_import_scalar_field_3d_dialog, boost::ref(*this)));

	// ----
	QObject::connect(action_Manage_Feature_Collections, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_manage_feature_collections_dialog()));
	QObject::connect(action_File_Errors, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_read_error_accumulation_dialog()));

	// ----
	QObject::connect(action_Quit, SIGNAL(triggered()),
			this, SLOT(close()));

	QObject::connect(actionConnect_WFS, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_connect_wfs_dialog()));
}


void
GPlatesQtWidgets::ViewportWindow::connect_edit_menu_actions()
{
	// Unfortunately, the Undo and Redo actions cannot be added in the Designer,
	// or at least, not nicely. We need to ask the QUndoGroup to create some
	// QActions for us, and add them programmatically. To follow the principle
	// of least surprise, placeholder actions are set up in the designer, which
	// this code can use to insert the actions in the correct place with the
	// correct shortcut.
	// The new actions will be linked to the QUndoGroup appropriately.
	d_undo_action_ptr->setShortcut(action_Undo_Placeholder->shortcut());
	d_redo_action_ptr->setShortcut(action_Redo_Placeholder->shortcut());
	d_undo_action_ptr->setIcon(action_Undo_Placeholder->icon());
	d_redo_action_ptr->setIcon(action_Redo_Placeholder->icon());
	menu_Edit->insertAction(action_Undo_Placeholder, d_undo_action_ptr);
	menu_Edit->insertAction(action_Redo_Placeholder, d_redo_action_ptr);
	menu_Edit->removeAction(action_Undo_Placeholder);
	menu_Edit->removeAction(action_Redo_Placeholder);
	QObject::connect(
			d_undo_action_ptr,
			SIGNAL(changed()),
			this,
			SLOT(update_undo_action_tooltip()));
	QObject::connect(
			d_redo_action_ptr,
			SIGNAL(changed()),
			this,
			SLOT(update_redo_action_tooltip()));
	add_shortcut_to_tooltip(d_undo_action_ptr);
	add_shortcut_to_tooltip(d_redo_action_ptr);
	// ----
	QObject::connect(action_Query_Feature, SIGNAL(triggered()),
			&dialogs().feature_properties_dialog(), SLOT(choose_query_widget_and_open()));
	QObject::connect(action_Edit_Feature, SIGNAL(triggered()),
			&dialogs().feature_properties_dialog(), SLOT(choose_edit_widget_and_open()));
	QObject::connect(action_Clone_Geometry, SIGNAL(triggered()),
			d_clone_operation_ptr.get(), SLOT(clone_focused_geometry()));
	QObject::connect(action_Clone_Feature, SIGNAL(triggered()),
			this, SLOT(clone_feature_with_dialog()));
	QObject::connect(action_Delete_Feature, SIGNAL(triggered()),
			d_delete_feature_operation_ptr.get(), SLOT(delete_focused_feature()));
	add_shortcut_to_tooltip(action_Query_Feature);
	add_shortcut_to_tooltip(action_Edit_Feature);
	add_shortcut_to_tooltip(action_Clone_Geometry);
	add_shortcut_to_tooltip(action_Clone_Feature);
	add_shortcut_to_tooltip(action_Delete_Feature);
	// ----
	// Replace action_Clear_Placeholder with the TaskPanel's clear action.
	QAction *clear_action_ptr = d_task_panel_ptr->get_clear_action();
	clear_action_ptr->setShortcut(action_Clear_Placeholder->shortcut());
	menu_Edit->insertAction(action_Clear_Placeholder, clear_action_ptr);
	menu_Edit->removeAction(action_Clear_Placeholder);
	// ----
	QObject::connect(action_Preferences, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_preferences_dialog()));
}


void
GPlatesQtWidgets::ViewportWindow::connect_view_menu_actions()
{
	QObject::connect(action_Set_Projection, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_set_projection_dialog()));
	// ----
	QObject::connect(action_Set_Camera_Viewpoint, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_set_camera_viewpoint_dialog()));
	QObject::connect(action_Move_Camera_Up, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_up()));
	QObject::connect(action_Move_Camera_Down, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_down()));
	QObject::connect(action_Move_Camera_Left, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_left()));
	QObject::connect(action_Move_Camera_Right, SIGNAL(triggered()),
			this, SLOT(handle_move_camera_right()));
	// ----
	QObject::connect(action_Rotate_Camera_Clockwise, SIGNAL(triggered()),
			this, SLOT(handle_rotate_camera_clockwise()));
	QObject::connect(action_Rotate_Camera_Anticlockwise, SIGNAL(triggered()),
			this, SLOT(handle_rotate_camera_anticlockwise()));
	QObject::connect(action_Reset_Camera_Orientation, SIGNAL(triggered()),
			this, SLOT(handle_reset_camera_orientation()));
	// ----
	QObject::connect(action_Set_Zoom, SIGNAL(triggered()),
			d_reconstruction_view_widget_ptr, SLOT(activate_zoom_spinbox()));
	QObject::connect(action_Zoom_In, SIGNAL(triggered()),
			&get_view_state().get_viewport_zoom(), SLOT(zoom_in()));
	QObject::connect(action_Zoom_Out, SIGNAL(triggered()),
			&get_view_state().get_viewport_zoom(), SLOT(zoom_out()));
	QObject::connect(action_Reset_Zoom_Level, SIGNAL(triggered()),
			&get_view_state().get_viewport_zoom(), SLOT(reset_zoom()));
	// ----
	QObject::connect(action_Configure_Text_Overlay, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_configure_text_overlay_dialog()));
	QObject::connect(action_Configure_Velocity_Legend, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_configure_velocity_legend_overlay_dialog()));
	QObject::connect(action_Configure_Graticules, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_configure_graticules_dialog()));
	QObject::connect(action_Choose_Background_Colour, SIGNAL(triggered()),
			this, SLOT(pop_up_background_colour_picker()));
	QObject::connect(action_Show_Stars, SIGNAL(triggered()),
			this, SLOT(enable_stars_display()));
	// ----
	QObject::connect(action_Show_Static_Points, SIGNAL(triggered()),
			this, SLOT(enable_static_point_display()));
	QObject::connect(action_Show_Static_Lines, SIGNAL(triggered()),
			this, SLOT(enable_static_line_display()));
	QObject::connect(action_Show_Static_Polygons, SIGNAL(triggered()),
			this, SLOT(enable_static_polygon_display()));
	QObject::connect(action_Show_Static_Multipoints, SIGNAL(triggered()),
			this, SLOT(enable_static_multipoint_display()));
	QObject::connect(action_Show_Velocity_Arrows, SIGNAL(triggered()),
			this, SLOT(enable_velocity_arrow_display()));
	QObject::connect(action_Show_Topological_Sections, SIGNAL(triggered()),
			this, SLOT(enable_topological_section_display()));
	QObject::connect(action_Show_Topological_Lines, SIGNAL(triggered()),
			this, SLOT(enable_topological_line_display()));
	QObject::connect(action_Show_Topological_Polygons, SIGNAL(triggered()),
			this, SLOT(enable_topological_polygon_display()));
	QObject::connect(action_Show_Topological_Networks, SIGNAL(triggered()),
			this, SLOT(enable_topological_network_display()));
	QObject::connect(action_Show_Rasters, SIGNAL(triggered()),
			this, SLOT(enable_raster_display()));
	QObject::connect(action_Show_3D_Scalar_Fields, SIGNAL(triggered()),
			this, SLOT(enable_3d_scalar_field_display()));
	QObject::connect(action_Show_Scalar_Coverages, SIGNAL(triggered()),
			this, SLOT(enable_scalar_coverage_display()));
	QObject::connect(action_Show_All_Geometries, SIGNAL(triggered()),
			this, SLOT(enable_all_geometries_display()));
	// Also update the GUI when the RenderSettings change.
	QObject::connect(&get_view_state().get_render_settings(), SIGNAL(settings_changed()),
			this, SLOT(handle_render_settings_changed()));
}


void
GPlatesQtWidgets::ViewportWindow::connect_features_menu_actions()
{
	if(!GPlatesUtils::ComponentManager::instance().is_enabled(GPlatesUtils::ComponentManager::Component::python()))
	{
		QObject::connect(action_Manage_Colouring, SIGNAL(triggered()),
				&dialogs(), SLOT(pop_up_colouring_dialog()));
	}
	else
	{
		QObject::connect(action_Manage_Colouring, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_draw_style_dialog()));
	}
	// ----
	QObject::connect(action_Load_Symbol, SIGNAL(triggered()),
			this, SLOT(handle_load_symbol_file()));
	QObject::connect(action_Unload_Symbol, SIGNAL(triggered()),
			this, SLOT(handle_unload_symbol_file()));
	// ----
	QObject::connect(action_View_Total_Reconstruction_Sequences, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_total_reconstruction_sequences_dialog()));
	QObject::connect(action_View_Shapefile_Attributes, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_shapefile_attribute_viewer_dialog()));
	// ----
	QObject::connect(action_Create_VGP, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_create_vgp_dialog()));
	QObject::connect(action_Assign_Plate_IDs, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_assign_reconstruction_plate_ids_dialog()));
	QObject::connect(action_Generate_Citcoms_Velocity_Domain, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_velocity_domain_citcoms_dialog()));
	QObject::connect(action_Generate_Terra_Velocity_Domain, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_velocity_domain_terra_dialog()));
	QObject::connect(action_Generate_LatitudeLongitude_Velocity_Domain, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_velocity_domain_lat_lon_dialog()));
	QObject::connect(action_Generate_Deforming_Mesh_Points, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_generate_deforming_mesh_points_dialog()));
}


void
GPlatesQtWidgets::ViewportWindow::connect_reconstruction_menu_actions()
{
	QObject::connect(action_Reconstruct_to_Time, SIGNAL(triggered()),
			d_reconstruction_view_widget_ptr, SLOT(activate_time_spinbox()));
	QObject::connect(action_Increment_Animation_Time_Forwards, SIGNAL(triggered()),
			&get_view_state().get_animation_controller(), SLOT(step_forward()));
	QObject::connect(action_Increment_Animation_Time_Backwards, SIGNAL(triggered()),
			&get_view_state().get_animation_controller(), SLOT(step_back()));
	QObject::connect(action_Reset_Animation, SIGNAL(triggered()),
			&get_view_state().get_animation_controller(), SLOT(seek_beginning()));
	QObject::connect(action_Play, SIGNAL(triggered(bool)),
			&get_view_state().get_animation_controller(), SLOT(set_play_or_pause(bool)));
	QObject::connect(&get_view_state().get_animation_controller(), SIGNAL(animation_state_changed(bool)),
			action_Play, SLOT(setChecked(bool)));
	QObject::connect(action_Animate, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_animate_dialog()));
	// ----
	QObject::connect(action_Specify_Anchored_Plate_ID, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_specify_anchored_plate_id_dialog()));
	QObject::connect(action_View_Reconstruction_Poles, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_total_reconstruction_poles_dialog()));
	// ----
	QObject::connect(action_Export, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_export_animation_dialog()));
}


void
GPlatesQtWidgets::ViewportWindow::connect_utilities_menu_actions()
{
	QObject::connect(action_Calculate_Reconstruction_Pole, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_calculate_reconstruction_pole_dialog()));

	QObject::connect(action_Finite_Rotation_Calculator, SIGNAL(triggered()),
		&dialogs(), SLOT(pop_up_finite_rotation_calculator_dialog()));

	QObject::connect(action_Open_Kinematics_Tool, SIGNAL(triggered()),
					 &dialogs(), SLOT(pop_up_kinematics_tool_dialog()));

	if (GPlatesUtils::ComponentManager::instance().is_enabled(
				GPlatesUtils::ComponentManager::Component::hellinger_three_plate()))
	{
		action_Manage_Age_Models->setVisible(true);
		QObject::connect(action_Manage_Age_Models, SIGNAL(triggered()),
				&dialogs(), SLOT(pop_up_age_model_manager_dialog()));

	}

	if(GPlatesUtils::ComponentManager::instance().is_enabled(
			GPlatesUtils::ComponentManager::Component::python()))
	{
		d_utilities_menu_ptr = new GPlatesGui::UtilitiesMenu(
				menu_Utilities,
				action_Open_Python_Console,
				get_view_state().get_python_manager(),
				this);
		
		// ----
		QObject::connect(action_Open_Python_Console, SIGNAL(triggered()),
				this, SLOT(pop_up_python_console()));
	}
	else
	{
		hide_python_menu();
	}
}


void
GPlatesQtWidgets::ViewportWindow::connect_tools_menu_actions()
{
	// Tools menu. This is mostly auto-populated with Canvas Tool actions,
	// which don't need to be hooked up here, however there are a few little
	// extras which aren't regular canvas tools and should be connected here:-
	QObject::connect(action_Use_Small_Icons, SIGNAL(toggled(bool)),
		d_canvas_tools_dock_ptr, SLOT(use_small_canvas_tool_icons(bool)));
	QObject::connect(action_Configure_Geometry_Rendering, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_configure_canvas_tool_geometry_render_parameters_dialog()));

	// Populate the Tools menu with a sub-menu for each canvas tool workflow.
	// And for each workflow populate its sub-menu with the workflow tool actions.
	for (unsigned int tool_workflow = 0;
		tool_workflow < GPlatesGui::CanvasToolWorkflows::NUM_WORKFLOWS;
		++tool_workflow)
	{
		const GPlatesGui::CanvasToolWorkflows::WorkflowType canvas_tool_workflow =
				static_cast<GPlatesGui::CanvasToolWorkflows::WorkflowType>(tool_workflow);

		// Create a new sub-menu for the current workflow.
		const QString workflow_menu_name =
				d_canvas_tools_dock_ptr->get_workflow_tool_menu_name(canvas_tool_workflow);
		QMenu *canvas_workflow_menu = new QMenu(workflow_menu_name, menu_Tools);
		menu_Tools->addMenu(canvas_workflow_menu);

		// Get the tool actions for the current workflow.
		QList<QAction *> canvas_tool_actions =
				d_canvas_tools_dock_ptr->get_workflow_tool_menu_actions(canvas_tool_workflow);

		// Add the workflow tool actions to the sub-menu.
		Q_FOREACH(QAction *canvas_tool_action, canvas_tool_actions)
		{
			canvas_workflow_menu->addAction(canvas_tool_action);
		}
	}
}


void
GPlatesQtWidgets::ViewportWindow::connect_window_menu_actions()
{
	QObject::connect(menu_Window, SIGNAL(aboutToShow()),
			this, SLOT(handle_window_menu_about_to_show()));
	QObject::connect(action_New_Window, SIGNAL(triggered()),
			this, SLOT(open_new_window()));
	// ----
	QObject::connect(action_Show_Layers, SIGNAL(triggered(bool)),
			this, SLOT(set_visual_layers_dialog_visibility(bool)));

	QAction *action_show_bottom_panel = d_search_results_dock_ptr->toggleViewAction();
	action_show_bottom_panel->setText(tr("Show &Bottom Panel"));
	action_show_bottom_panel->setObjectName("action_Show_Bottom_Panel");
	menu_Window->insertAction(action_Show_Bottom_Panel_Placeholder, action_show_bottom_panel);
	menu_Window->removeAction(action_Show_Bottom_Panel_Placeholder);

	QObject::connect(action_Log_Dialog, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_log_dialog()));
	// ----
	QObject::connect(action_Full_Screen, SIGNAL(triggered(bool)),
			d_full_screen_mode, SLOT(toggle_full_screen(bool)));
}


void
GPlatesQtWidgets::ViewportWindow::connect_help_menu_actions()
{
	QObject::connect(action_View_Online_Documentation, SIGNAL(triggered()),
			this, SLOT(open_online_documentation()));
	// ----
	QObject::connect(action_About, SIGNAL(triggered()),
			&dialogs(), SLOT(pop_up_about_dialog()));

	QObject::connect(action_About_GPlates_Dataset, SIGNAL(triggered()),
			this, SLOT(open_dataset_webpage()));
}


GPlatesQtWidgets::CanvasToolBarDockWidget &
GPlatesQtWidgets::ViewportWindow::canvas_tool_bar_dock_widget()
{
	return *d_canvas_tools_dock_ptr;
}


GPlatesQtWidgets::SearchResultsDockWidget &
GPlatesQtWidgets::ViewportWindow::search_results_dock_widget()
{
	return *d_search_results_dock_ptr;
}


GPlatesQtWidgets::GlobeCanvas &
GPlatesQtWidgets::ViewportWindow::globe_canvas()
{
	return d_reconstruction_view_widget_ptr->globe_canvas();
}


const GPlatesQtWidgets::GlobeCanvas &
GPlatesQtWidgets::ViewportWindow::globe_canvas() const
{
	return d_reconstruction_view_widget_ptr->globe_canvas();
}


GPlatesQtWidgets::MapView &
GPlatesQtWidgets::ViewportWindow::map_view()
{
	return d_reconstruction_view_widget_ptr->map_view();
}


const GPlatesQtWidgets::MapView &
GPlatesQtWidgets::ViewportWindow::map_view() const
{
	return d_reconstruction_view_widget_ptr->map_view();
}


GPlatesGui::Dialogs &
GPlatesQtWidgets::ViewportWindow::dialogs() const
{
	return *d_dialogs_ptr;
}


GPlatesGui::FileIOFeedback &
GPlatesQtWidgets::ViewportWindow::file_io_feedback()
{
	return *d_file_io_feedback_ptr;
}


GPlatesGui::CanvasToolWorkflows &
GPlatesQtWidgets::ViewportWindow::canvas_tool_workflows()
{
	return *d_canvas_tool_workflows;
}



GPlatesGui::TrinketArea &
GPlatesQtWidgets::ViewportWindow::trinket_area()
{
	return *d_trinket_area_ptr;
}


GPlatesQtWidgets::TaskPanel *
GPlatesQtWidgets::ViewportWindow::task_panel_ptr() const
{
	return d_task_panel_ptr;
}


GPlatesGui::ImportMenu &
GPlatesQtWidgets::ViewportWindow::import_menu()
{
	return *d_import_menu_ptr;
}


GPlatesGui::UtilitiesMenu &
GPlatesQtWidgets::ViewportWindow::utilities_menu()
{
	return *d_utilities_menu_ptr;
}


void
GPlatesQtWidgets::ViewportWindow::populate_gmenu_from_menubar()
{
	// Populate the GMenu with the menubar's menu actions.
	// For now, this has to be done here in ViewportWindow so we can get at 'menubar'.
	// It is also difficult to do this in GMenu's constructor, where I'd prefer to put
	// it, because at that time @a ViewportWindow::setupUi() hasn't been called yet,
	// so the menu structure does not exist.
	
	// Find the GMenu by Qt object name. This is a lot more convenient for this kind
	// of one-off setup than going through ReconstructionViewWidget, etc.
	QMenu *gmenu = findChild<QMenu *>("GMenu");
	if (gmenu) {
		// Add each of the top-level menu items from the main menu bar.
		QList<QAction *> main_menubar_actions = menubar->actions();
		Q_FOREACH(QAction *action, main_menubar_actions) {
			gmenu->addAction(action);
		}
	}
}


GPlatesAppLogic::ApplicationState &
GPlatesQtWidgets::ViewportWindow::get_application_state()
{
	return d_application_state;
}


GPlatesPresentation::ViewState &
GPlatesQtWidgets::ViewportWindow::get_view_state()
{
	return d_view_state;
}


GPlatesQtWidgets::ReconstructionViewWidget &
GPlatesQtWidgets::ViewportWindow::reconstruction_view_widget()
{
	return *d_reconstruction_view_widget_ptr;
}

const GPlatesQtWidgets::ReconstructionViewWidget &
GPlatesQtWidgets::ViewportWindow::reconstruction_view_widget() const
{
	return *d_reconstruction_view_widget_ptr;
}


void	
GPlatesQtWidgets::ViewportWindow::set_up_task_panel_actions()
{
	ActionButtonBox &feature_actions = d_task_panel_ptr->feature_action_button_box();

	// If you want to add a new action button, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	// 3. Drag this action to a menu (optional).
	// 4. Add code for the triggered() signal your action generates,
	//    see ViewportWindow::connect_menu_actions().
	// 5. Add a new line of code here adding the QAction to the ActionButtonBox.

	feature_actions.add_action(action_Query_Feature);
	feature_actions.add_action(action_Edit_Feature);
	feature_actions.add_action(action_Clone_Geometry);
	feature_actions.add_action(action_Clone_Feature);
	feature_actions.add_action(action_Delete_Feature);
}


void
GPlatesQtWidgets::ViewportWindow::set_visual_layers_dialog_visibility(
		bool visible)
{
	if (visible)
	{
		dialogs().visual_layers_dialog().pop_up();
	}
	else
	{
		dialogs().visual_layers_dialog().hide();
	}
}


void
GPlatesQtWidgets::ViewportWindow::handle_window_menu_about_to_show()
{
	action_Show_Layers->setChecked(dialogs().visual_layers_dialog().isVisible());
}


void
GPlatesQtWidgets::ViewportWindow::handle_load_symbol_file()
{
    QString filename = QFileDialog::getOpenFileName(
		    this,
		    QObject::tr("Open symbol file"),
		    get_view_state().get_last_open_directory(),
		    QObject::tr("Symbol file (*.sym)"));
	if (filename.isNull())
	{
		return;
	}

    try
	{
		GPlatesFileIO::SymbolFileReader::read_file(
			filename,
			get_view_state().get_feature_type_symbol_map());
    }
	catch (std::exception &exc)
	{
		qWarning() << "Failed to load symbol file: " << exc.what();
	}
	catch (...)
	{
		qWarning() << "Failed to load symbol file: unknown error";
    }

    get_application_state().reconstruct();
}

void
GPlatesQtWidgets::ViewportWindow::handle_unload_symbol_file()
{
    get_view_state().get_feature_type_symbol_map().clear();
    get_application_state().reconstruct();
}


void
GPlatesQtWidgets::ViewportWindow::enable_or_disable_feature_actions(
		GPlatesGui::FeatureFocus &feature_focus)
{
	// Note: Enabling/disabling canvas tools is now done in class 'EnableCanvasTool'.
	bool is_feature_focused = feature_focus.focused_feature().is_valid();
	
	action_Query_Feature->setEnabled(is_feature_focused);
	action_Edit_Feature->setEnabled(is_feature_focused);
	action_Delete_Feature->setEnabled(is_feature_focused);
	action_Clone_Feature->setEnabled(is_feature_focused);

	action_Clone_Geometry->setEnabled(is_feature_focused &&
			get_view_state().get_focused_feature_geometry_builder().has_geometry());

#if 0
	// FIXME: Add to Selection is unimplemented and should stay disabled for now.
	// FIXME: To handle the "Remove from Selection", "Clear Selection" actions,
	// we may want to modify this method to also test for a nonempty selection of features.
	action_Add_Feature_To_Selection->setEnabled(enable_canvas_tool_actions);
#endif
}


void
GPlatesQtWidgets::ViewportWindow::closeEvent(
		QCloseEvent *close_event)
{
	// FIXME: Refactor the code below into some app-logic type thing,
	//        and then merely call it from here (checking the bool return value naturally)

	// STEP 1: UNSAVED CHANGES WARNING

	// Check for unsaved changes and ask the user what to do if there are any.
	const GPlatesGui::UnsavedChangesTracker::UnsavedChangesResult unsaved_changes_result =
			d_unsaved_changes_tracker_ptr->close_event_hook();
	if (unsaved_changes_result == GPlatesGui::UnsavedChangesTracker::DONT_DISCARD_UNSAVED_CHANGES)
	{
		// User is Not OK with quitting GPlates at this point.
		close_event->ignore();
		return;
	}

	// STEP 2: RECORDING SESSION DETAILS

	// Remember the current session for next time unless user is discarding unsaved changes
	// (feature collections and/or project session changes).
	if (unsaved_changes_result != GPlatesGui::UnsavedChangesTracker::DISCARD_UNSAVED_CHANGES)
	{
		get_view_state().get_session_management().close_event_hook();
	}

	// STEP 3: FINAL TIDY-UP BEFORE QUITTING

	// User is OK with quitting GPlates at this point.
	close_event->accept();
	// If we decide to accept the close event, we should also tidy up after ourselves.
	dialogs().close_all_dialogs();
	// Make sure we really do quit - stray dialogs not caught by @a close_all_dialogs()
	// (e.g. PyQt windows) will keep GPlates open.
	QCoreApplication::quit();

	//
	// Optimisations to avoid long shutdown times for GPlates.
	//

	//
	// NOTE: This is a very SIGNIFICANT optimisation for large files.
	//
	// For small files it's not noticeable, but for very large files it can reduce shutdown
	// times from minutes to a few seconds.
	//
	// NOTE: This used to be in the destructor of class Application but was moved here because
	// there was a lot of slowdown between here and the Application destructor due to the main
	// event loop emitting events while shutting down (after which the Application destructor starts).
	//
	// Prevent modifications to any rendered geometry collection from signaling updates
	// to various listening clients. We're shutting down so rendered geometry updates are not
	// getting drawn (or used for export, etc).
	// Note that this call starts blocking updates and also we don't subsequently call
	// 'end_update_all_registered_collections()' to unblock them as this is not necessary
	// (if we did it would be limited to this scope anyway and wouldn't block updates from here onwards,
	// including the destructor of class Application and the destructors of all its sub-objects, etc).
	//
	GPlatesViewOperations::RenderedGeometryCollection::begin_update_all_registered_collections();
}


void
GPlatesQtWidgets::ViewportWindow::dragEnterEvent(
		QDragEnterEvent *ev)
{
	if (ev->mimeData()->hasUrls())
	{
		// If there's exactly one project filename then allow it.
		// If there's more than one then ignore altogether.
		const QStringList project_filenames =
				d_file_io_feedback_ptr->extract_project_filenames_from_file_urls(
						ev->mimeData()->urls());
		if (project_filenames.size() == 1)
		{
			ev->acceptProposedAction();
			return;
		}

		const QStringList feature_collection_filenames =
				d_file_io_feedback_ptr->extract_feature_collection_filenames_from_file_urls(
						ev->mimeData()->urls());
		if (!feature_collection_filenames.isEmpty())
		{
			ev->acceptProposedAction();
			return;
		}
	}

	ev->ignore();
}


void
GPlatesQtWidgets::ViewportWindow::dropEvent(
		QDropEvent *ev)
{
	if (ev->mimeData()->hasUrls())
	{
		// If there's exactly one project filename then open it.
		// If there's more than one then ignore altogether.
		const QStringList project_filenames =
				d_file_io_feedback_ptr->extract_project_filenames_from_file_urls(
						ev->mimeData()->urls());
		if (project_filenames.size() == 1)
		{
			ev->acceptProposedAction();
			d_file_io_feedback_ptr->open_project(project_filenames[0]);
			return;
		}

		// Else if there are any feature collection filenames then open them.
		const QStringList feature_collection_filenames =
				d_file_io_feedback_ptr->extract_feature_collection_filenames_from_file_urls(
						ev->mimeData()->urls());
		if (!feature_collection_filenames.isEmpty())
		{
			ev->acceptProposedAction();
			d_file_io_feedback_ptr->open_files(feature_collection_filenames);
			return;
		}
	}

	ev->ignore();
}


void
GPlatesQtWidgets::ViewportWindow::enable_static_point_display()
{
	get_view_state().get_render_settings().set_show_static_points(
			action_Show_Static_Points->isChecked());
}


void
GPlatesQtWidgets::ViewportWindow::enable_static_line_display()
{
	get_view_state().get_render_settings().set_show_static_lines(
			action_Show_Static_Lines->isChecked());
}


void
GPlatesQtWidgets::ViewportWindow::enable_static_polygon_display()
{
	get_view_state().get_render_settings().set_show_static_polygons(
			action_Show_Static_Polygons->isChecked());
}


void
GPlatesQtWidgets::ViewportWindow::enable_static_multipoint_display()
{
	get_view_state().get_render_settings().set_show_static_multipoints(
			action_Show_Static_Multipoints->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_velocity_arrow_display()
{
	get_view_state().get_render_settings().set_show_velocity_arrows(
			action_Show_Velocity_Arrows->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_topological_section_display()
{
	get_view_state().get_render_settings().set_show_topological_sections(
			action_Show_Topological_Sections->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_topological_line_display()
{
	get_view_state().get_render_settings().set_show_topological_lines(
			action_Show_Topological_Lines->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_topological_polygon_display()
{
	get_view_state().get_render_settings().set_show_topological_polygons(
			action_Show_Topological_Polygons->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_topological_network_display()
{
	get_view_state().get_render_settings().set_show_topological_networks(
			action_Show_Topological_Networks->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_raster_display()
{
	get_view_state().get_render_settings().set_show_rasters(
			action_Show_Rasters->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_3d_scalar_field_display()
{
	get_view_state().get_render_settings().set_show_3d_scalar_fields(
			action_Show_3D_Scalar_Fields->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_scalar_coverage_display()
{
	get_view_state().get_render_settings().set_show_scalar_coverages(
			action_Show_Scalar_Coverages->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_all_geometries_display()
{
	const bool show_all = action_Show_All_Geometries->isChecked();

	// This will show/hide all geometries in the render settings which will also
	// signal 'handle_render_settings_changed()' to change the individual checkboxes.
	get_view_state().get_render_settings().set_show_all(show_all);
}

void
GPlatesQtWidgets::ViewportWindow::handle_render_settings_changed()
{
	GPlatesGui::RenderSettings &render_settings = get_view_state().get_render_settings();

	// Note: Calling 'setChecked()' does not result in the QAction 'triggered' signal so
	// we don't need to worry about infinite recursion.
	action_Show_Static_Points->setChecked(render_settings.show_static_points());
	action_Show_Static_Lines->setChecked(render_settings.show_static_lines());
	action_Show_Static_Polygons->setChecked(render_settings.show_static_polygons());
	action_Show_Static_Multipoints->setChecked(render_settings.show_static_multipoints());
	action_Show_Velocity_Arrows->setChecked(render_settings.show_velocity_arrows());
	action_Show_Topological_Sections->setChecked(render_settings.show_topological_sections());
	action_Show_Topological_Lines->setChecked(render_settings.show_topological_lines());
	action_Show_Topological_Polygons->setChecked(render_settings.show_topological_polygons());
	action_Show_Topological_Networks->setChecked(render_settings.show_topological_networks());
	action_Show_Rasters->setChecked(render_settings.show_rasters());
	action_Show_3D_Scalar_Fields->setChecked(render_settings.show_3d_scalar_fields());
	action_Show_Scalar_Coverages->setChecked(render_settings.show_scalar_coverages());
}

void
GPlatesQtWidgets::ViewportWindow::enable_stars_display()
{
	get_view_state().set_show_stars(
			action_Show_Stars->isChecked());
	if (reconstruction_view_widget().globe_is_active())
	{
		globe_canvas().update_canvas();
	}
}

void
GPlatesQtWidgets::ViewportWindow::update_tools_and_status_message()
{	
// FIXME: 
// There seems to be some sequencing bug where these actions_ never get enabled?
// for now, just comment out ... 
#if 0
	bool globe_is_active = d_reconstruction_view_widget_ptr->globe_is_active();
	action_Show_Arrow_Decorations->setEnabled(globe_is_active);
	action_Show_Stars->setEnabled(globe_is_active);
#endif
}


void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_up()
{
	d_reconstruction_view_widget_ptr->active_view().move_camera_up();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_down()
{
	d_reconstruction_view_widget_ptr->active_view().move_camera_down();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_left()
{
	d_reconstruction_view_widget_ptr->active_view().move_camera_left();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_right()
{
	d_reconstruction_view_widget_ptr->active_view().move_camera_right();
}

void
GPlatesQtWidgets::ViewportWindow::handle_rotate_camera_clockwise()
{
	d_reconstruction_view_widget_ptr->active_view().rotate_camera_clockwise();
}

void
GPlatesQtWidgets::ViewportWindow::handle_rotate_camera_anticlockwise()
{
	d_reconstruction_view_widget_ptr->active_view().rotate_camera_anticlockwise();
}

void
GPlatesQtWidgets::ViewportWindow::handle_reset_camera_orientation()
{
	d_reconstruction_view_widget_ptr->active_view().reset_camera_orientation();
}

void
GPlatesQtWidgets::ViewportWindow::handle_canvas_tool_activated(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool)
{
	// Switch to the proper task panel tab depending on which canvas tool was just activated.
	switch (tool)
	{
	case GPlatesGui::CanvasToolWorkflows::TOOL_DRAG_GLOBE:
	case GPlatesGui::CanvasToolWorkflows::TOOL_ZOOM_GLOBE:
		// We don't pick a tab - the previous tab from another canvas tool workflow will remain.
		break;

#if 0 // Disable lighting tool until volume visualisation is officially released (in GPlates 1.5)...
	case GPlatesGui::CanvasToolWorkflows::TOOL_CHANGE_LIGHTING:
		d_task_panel_ptr->choose_lighting_tab();
		break;
#endif

	case GPlatesGui::CanvasToolWorkflows::TOOL_CLICK_GEOMETRY:
		d_task_panel_ptr->choose_feature_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYLINE:
		d_task_panel_ptr->choose_digitisation_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_MULTIPOINT:
		d_task_panel_ptr->choose_digitisation_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYGON:
		d_task_panel_ptr->choose_digitisation_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_MOVE_VERTEX:
		d_task_panel_ptr->choose_modify_geometry_tab(true/*enable_move_nearby_vertices*/);
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_DELETE_VERTEX:
		d_task_panel_ptr->choose_modify_geometry_tab(false/*enable_move_nearby_vertices*/);
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_INSERT_VERTEX:
		d_task_panel_ptr->choose_modify_geometry_tab(false/*enable_move_nearby_vertices*/);
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_SPLIT_FEATURE:
		d_task_panel_ptr->choose_modify_geometry_tab(false/*enable_move_nearby_vertices*/);
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_MEASURE_DISTANCE:
		d_task_panel_ptr->choose_measure_distance_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_MANIPULATE_POLE:
		d_task_panel_ptr->choose_modify_pole_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_MOVE_POLE:
		d_task_panel_ptr->choose_move_pole_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_LINE_TOPOLOGY:
		d_task_panel_ptr->choose_topology_tools_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_BOUNDARY_TOPOLOGY:
		d_task_panel_ptr->choose_topology_tools_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_BUILD_NETWORK_TOPOLOGY:
		d_task_panel_ptr->choose_topology_tools_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_EDIT_TOPOLOGY:
		d_task_panel_ptr->choose_topology_tools_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_CREATE_SMALL_CIRCLE:
		d_task_panel_ptr->choose_small_circle_tab();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_SELECT_HELLINGER_GEOMETRIES:
		// NOTE: We don't currently have any hellinger task panels
		// (and we may never have any), so we just open the
		// Hellinger dialog here, unlike most other tools which will
		// activate their associated task panels.
		dialogs().pop_up_and_reposition_hellinger_dialog();
		break;

	case GPlatesGui::CanvasToolWorkflows::TOOL_ADJUST_FITTED_POLE_ESTIMATE:
		dialogs().pop_up_hellinger_dialog();
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


void
GPlatesQtWidgets::ViewportWindow::handle_changed_project_filename(
		boost::optional<QString> project_filename)
{
	// Add the project filename (if any) in the window title.
	if (project_filename)
	{
		// 'completeBaseName()' removes the path and the last extension.
		const QString project_file_display_name = QFileInfo(project_filename.get()).completeBaseName();

		set_window_title(project_file_display_name);
	}
	else
	{
		set_window_title();
	}
}


void
GPlatesQtWidgets::ViewportWindow::show_menu_item_status_tip_in_status_bar()
{
	// Get the QObject that triggered this slot.
	QObject *signal_sender = sender();
	// Return early in case this slot not activated by a signal - shouldn't happen.
	if (!signal_sender)
	{
		return;
	}

	// See if a QAction triggered this slot.
	QAction *action = qobject_cast<QAction *>(signal_sender);
	if (action)
	{
		status_message(action->statusTip());
		return;
	}

	// See if a QMenu triggered this slot.
	QMenu *menu = qobject_cast<QMenu *>(signal_sender);
	if (menu)
	{
		status_message(menu->statusTip());
		return;
	}
}


void
GPlatesQtWidgets::ViewportWindow::install_gui_debug_menu()
{
	// Add the GUI Debug menu and associated functionality.
	// This is okay, we're not bleeding memory out of our ears, Qt parents it
	// to ViewportWindow and cleans up after us. We don't really need to keep
	// a reference to this class around afterwards, which will help keep us
	// be free of header and initialiser list spaghetti.
	static GPlatesGui::GuiDebug *gui_debug =
			new GPlatesGui::GuiDebug(*this, get_view_state(), get_application_state(), this);

	gui_debug->setObjectName("GuiDebug");
}


void
GPlatesQtWidgets::ViewportWindow::handle_colour_scheme_delegator_changed()
{
	d_reconstruction_view_widget_ptr->globe_and_map_widget().update_canvas();
}


void
GPlatesQtWidgets::ViewportWindow::set_window_title(
		boost::optional<QString> project_filename)
{
	QString window_title("GPlates");

	// Append the GPlates version if not an official public release (including pre-release suffix, eg, 2.3.0-dev1).
	// Otherwise just leave as "GPlates" for official public releases.
#if !defined(GPLATES_PUBLIC_RELEASE)  // Flag defined by CMake build system (in "global/config.h").
	const QString FORMAT = " (%1)";
	window_title.append(FORMAT.arg(GPlatesGlobal::Version::get_GPlates_version()));
#endif

	// Add the project filename if there is one.
	if (project_filename)
	{
		window_title.append(" - ");
		window_title.append(project_filename.get());
	}

	setWindowTitle(window_title);
}


void
GPlatesQtWidgets::ViewportWindow::handle_visual_layer_added(
		size_t added)
{
	set_visual_layers_dialog_visibility(true);

	// Disconnect from signal so that we only open the Layers dialog automatically
	// the first time a visual layer is added.
	QObject::disconnect(
			&(get_view_state().get_visual_layers()),
			SIGNAL(layer_added(size_t)),
			this,
			SLOT(handle_visual_layer_added(size_t)));
}


void
GPlatesQtWidgets::ViewportWindow::open_new_window()
{
	QString file_path = QCoreApplication::applicationFilePath();

	// Note that even if we are not interested in passing arguments to the new
	// instance, we need to use this overload:
	//     bool QProcess::startDetached ( const QString & program, const QStringList & arguments )
	// instead of
	//     bool QProcess::startDetached ( const QString & program )
	// because with the latter, the program string contains both the program
	// name and the arguments, separated by whitespace. Thus, it does the wrong
	// thing if the path to the executable has whitespace, e.g. on Windows.
	if (!QProcess::startDetached(file_path, QStringList()))
	{
		qDebug() << "ViewportWindow::open_new_window: new instance could not be started";
	}
}


void
GPlatesQtWidgets::ViewportWindow::status_message(
		const QString &message,
		int timeout)
{
#ifdef Q_OS_MACOS
	static const QString CLOVERLEAF(QChar(0x2318));
	QString fixed_message = message;
	fixed_message.replace(QString("ctrl"), CLOVERLEAF, Qt::CaseInsensitive);
	statusBar()->showMessage(fixed_message, timeout);
#else
	statusBar()->showMessage(message, timeout);
#endif
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_background_colour_picker()
{
	boost::optional<GPlatesGui::Colour> new_colour =
		QtWidgetUtils::get_colour_with_alpha(get_view_state().get_background_colour(), this);
	if (new_colour)
	{
		get_view_state().set_background_colour(*new_colour);
		reconstruction_view_widget().update();
	}
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_import_raster_dialog(
		bool time_dependent_raster)
{
	// Note: the ImportRasterDialog needs to be constructed each time we want to
	// use it, unlike the other dialogs, otherwise the pages are incorrectly initialised.
	ImportRasterDialog import_raster_dialog(
			get_application_state(),
			get_view_state(),
			d_unsaved_changes_tracker_ptr.data(),
			d_file_io_feedback_ptr.data(),
			this);

	ReadErrorAccumulationDialog &read_errors_dialog = dialogs().read_error_accumulation_dialog();

	GPlatesFileIO::ReadErrorAccumulation &read_errors = read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();

	import_raster_dialog.display(time_dependent_raster, &read_errors);

	read_errors_dialog.update();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors)
	{
		read_errors_dialog.show();
	}
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_import_raster_dialog()
{
	pop_up_import_raster_dialog(false);
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_import_time_dependent_raster_dialog()
{
	pop_up_import_raster_dialog(true);
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_import_scalar_field_3d_dialog()
{
	// Note: the ImportScalarField3DDialog needs to be constructed each time we want to
	// use it, unlike the other dialogs, otherwise the pages are incorrectly initialised.
	ImportScalarField3DDialog import_scalar_field_dialog(
			get_application_state(),
			get_view_state(),
			*this,
			d_unsaved_changes_tracker_ptr.data(),
			d_file_io_feedback_ptr.data(),
			this);

	ReadErrorAccumulationDialog &read_errors_dialog = dialogs().read_error_accumulation_dialog();

	GPlatesFileIO::ReadErrorAccumulation &read_errors = read_errors_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();

	import_scalar_field_dialog.display(&read_errors);

	read_errors_dialog.update();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors)
	{
		read_errors_dialog.show();
	}
}


void
GPlatesQtWidgets::ViewportWindow::clone_feature_with_dialog()
{
	ChooseFeatureCollectionDialog &choose_feature_collection_dialog =
			dialogs().choose_feature_collection_dialog();;

	GPlatesModel::FeatureHandle::weak_ref feature_ref = get_view_state().get_feature_focus().focused_feature();
	if (!feature_ref.is_valid())
	{
		return;
	}

	GPlatesModel::FeatureCollectionHandle *feature_collection_ptr = feature_ref->parent_ptr();
	if (!feature_collection_ptr)
	{
		return;
	}

	boost::optional<std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> > dialog_result =
		choose_feature_collection_dialog.get_file_reference(feature_collection_ptr->reference());
	if (dialog_result)
	{
		d_clone_operation_ptr->clone_focused_feature(
				dialog_result->first.get_file().get_feature_collection());
	}
}


void
GPlatesQtWidgets::ViewportWindow::update_undo_action_tooltip()
{
	if (d_inside_update_undo_action_tooltip)
	{
		return;
	}

	d_inside_update_undo_action_tooltip = true;
	add_shortcut_to_tooltip(d_undo_action_ptr);
	d_inside_update_undo_action_tooltip = false;
}


void
GPlatesQtWidgets::ViewportWindow::update_redo_action_tooltip()
{
	if (d_inside_update_redo_action_tooltip)
	{
		return;
	}

	d_inside_update_redo_action_tooltip = true;
	add_shortcut_to_tooltip(d_redo_action_ptr);
	d_inside_update_redo_action_tooltip = false;
}


void
GPlatesQtWidgets::ViewportWindow::open_online_documentation()
{
	QDesktopServices::openUrl(QUrl("http://www.gplates.org/docs.html"));
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_python_console()
{
	d_view_state.get_python_manager().pop_up_python_console();
}


void
GPlatesQtWidgets::ViewportWindow::open_dataset_webpage()
{
	QDesktopServices::openUrl(
			QUrl("http://www.earthbyte.org/gplates-2-5-software-and-data-sets"));
}


void
GPlatesQtWidgets::ViewportWindow::showEvent(
		QShowEvent *ev)
{
	// We wait until the main window is visible before checking our status messages and
	// View-dependent menu items are configured appropriately; this is largely because of
	// the definition of ReconstructionViewWidget::globe_is_active().
	update_tools_and_status_message();
}
