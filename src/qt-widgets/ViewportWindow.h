/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 The University of Sydney, Australia
 * Copyright (C) 2007, 2008, 2009, 2010 Geological Survey of Norway
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

#include <vector>
#include <string>
#include <list>
#include <memory>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <QtCore/QTimer>
#include <QPointer>
#include <QCloseEvent>
#include <QStringList>
#include <QUndoGroup>

#include "ViewportWindowUi.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "canvas-tools/CanvasToolType.h"

#include "gui/AnimationController.h"
#include "gui/FullScreenMode.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileIO;
}

namespace GPlatesCanvasTools
{
	class MeasureDistanceState;
}

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
	class DockState;
	class EnableCanvasTool;
	class FeatureFocus;
	class FeatureTableModel;
	class FileIOFeedback;
	class GlobeCanvasToolAdapter;
	class GlobeCanvasToolChoice;
	class ImportMenu;
	class MapCanvasToolAdapter;
	class MapCanvasToolChoice;
	class SessionMenu;
	class TopologySectionsTable;
	class TopologySectionsContainer;
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
	class ActiveGeometryOperation;
	class CloneOperation;
	class DeleteFeatureOperation;
	class FocusedFeatureGeometryManipulator;
	class GeometryBuilder;
	class GeometryOperationTarget;
}

namespace GPlatesQtWidgets
{
	class AboutDialog;
	class AnimateDialog;
	class AssignReconstructionPlateIdsDialog;
	class CalculateReconstructionPoleDialog;
	class ChooseFeatureCollectionDialog;
	class ColouringDialog;
	class ConnectWFSDialog;
	class ConfigureGraticulesDialog;
	class ConfigureTextOverlayDialog;
	class DataAssociationDialog;
	class DockWidget;
	class CreateVGPDialog;
	class ExportAnimationDialog;
	class FeaturePropertiesDialog;
	class GlobeCanvas;
	class ManageFeatureCollectionsDialog;
	class MapView;
	class MeshDialog;
	class PreferencesDialog;
	class PythonConsoleDialog;
	class ReadErrorAccumulationDialog;
	class ReconstructionViewWidget;
	class SaveFileDialog;
	class SetCameraViewpointDialog;
	class SetProjectionDialog;
	class ShapefileAttributeViewerDialog;
	class SmallCircleManager;
	class SpecifyAnchoredPlateIdDialog;
	class SymbolManagerDialog;
	class TaskPanel;
	class TotalReconstructionPolesDialog;
	class TotalReconstructionSequencesDialog;
	class VisualLayersDialog;

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

		void
		load_files(
				const QStringList &filenames);

		void
		reconstruct_to_time(
				const double &recon_time);

		GPlatesQtWidgets::ReconstructionViewWidget &
		reconstruction_view_widget()
		{
			return *d_reconstruction_view_widget_ptr;
		}

		const GPlatesQtWidgets::ReconstructionViewWidget &
		reconstruction_view_widget() const
		{
			return *d_reconstruction_view_widget_ptr;
		}

		GlobeCanvas &
		globe_canvas();

		const GlobeCanvas &
		globe_canvas() const;

		MapView &
		map_view();

		const MapView &
		map_view() const;

		void
		create_svg_file(
				const QString &filename);


		GPlatesGui::FeatureTableModel &
		feature_table_model() 
		{
			return *d_feature_table_model_ptr;
		}

		GPlatesGui::TrinketArea &
		trinket_area() 
		{
			return *d_trinket_area_ptr;
		}

		/** Get a pointer to the TopologySectionsContainer */
		GPlatesGui::TopologySectionsContainer &
		topology_sections_container()
		{
			return *d_topology_sections_container_ptr;	
		}

		/** Get a pointer to the TaskPanel */
		GPlatesQtWidgets::TaskPanel *
		task_panel_ptr() const
		{
			return d_task_panel_ptr;
		}

		ReadErrorAccumulationDialog &
		read_errors_dialog()
		{
			return *d_read_errors_dialog_ptr;
		}

