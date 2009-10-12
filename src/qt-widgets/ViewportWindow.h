/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <vector>
#include <string>
#include <list>
#include <memory>
#include <boost/scoped_ptr.hpp>
#include <QtCore/QTimer>
#include <QCloseEvent>
#include <QStringList>
#include <QUndoGroup>

#include "AboutDialog.h"
#include "AnimateDialog.h"
#include "ExportAnimationDialog.h"
#include "ExportReconstructedFeatureGeometryDialog.h"
#include "FeaturePropertiesDialog.h"
#include "LicenseDialog.h"
#include "ManageFeatureCollectionsDialog.h"
#include "ReadErrorAccumulationDialog.h"
#include "ReconstructionViewWidget.h"
#include "SetCameraViewpointDialog.h"
#include "SetProjectionDialog.h"
#include "SetRasterSurfaceExtentDialog.h"
#include "SpecifyAnchoredPlateIdDialog.h"
#include "SpecifyTimeIncrementDialog.h"
#include "TaskPanel.h"
#include "TotalReconstructionPolesDialog.h"
#include "ShapefileAttributeViewerDialog.h"
#include "ViewportWindowUi.h"

#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/PlateVelocities.h"

#include "file-io/FeatureCollectionFileFormat.h"

#include "gui/AnimationController.h"
#include "gui/ChooseCanvasTool.h"
#include "gui/ColourTable.h"
#include "gui/EnableCanvasTool.h"
#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"
#include "gui/FullScreenMode.h"

#include "maths/GeometryOnSphere.h"

#include "model/ModelInterface.h"
#include "model/types.h"

#include "view-operations/ActiveGeometryOperation.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/FocusedFeatureGeometryManipulator.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesCanvasTools
{
	class MeasureDistanceState;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
	class GlobeCanvasToolAdapter;
	class GlobeCanvasToolChoice;
	class MapCanvasToolAdapter;
	class MapCanvasToolChoice;
	class TopologySectionsTable;
	class TopologySectionsContainer;
}

namespace GPlatesPresentation
{
	class ViewState;
};

namespace GPlatesPresentation
{
	class Application;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow:
			public QMainWindow, 
			protected Ui_ViewportWindow
	{
		Q_OBJECT
		
	public:
		ViewportWindow(
				GPlatesPresentation::Application &application);

		~ViewportWindow();


		void
		load_files(
				const QStringList &filenames);


		void
		reconstruct_to_time_with_root(
				double recon_time,
				unsigned long recon_root);
		

		const GPlatesQtWidgets::ReconstructionViewWidget &
		reconstruction_view_widget() const
		{
			return d_reconstruction_view_widget;
		}

		GlobeCanvas &
		globe_canvas() const
		{
			return *d_globe_canvas_ptr;
		}

		void
		create_svg_file();

		void
		create_svg_file(
				const QString &filename);


		void	
		change_tab(int i) {
			tabWidget->setCurrentIndex( i );
		}

		int
		get_tab() {
			return tabWidget->currentIndex();
		}


		GPlatesGui::FeatureTableModel &
		feature_table_model() 
		{
			return *d_feature_table_model_ptr;
		}

		/** Get a pointer to the TopologySectionsContainer */
		GPlatesGui::TopologySectionsContainer &
		topology_sections_container()
		{
			return *d_topology_sections_container_ptr;	
		}

		/** Get a pointer to the TaskPanel */
		GPlatesQtWidgets::TaskPanel *
		task_panel_ptr()
		{
			return d_task_panel_ptr;
		}



	public slots:
		
		void
		status_message(
				const QString &message,
				int timeout = 20000) const
		{
			statusBar()->showMessage(message, timeout);
		}

		/**
		 * Highlights the first row in the "Clicked" feature table.
		 */
		void
		highlight_first_clicked_feature_table_row() const;

		void
		handle_reconstruction();

		void
		pop_up_license_dialog();

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
		choose_manipulate_pole_tool();

		void
		choose_build_topology_tool();

		void
		choose_edit_topology_tool();

		void
		choose_measure_distance_tool();

		void
		enable_or_disable_feature_actions(
				GPlatesModel::FeatureHandle::weak_ref focused_feature);

		void		
		choose_colour_by_plate_id();

		void
		choose_colour_by_single_colour();
		
		void	
		choose_colour_by_feature_type();

		void
		choose_colour_by_age();

		void
		choose_clicked_geometry_table() const
		{
			tabWidget->setCurrentWidget(tab_clicked);
		}

		void
		choose_selected_feature_table()
		{
			tabWidget->setCurrentWidget(tab_selected);
		}

		void
		choose_topology_sections_table()
		{
			tabWidget->setCurrentWidget(tab_topology);
		}

		void
		pop_up_read_errors_dialog();

		void
		pop_up_manage_feature_collections_dialog();

		void
		pop_up_export_geometry_snapshot_dialog()
		{
			create_svg_file();
		}

		void
		pop_up_export_animation_dialog();

		void
		pop_up_export_reconstruction_dialog();

		void
		pop_up_total_reconstruction_poles_dialog();
	
		void
		pop_up_animate_dialog();

		void
		open_raster();

		void
		open_time_dependent_raster_sequence();
		
		// FIXME: Should be a ViewState operation, or /somewhere/ better than this.
		void
		delete_focused_feature();

		void
		update_tools_and_status_message();


		void
		handle_read_errors(
				GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_io,
				const GPlatesFileIO::ReadErrorAccumulation &new_read_errors);

		void
		end_add_feature_collections(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_begin,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_end);

		void
		end_remove_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state);


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


