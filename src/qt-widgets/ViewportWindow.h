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
 
#ifndef GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
#define GPLATES_QTWIDGETS_VIEWPORTWINDOW_H

#include <list>
#include <memory>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <QtCore/QTimer>
#include <QCloseEvent>
#include <QMainWindow>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QUndoGroup>

#include "ui_ViewportWindowUi.h"

#include "canvas-tools/CanvasTool.h"

#include "gui/CanvasToolWorkflows.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileIO;
}

namespace GPlatesCanvasTools
{
	class GeometryOperationState;
	class MeasureDistanceState;
	class ModifyGeometryState;
}

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
}

namespace GPlatesGui
{
	class Dialogs;
	class DockState;
	class EnableCanvasTool;
	class FeatureFocus;
	class FileIOFeedback;
	class FullScreenMode;
	class ImportMenu;
	class SessionMenu;
	class TrinketArea;
	class UnsavedChangesTracker;
	class UtilitiesMenu;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
}

namespace GPlatesViewOperations
{
	class CloneOperation;
	class DeleteFeatureOperation;
}

namespace GPlatesQtWidgets
{
	class CanvasToolBarDockWidget;
	class DockWidget;
	class GlobeCanvas;
	class MapView;
	class PythonConsoleDialog;
	class ReconstructionViewWidget;
	class SearchResultsDockWidget;
	class SmallCircleManager;
	class TaskPanel;