		TotalReconstructionPolesDialog &
		total_reconstruction_poles_dialog()
		{
			return *d_total_reconstruction_poles_dialog_ptr;
		}

		VisualLayersDialog &
		visual_layers_dialog()
		{
			return *d_visual_layers_dialog_ptr;
		}

		void
		restore_canvas_tool_last_chosen_by_user();

		GPlatesGui::ImportMenu &
		import_menu()
		{
			return *d_import_menu_ptr;
		}

		GPlatesGui::UtilitiesMenu &
		utilities_menu()
		{
			return *d_utilities_menu_ptr;
		}

	public slots:
		
		void
		status_message(
				const QString &message,
				int timeout = 20000);

		/**
		 * Highlights the first row in the "Clicked" feature table.
		 */
		void
		highlight_first_clicked_feature_table_row();

		/**
		 * Highlights the row of the table that corresponds to the focused feature.
		 */
		void
		highlight_focused_feature_in_table();

		void
		enable_drag_globe_tool(
				bool enable = true);

		void
		enable_zoom_globe_tool(
				bool enable = true);

		void
		enable_click_geometry_tool(
				bool enable = true);

		void
		enable_digitise_polyline_tool(
				bool enable = true);

		void
		enable_digitise_multipoint_tool(
				bool enable = true);

		void
		enable_digitise_polygon_tool(
				bool enable = true);

		void
		enable_move_geometry_tool(
				bool enable = true);

		void
		enable_move_vertex_tool(
				bool enable = true);

		void
		enable_delete_vertex_tool(
				bool enable = true);

		void
		enable_insert_vertex_tool(
				bool enable = true);

		void
			enable_split_feature_tool(
			bool enable = true);

		void
		enable_manipulate_pole_tool(
				bool enable = true);

		void
		enable_build_topology_tool(
				bool enable = true);

		void
		enable_edit_topology_tool(
				bool enable = true);

		void
		enable_measure_distance_tool(
				bool enable = true);

		void
		choose_drag_globe_tool();

		void
		choose_zoom_globe_tool();

		void
		choose_click_geometry_tool();

		void
		choose_digitise_polyline_tool();

		void
		choose_digitise_multipoint_tool();

		void
		choose_digitise_polygon_tool();

		void
		choose_move_geometry_tool();

		void
		choose_move_vertex_tool();

		void
		choose_delete_vertex_tool();

		void
		choose_insert_vertex_tool();

		void
		choose_split_feature_tool();

		void
		choose_manipulate_pole_tool();

		void
		choose_build_topology_tool();

		void
		choose_edit_topology_tool();

		void
		choose_measure_distance_tool();

		void
		enable_or_disable_feature_actions(
				GPlatesGui::FeatureFocus &feature_focus);

		void
		choose_clicked_geometry_table();

		void
		choose_topology_sections_table();

		void
		pop_up_read_errors_dialog();

		void
		pop_up_manage_feature_collections_dialog();

		void
		pop_up_export_animation_dialog();

		void
		pop_up_assign_reconstruction_plate_ids_dialog();

		void
		pop_up_total_reconstruction_poles_dialog();

		void
		pop_up_total_reconstruction_poles_dialog(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);
	
		void
		pop_up_animate_dialog();

		void
		pop_up_preferences_dialog();

		void
		pop_up_configure_text_overlay_dialog();

		void
		pop_up_configure_graticules_dialog();

		void
		pop_up_colouring_dialog();

		void
		handle_load_symbol_file();

		void 
		handle_unload_symbol_file();

		void
		pop_up_symbol_manager_dialog();

		void
		pop_up_connect_wfs();

		void
		update_tools_and_status_message();

		void
		use_small_canvas_tool_icons(
				bool use_small_icons);

		void
		handle_read_errors(
				GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io,
				const GPlatesFileIO::ReadErrorAccumulation &new_read_errors);