	private:
		//! Holds application state and view state.
		GPlatesPresentation::Application &d_application;

		/**
		 * Is the reconstruction engine/framework.
		 * Currently must be declared after 'd_rendered_geom_collection' because
		 * it contains objects created by it.
		 */
		GPlatesModel::Reconstruction::non_null_ptr_type d_reconstruction;

		GPlatesGui::AnimationController d_animation_controller;
		GPlatesGui::FullScreenMode d_full_screen_mode;

		ReconstructionViewWidget d_reconstruction_view_widget;
		AboutDialog d_about_dialog;
		AnimateDialog d_animate_dialog;
		ExportAnimationDialog d_export_animation_dialog;
		TotalReconstructionPolesDialog d_total_reconstruction_poles_dialog;
		FeaturePropertiesDialog d_feature_properties_dialog;
		LicenseDialog d_license_dialog;
		ManageFeatureCollectionsDialog d_manage_feature_collections_dialog;
		ReadErrorAccumulationDialog d_read_errors_dialog;
		SetCameraViewpointDialog d_set_camera_viewpoint_dialog;
		SetRasterSurfaceExtentDialog d_set_raster_surface_extent_dialog;
		SpecifyAnchoredPlateIdDialog d_specify_anchored_plate_id_dialog;
		ExportReconstructedFeatureGeometryDialog d_export_rfg_dialog;
		SpecifyTimeIncrementDialog d_specify_time_increment_dialog;

		SetProjectionDialog d_set_projection_dialog;

		GlobeCanvas *d_globe_canvas_ptr;
		
		// Tool Adapter and Choice for the Globe. 
		boost::scoped_ptr<GPlatesGui::GlobeCanvasToolChoice> d_globe_canvas_tool_choice_ptr;		// Depends on FeatureFocus, because QueryFeature does. Also depends on DigitisationWidget.
		boost::scoped_ptr<GPlatesGui::GlobeCanvasToolAdapter> d_globe_canvas_tool_adapter_ptr;
		GPlatesGui::ChooseCanvasTool d_choose_canvas_tool;
		// Tool Adapter and Choice for the Map. 
		boost::scoped_ptr<GPlatesGui::MapCanvasToolAdapter> d_map_canvas_tool_adapter_ptr;
		boost::scoped_ptr<GPlatesGui::MapCanvasToolChoice> d_map_canvas_tool_choice_ptr;

		GPlatesViewOperations::GeometryBuilder d_digitise_geometry_builder;
		GPlatesViewOperations::GeometryBuilder d_focused_feature_geometry_builder;
		GPlatesViewOperations::GeometryOperationTarget d_geometry_operation_target;
		GPlatesViewOperations::ActiveGeometryOperation d_active_geometry_operation;
		GPlatesGui::EnableCanvasTool d_enable_canvas_tool;
		GPlatesViewOperations::FocusedFeatureGeometryManipulator d_focused_feature_geom_manipulator;

		boost::scoped_ptr<GPlatesCanvasTools::MeasureDistanceState> d_measure_distance_state_ptr;

		TaskPanel *d_task_panel_ptr;	// Depends on FeatureFocus and the Model d_model_ptr.
		ShapefileAttributeViewerDialog d_shapefile_attribute_viewer_dialog;

		boost::scoped_ptr<GPlatesGui::FeatureTableModel> d_feature_table_model_ptr;	// The 'Clicked' table. Should be in ViewState. Depends on FeatureFocus.		

		boost::scoped_ptr<GPlatesGui::TopologySectionsContainer> d_topology_sections_container_ptr;	// The data behind the Topology Sections table.
		GPlatesGui::TopologySectionsTable *d_topology_sections_table_ptr;	// Manages the 'Topology Sections' table, and is parented to it - Qt will clean up when the table disappears!

		//  map a time value to a raster filename
		QMap<int,QString> d_time_dependent_raster_map;

		// The last path used for opening raster files.
		QString d_open_file_path; 
	

		/**
		 * Connects all the Signal/Slot relationships for ViewportWindow toolbar
		 * buttons and menu items.
		 */
		void
		connect_menu_actions();

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
		 * Connects signals of @a FeatureCollectionFileState to slots of 'this'.
		 */
		void
		connect_feature_collection_file_state_signals();

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
		uncheck_all_colouring_tools();

		bool
		load_raster(
			QString filename);

		void
		update_time_dependent_raster();

	private slots:
		void
		pop_up_specify_anchored_plate_id_dialog();

		void
		pop_up_set_camera_viewpoint_dialog();
		
		void
		pop_up_about_dialog();

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
		enable_raster_display();

		void
		pop_up_set_raster_surface_extent_dialog();

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
		
		/**
		 * A secret action triggerable with Ctrl+Shift+? to aid GUI-related debugging.
		 */
		void
		handle_gui_debug_action();

	protected:
	
		/**
		 * A reimplementation of QWidget::closeEvent() to allow closure to be postponed.
		 * To request program termination in the same manner as using the window manager's
		 * 'close' button, you should call ViewportWindow::close().
		 */
		void
		closeEvent(QCloseEvent *close_event);
	};
}

#endif  // GPLATES_QTWIDGETS_VIEWPORTWINDOW_H