	class ViewportWindow :
			public QMainWindow, 
			protected Ui_ViewportWindow
	{
		Q_OBJECT
		
	public:

		explicit
		ViewportWindow(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state);

		virtual
		~ViewportWindow();


		/**
		 * Loads the specified project file as a convenient alternative to having to
		 * explicitly load it by accessing the GUI.
		 */
		void
		load_project(
				const QString &project_filename);


		/**
		 * Loads the specified feature collection files as a convenient alternative to having to
		 * explicitly load them by accessing the GUI.
		 */
		void
		load_feature_collections(
				const QStringList &filenames);


		/**
		 * Shows the main window.
		 *
		 * Internally this calls QMainWindow::show() and then calls functions that rely
		 * on the main window being visible (such as activating the default canvas tool which
		 * checks for visibility of the main view canvas).
		 */
		void
		display();


		//! Returns the application state.
		GPlatesAppLogic::ApplicationState &
		get_application_state();

		//! Returns the view state.
		GPlatesPresentation::ViewState &
		get_view_state();

		ReconstructionViewWidget &
		reconstruction_view_widget();

		const ReconstructionViewWidget &
		reconstruction_view_widget() const;

		CanvasToolBarDockWidget &
		canvas_tool_bar_dock_widget();

		SearchResultsDockWidget &
		search_results_dock_widget();

		GlobeCanvas &
		globe_canvas();

		const GlobeCanvas &
		globe_canvas() const;

		MapView &
		map_view();

		const MapView &
		map_view() const;
		
		
		/**
		 * Accessor for the Dialogs class which manages all the instances of major dialogs/windows
		 * that would ordinarily hang off of ViewportWindow and clutter things up.
		 */
		GPlatesGui::Dialogs &
		dialogs() const;

		GPlatesGui::FileIOFeedback &
		file_io_feedback();

		GPlatesGui::CanvasToolWorkflows &
		canvas_tool_workflows();


		GPlatesGui::TrinketArea &
		trinket_area();

		/** Get a pointer to the TaskPanel */
		TaskPanel *
		task_panel_ptr() const;

		GPlatesGui::ImportMenu &
		import_menu();

		GPlatesGui::UtilitiesMenu &
		utilities_menu();

	public Q_SLOTS:
		
		void
		status_message(
				const QString &message,
				int timeout = 20000);

		void
		enable_or_disable_feature_actions(
				GPlatesGui::FeatureFocus &feature_focus);

		void
		handle_load_symbol_file();

		void 
		handle_unload_symbol_file();

		void
		update_tools_and_status_message();

		void
		handle_read_errors(
				const GPlatesFileIO::ReadErrorAccumulation &new_read_errors);

		/**
		 * Add secret menu filled with actions aid GUI-related debugging.
		 * Triggered from gplates_main and commandline switch --debug-gui.
		 */
		void
		install_gui_debug_menu();

		void
		hide_symbol_menu()
		{
			action_Load_Symbol->setVisible(false);
			action_Unload_Symbol->setVisible(false);
		}

		void
		hide_python_menu()
		{
			action_Open_Python_Console->setVisible(false);
		}

	protected:
	
		/**
		 * A reimplementation of QWidget::closeEvent() to allow closure to be postponed.
		 * To request program termination in the same manner as using the window manager's
		 * 'close' button, you should call ViewportWindow::close().
		 */
		virtual
		void
		closeEvent(QCloseEvent *close_event);

		/**
		 * Reimplementation of drag/drop events so we can handle users dragging files onto
		 * GPlates main window.
		 */
		virtual
		void
		dragEnterEvent(
				QDragEnterEvent *ev);

		/**
		 * Reimplementation of drag/drop events so we can handle users dragging files onto
		 * GPlates main window.
		 */
		virtual
		void
		dropEvent(
				QDropEvent *ev);


		virtual
		void
		showEvent(
				QShowEvent *ev);

	private:

		/**
		 * Connects all the Signal/Slot relationships for ViewportWindow toolbar
		 * buttons and menu items.
		 */
		void
		connect_menu_actions();

		void
		connect_file_menu_actions();

		void
		connect_edit_menu_actions();

		void
		connect_view_menu_actions();

		void
		connect_features_menu_actions();

		void
		connect_reconstruction_menu_actions();

		void
		connect_utilities_menu_actions();

		void
		connect_tools_menu_actions();

		void
		connect_window_menu_actions();

		void
		connect_help_menu_actions();

		/**
		 * Copies the menu structure found in ViewportWindow's menu bar into the
		 * special full-screen-mode 'GMenu' button.
		 */
		void
		populate_gmenu_from_menubar();

		/**
		 * Configures the ActionButtonBox inside the Feature tab of the Task Panel
		 * with some of the QActions that ViewportWindow has on the menu bar.
		 */
		void
		set_up_task_panel_actions();

		void
		set_window_title(
				boost::optional<QString> project_filename = boost::none);

	private Q_SLOTS:

		void
		set_visual_layers_dialog_visibility(
				bool visible);

		void
		handle_window_menu_about_to_show();

		void
		enable_static_point_display();

		void
		enable_static_line_display();

		void
		enable_static_polygon_display();

		void
		enable_static_multipoint_display();

		void
		enable_velocity_arrow_display();

		void
		enable_topological_section_display();

		void
		enable_topological_line_display();

		void
		enable_topological_polygon_display();

		void
		enable_topological_network_display();

		void
		enable_raster_display();

		void
		enable_3d_scalar_field_display();

		void
		enable_scalar_coverage_display();

		void
		enable_all_geometries_display();

		void
		handle_render_settings_changed();

		void
		enable_stars_display();

		void
		handle_move_camera_up();

		void
		handle_move_camera_down();

		void
		handle_move_camera_left();

		void
		handle_move_camera_right();

		void
		handle_rotate_camera_clockwise();

		void
		handle_rotate_camera_anticlockwise();

		void
		handle_reset_camera_orientation();

		void
		handle_canvas_tool_activated(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool);

		void
		handle_changed_project_filename(
				boost::optional<QString> project_filename);

		void
		show_menu_item_status_tip_in_status_bar();

		void
		pop_up_import_raster_dialog();

		void
		pop_up_import_raster_dialog(
				bool time_dependent_raster);

		void
		pop_up_import_time_dependent_raster_dialog();

		void
		pop_up_import_scalar_field_3d_dialog();

		void
		handle_colour_scheme_delegator_changed();

		void
		handle_visual_layer_added(
				size_t index);

		void
		open_new_window();

		void
		pop_up_background_colour_picker();

		void
		clone_feature_with_dialog();

		void
		update_undo_action_tooltip();

		void
		update_redo_action_tooltip();

		void
		open_online_documentation();

		void
		pop_up_python_console();

		void
		open_dataset_webpage();
		
	private:

		//
		// Some pointers below are QPointer and some are boost::scoped_ptr.
		//
		// QPointer is used when deletion of the object is taken care of because it has a parent
		// (and the object inherits from QObject since QPointer only works with QObject derivations).
		//
		// boost::scoped_ptr is used when the object must be deleted because no one owns it.
		// The object could be a regular object or one derived from QObject.
		// In the case of QObject it must be explicitly deleted because it has no parent.
		//
		// Note: Apparently the Qt memory management system can detect if an object
		// (derived from QObject) has already been deleted (prematurely) and avoid a double-delete.
		// It's a nice safeguard in case of a programming error, but it shouldn't be relied upon.
		//

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;

		//! The state targeted by geometry operation canvas tools and displayed in task panel.
		boost::scoped_ptr<GPlatesCanvasTools::GeometryOperationState> d_geometry_operation_state_ptr;

		//! The state targeted by canvas tools that modify geometry and displayed in task panel.
		boost::scoped_ptr<GPlatesCanvasTools::ModifyGeometryState> d_modify_geometry_state;

		//! The state targeted by measure distance canvas tool and displayed in task panel.
		boost::scoped_ptr<GPlatesCanvasTools::MeasureDistanceState> d_measure_distance_state_ptr;

		//! The selected canvas tool state.
		boost::scoped_ptr<GPlatesGui::CanvasToolWorkflows> d_canvas_tool_workflows;

		//! For cloning a feature.
		boost::scoped_ptr<GPlatesViewOperations::CloneOperation> d_clone_operation_ptr;

		//! For deleting a feature.
		boost::scoped_ptr<GPlatesViewOperations::DeleteFeatureOperation> d_delete_feature_operation_ptr;


		/**
		 * Manages all the major dialogs that would otherwise clutter up ViewportWindow.
		 *
		 * NOTE: This is one of the first data members other data members rely on it and
		 * its constructor doesn't initialise any dialog so its creation has few dependencies.
		 */
		QPointer<GPlatesGui::Dialogs> d_dialogs_ptr;

		//! Handles transitions to/from fullscreen mode.
		QPointer<GPlatesGui::FullScreenMode> d_full_screen_mode;

		//! Manages the icons in the status bar
		QPointer<GPlatesGui::TrinketArea> d_trinket_area_ptr;

		/**
		 * Tracks changes to saved/unsaved status of files and manages user notification of same.
		 *
		 * QPointer is a guarded pointer which will be set to null when the QObject it points to
		 * gets deleted; The UnsavedChangesTracker is parented to ViewportWindow, so the Qt
		 * object system handles cleanup, and so that I have easier access to it via GuiDebug.
		 */
		QPointer<GPlatesGui::UnsavedChangesTracker> d_unsaved_changes_tracker_ptr;

		/**
		 * Wraps file loading and saving, opening dialogs appropriately for filenames and error feedback.
		 * Can later provide save/load progress reports to progress bars in GUI.
		 *
		 * QPointer is a guarded pointer which will be set to null when the QObject it points to
		 * gets deleted; The FileIOFeedback is parented to ViewportWindow, so the Qt
		 * object system handles cleanup, and so that I have easier access to it via GuiDebug.
		 */
		QPointer<GPlatesGui::FileIOFeedback> d_file_io_feedback_ptr;

		/**
		 * Manages the Open Recent Session menu.
		 */
		QPointer<GPlatesGui::SessionMenu> d_session_menu_ptr;

		/**
		 * Encapsulates logic regarding the Import submenu of the File menu.
		 */
		QPointer<GPlatesGui::ImportMenu> d_import_menu_ptr;

		/**
		 * Allows Python scripts to be run from the Utilities menu.
		 */
		QPointer<GPlatesGui::UtilitiesMenu> d_utilities_menu_ptr;

		/**
		 * Deals with all the micro-management of the ViewportWindow's docks.
		 */
		QPointer<GPlatesGui::DockState> d_dock_state_ptr;

		/**
		 * A tabbed search results dock widget.
		 */
		QPointer<SearchResultsDockWidget> d_search_results_dock_ptr;

		/**
		 * A tabbed toolbar for the canvas tools.
		 */
		QPointer<CanvasToolBarDockWidget> d_canvas_tools_dock_ptr;

		/**
		 * The central widget in the main window containing everything except the menubar,
		 * search results dock and canvas tools dock.
		 */
		QPointer<ReconstructionViewWidget> d_reconstruction_view_widget_ptr;

		/**
		 * Depends on FeatureFocus, Model, topology sections container.
		 * Is parented by 'this' - Qt will clean up when 'this' is destroyed.
		 */
		QPointer<TaskPanel> d_task_panel_ptr;

		QPointer<QAction> d_undo_action_ptr;

		QPointer<QAction> d_redo_action_ptr;

		// To prevent infinite loops.
		bool d_inside_update_undo_action_tooltip;
		bool d_inside_update_redo_action_tooltip;
	};
}

#endif  // GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