		/**
		 * Add secret menu filled with actions aid GUI-related debugging.
		 * Triggered from gplates_main and commandline switch --debug-gui.
		 */
		void
		install_gui_debug_menu();

		void
		install_data_mining_menu();

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

		//! Returns the application state.
		GPlatesAppLogic::ApplicationState &
		get_application_state();


		//! Returns the view state.
		GPlatesPresentation::ViewState &
		get_view_state();

		/**
		 * Temporarily enables or disables a reconstruction tree by adding or
		 * removing it from the list of active reconstruction files. This does not
		 * un-load the file.
		 * Connects Signal/Slots for the map- and globe- canvas tools.
		 */
		void
		connect_canvas_tools();

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
		 * Connects signals of @a FeatureCollectionFileIO to slots of 'this'.
		 */
		void
		connect_feature_collection_file_io_signals();

		/**
		 * Configures the ActionButtonBox inside the Feature tab of the Task Panel
		 * with some of the QActions that ViewportWindow has on the menu bar.
		 */
		void
		set_up_task_panel_actions();

		/**
		 * Creates a context menu to allow the user to dock DockWidgets even if
		 * they are misbehaving and not docking when dragged over the main window.
		 */
		void
		set_up_dock_context_menus();

		void
		set_internal_release_window_title();

	private slots:

		void
		pop_up_specify_anchored_plate_id_dialog();

		void
		pop_up_set_camera_viewpoint_dialog();
		
		void
		pop_up_about_dialog();

		void
		set_visual_layers_dialog_visibility(
				bool visible);

		void
		handle_window_menu_about_to_show();

		void
		close_all_dialogs();
		
		void
		dock_search_results_at_top();
		
		void
		dock_search_results_at_bottom();

		void
		enable_point_display();

		void
		enable_line_display();

		void
		enable_polygon_display();

		void
		enable_multipoint_display();

		void
		enable_arrows_display();

		void
		enable_stars_display();

		void
		pop_up_shapefile_attribute_viewer_dialog();

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
		pop_up_set_projection_dialog();
		
		void
		pop_up_create_vgp_dialog();
		
		void
		generate_mesh_cap();

		void
		pop_up_calculate_reconstruction_pole_dialog();

		void
		handle_colour_scheme_delegator_changed();
		
		void
		pop_up_import_raster_dialog();

		void
		pop_up_import_raster_dialog(
				bool time_dependent_raster);

		void
		pop_up_import_time_dependent_raster_dialog();

		void
		pop_up_total_reconstruction_sequences_dialog();

		void
		pop_up_data_association_dialog();

		void
		handle_visual_layer_added(
				size_t index);

		void
		handle_drag_globe_triggered();

		void
		handle_zoom_globe_triggered();

		void
		handle_click_geometry_triggered();

		void
		handle_digitise_new_polyline_triggered();

		void
		handle_digitise_new_multipoint_triggered();

		void
		handle_digitise_new_polygon_triggered();

		void
		handle_move_vertex_triggered();

		void
		handle_delete_vertex_triggered();

		void
		handle_insert_vertex_triggered();

		void
		handle_split_feature_triggered();

		void
		handle_manipulate_pole_triggered();

		void
		handle_build_topology_triggered();

		void
		handle_edit_topology_triggered();

		void
		handle_measure_distance_triggered();

		void
		open_new_window();

		void
		pop_up_small_circle_manager();

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
		handle_python_console_text_changed();

	private:

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;

		GPlatesGui::AnimationController d_animation_controller;
		GPlatesGui::FullScreenMode d_full_screen_mode;

		//! Manages the icons in the status bar
		boost::scoped_ptr<GPlatesGui::TrinketArea> d_trinket_area_ptr;

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
		 * Deals with all the micro-management of the ViewportWindow's docks.
		 */
		QPointer<GPlatesGui::DockState> d_dock_state_ptr;

		/**
		 * A temporary position for the new DockWidget for Clicked/Topology
		 * while I refactor it out of ViewportWindow --JC
		 */
		QPointer<DockWidget> d_info_dock_ptr;

		ReconstructionViewWidget *d_reconstruction_view_widget_ptr;

		boost::scoped_ptr<AboutDialog> d_about_dialog_ptr;
		boost::scoped_ptr<AnimateDialog> d_animate_dialog_ptr;
		boost::scoped_ptr<AssignReconstructionPlateIdsDialog> d_assign_recon_plate_ids_dialog_ptr;
		boost::scoped_ptr<CalculateReconstructionPoleDialog> d_calculate_reconstruction_pole_dialog_ptr;
		boost::scoped_ptr<ChooseFeatureCollectionDialog> d_choose_feature_collection_dialog_ptr;
		boost::scoped_ptr<ColouringDialog> d_colouring_dialog_ptr;
		boost::scoped_ptr<ConnectWFSDialog> d_connect_wfs_dialog_ptr;
		boost::scoped_ptr<ConfigureGraticulesDialog> d_configure_graticules_dialog_ptr;
		boost::scoped_ptr<ConfigureTextOverlayDialog> d_configure_text_overlay_dialog_ptr;
		boost::scoped_ptr<DataAssociationDialog> d_data_association_dialog_ptr;
		boost::scoped_ptr<CreateVGPDialog> d_create_vgp_dialog_ptr;
		boost::scoped_ptr<ExportAnimationDialog> d_export_animation_dialog_ptr;
		boost::scoped_ptr<FeaturePropertiesDialog> d_feature_properties_dialog_ptr;
		boost::scoped_ptr<ManageFeatureCollectionsDialog> d_manage_feature_collections_dialog_ptr;
		boost::scoped_ptr<MeshDialog> d_mesh_dialog_ptr;
		boost::scoped_ptr<PreferencesDialog> d_preferences_dialog_ptr;
		boost::scoped_ptr<PythonConsoleDialog> d_python_console_dialog_ptr;
		boost::scoped_ptr<ReadErrorAccumulationDialog> d_read_errors_dialog_ptr;
		boost::scoped_ptr<SetCameraViewpointDialog> d_set_camera_viewpoint_dialog_ptr;
		boost::scoped_ptr<SetProjectionDialog> d_set_projection_dialog_ptr;
		boost::scoped_ptr<ShapefileAttributeViewerDialog> d_shapefile_attribute_viewer_dialog_ptr;
		boost::scoped_ptr<SmallCircleManager> d_small_circle_manager_ptr;
		boost::scoped_ptr<SpecifyAnchoredPlateIdDialog> d_specify_anchored_plate_id_dialog_ptr;
		boost::scoped_ptr<SymbolManagerDialog> d_symbol_manager_dialog_ptr;
		boost::scoped_ptr<TotalReconstructionPolesDialog> d_total_reconstruction_poles_dialog_ptr;
		boost::scoped_ptr<TotalReconstructionSequencesDialog> d_total_reconstruction_sequences_dialog_ptr;
		boost::scoped_ptr<VisualLayersDialog> d_visual_layers_dialog_ptr;

		boost::shared_ptr<SaveFileDialog> d_export_geometry_snapshot_dialog_ptr;

		boost::scoped_ptr<GPlatesGui::ChooseCanvasTool> d_choose_canvas_tool;

		boost::scoped_ptr<GPlatesViewOperations::GeometryBuilder> d_digitise_geometry_builder;

		boost::scoped_ptr<GPlatesViewOperations::GeometryBuilder> d_focused_feature_geometry_builder;

		// Depends on d_digitise_geometry_builder, d_focused_feature_geometry_builder,
		// d_geometry_operation_target.
		boost::scoped_ptr<GPlatesViewOperations::GeometryOperationTarget> d_geometry_operation_target;

		boost::scoped_ptr<GPlatesViewOperations::CloneOperation> d_clone_operation_ptr;

		boost::scoped_ptr<GPlatesViewOperations::DeleteFeatureOperation> d_delete_feature_operation_ptr;

		boost::scoped_ptr<GPlatesViewOperations::ActiveGeometryOperation> d_active_geometry_operation;

		// Depends on d_focused_feature_geometry_builder.
		boost::scoped_ptr<GPlatesViewOperations::FocusedFeatureGeometryManipulator>
				d_focused_feature_geom_manipulator;

		// Depends on d_geometry_operation_target, d_choose_canvas_tool.
		boost::scoped_ptr<GPlatesGui::EnableCanvasTool> d_enable_canvas_tool;

		// Depends on d_geometry_operation_target.
		boost::scoped_ptr<GPlatesCanvasTools::MeasureDistanceState> d_measure_distance_state_ptr;

		//! The data behind the Topology Sections table.
		boost::scoped_ptr<GPlatesGui::TopologySectionsContainer> d_topology_sections_container_ptr;

		/**
		 * Manages the 'Topology Sections' table, and is parented to it - Qt will clean up when the table disappears!
		 * Depends on d_topology_sections_container_ptr.
		 */
		boost::scoped_ptr<GPlatesGui::TopologySectionsTable> d_topology_sections_table_ptr;

		//! The 'Clicked' table. Should be in ViewState. Depends on FeatureFocus.		
		boost::scoped_ptr<GPlatesGui::FeatureTableModel> d_feature_table_model_ptr;

		//
		// Tool Adapter and Choice for the Globe.
		//

		// Depends on d_topology_sections_container_ptr, d_feature_table_model_ptr,
		// d_measure_distance_state_ptr, d_feature_table_model_ptr, d_feature_properties_dialog,
		// d_geometry_operation_target, d_active_geometry_operation, d_choose_canvas_tool.
		boost::scoped_ptr<GPlatesGui::GlobeCanvasToolChoice> d_globe_canvas_tool_choice_ptr;
		// Depends on d_globe_canvas_tool_choice_ptr.
		boost::scoped_ptr<GPlatesGui::GlobeCanvasToolAdapter> d_globe_canvas_tool_adapter_ptr;

		//
		// Tool Adapter and Choice for the Map.
		//

		// Depends on d_topology_sections_container_ptr, d_feature_table_model_ptr,
		// d_measure_distance_state_ptr, d_feature_table_model_ptr, d_feature_properties_dialog,
		// d_geometry_operation_target, d_active_geometry_operation, d_choose_canvas_tool.
		boost::scoped_ptr<GPlatesGui::MapCanvasToolChoice> d_map_canvas_tool_choice_ptr;
		// Depends on d_map_canvas_tool_choice_ptr.
		boost::scoped_ptr<GPlatesGui::MapCanvasToolAdapter> d_map_canvas_tool_adapter_ptr;


		/**
		 * Depends on FeatureFocus, Model, topology sections container.
		 * Is parented by 'this' - Qt will clean up when 'this' is destroyed.
		 */
		TaskPanel *d_task_panel_ptr;

		/**
		 * The last canvas tool explicitly chosen by the user (i.e. not the
		 * result of an automatic switch of canvas tool by GPlates code).
		 *
		 * In particular, this is used to switch back to the Click Geometry tool
		 * after the user clicks "Clone Geometry" and completes the "Create
		 * Feature" dialog; this is because "Clone Geometry" automatically takes
		 * the user to a digitisation tool.
		 */
		GPlatesCanvasTools::CanvasToolType::Value d_canvas_tool_last_chosen_by_user;

		QAction *d_undo_action_ptr;

		QAction *d_redo_action_ptr;

		// To prevent infinite loops.
		bool d_inside_update_undo_action_tooltip;
		bool d_inside_update_redo_action_tooltip;

		/**
		 * Encapsulates logic regarding the Import submenu of the File menu.
		 */
		GPlatesGui::ImportMenu *d_import_menu_ptr;

		/**
		 * Allows Python scripts to be run from the Utilities menu.
		 */
		GPlatesGui::UtilitiesMenu *d_utilities_menu_ptr;
	};
}

#endif  // GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
