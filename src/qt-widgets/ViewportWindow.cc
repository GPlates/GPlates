/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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
 
#include <iostream>
#include <iterator>
#include <memory>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

#include <QtGlobal>
#include <QFileDialog>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QHeaderView>
#include <QMessageBox>
#include <QColorDialog>
#include <QColor>
#include <QInputDialog>
#include <QProgressBar>
#include <QDockWidget>
#include <QActionGroup>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCoreApplication>
#include <QProcess>

#include "ViewportWindow.h"

#include "AboutDialog.h"
#include "AnimateDialog.h"
#include "ActionButtonBox.h"
#include "AssignReconstructionPlateIdsDialog.h"
#include "CalculateReconstructionPoleDialog.h"
#include "ColouringDialog.h"
#include "CreateFeatureDialog.h"
#include "CreateVGPDialog.h"
#include "ExportAnimationDialog.h"
#include "FeaturePropertiesDialog.h"
#include "GlobeCanvas.h"
#include "GlobeAndMapWidget.h"
#include "ImportRasterDialog.h"
#include "InformationDialog.h"
#include "ManageFeatureCollectionsDialog.h"
#include "MapView.h"
#include "PreferencesDialog.h"
#include "QtWidgetUtils.h"
#include "ReadErrorAccumulationDialog.h"
#include "SaveFileDialog.h"
#include "SetCameraViewpointDialog.h"
#include "SetProjectionDialog.h"
#include "SetVGPVisibilityDialog.h"
#include "ShapefileAttributeViewerDialog.h"
#include "ShapefilePropertyMapper.h"
#include "SmallCircleManager.h"
#include "SpecifyAnchoredPlateIdDialog.h"
#include "TaskPanel.h"
#include "TotalReconstructionPolesDialog.h"
#include "TotalReconstructionSequencesDialog.h"
#include "VisualLayersWidget.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/SessionManagement.h"

#include "canvas-tools/MeasureDistanceState.h"

#include "feature-visitors/GeometryTypeFinder.h"

#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/FileInfo.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/RasterReader.h"
#include "file-io/ShapefileReader.h"
#include "file-io/ErrorOpeningFileForWritingException.h"

#include "global/GPlatesException.h"
#include "global/SubversionInfo.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"

#include "gui/AddClickedGeometriesToFeatureTable.h"
#include "gui/ChooseCanvasTool.h"
#include "gui/EnableCanvasTool.h"
#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"
#include "gui/FileIOFeedback.h"
#include "gui/GlobeCanvasToolAdapter.h"
#include "gui/GlobeCanvasToolChoice.h"
#include "gui/GuiDebug.h"
#include "gui/MapCanvasToolAdapter.h"
#include "gui/MapCanvasToolChoice.h"
#include "gui/ProjectionException.h"
#include "gui/SessionMenu.h"
#include "gui/SvgExport.h"
#include "gui/TopologySectionsContainer.h"
#include "gui/TopologySectionsTable.h"
#include "gui/TrinketArea.h"
#include "gui/UnsavedChangesTracker.h"
#include "gui/ViewportProjection.h"

#include "maths/PolylineIntersections.h"
#include "maths/PointOnSphere.h"
#include "maths/LatLonPoint.h"
#include "maths/InvalidLatLonException.h"
#include "maths/Real.h"

#include "model/Model.h"
#include "model/types.h"

#include "opengl/OpenGLBadAllocException.h"
#include "opengl/OpenGLException.h"

#include "presentation/Application.h"
#include "presentation/ViewState.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/ShapefilePropertyMapper.h"
#include "qt-widgets/DataAssociationDialog.h"

#include "utils/Profile.h"

#include "view-operations/ActiveGeometryOperation.h"
#include "view-operations/CloneOperation.h"
#include "view-operations/DeleteFeatureOperation.h"
#include "view-operations/FocusedFeatureGeometryManipulator.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryParameters.h"
#include "view-operations/UndoRedo.h"

#include "MeshDialog.h"

//Data-mining temporary code
extern bool enable_data_mining;

void
GPlatesQtWidgets::ViewportWindow::load_files(
		const QStringList &filenames)
{
	d_file_io_feedback_ptr->open_files(filenames);
}


void
GPlatesQtWidgets::ViewportWindow::reconstruct_to_time(
		const double &recon_time)
{
	// Setting the reconstruction time will trigger a new reconstruction.
	get_application_state().set_reconstruction_time(recon_time);
}


void
GPlatesQtWidgets::ViewportWindow::handle_read_errors(
		GPlatesAppLogic::FeatureCollectionFileIO &,
		const GPlatesFileIO::ReadErrorAccumulation &new_read_errors)
{
	// Pop up errors only if we have new read errors.
	if (new_read_errors.is_empty())
	{
		return;
	}

	// Populate the dialog.
	d_read_errors_dialog_ptr->clear();
	d_read_errors_dialog_ptr->read_errors().accumulate(new_read_errors);
	d_read_errors_dialog_ptr->update();
	
	// At this point we can either throw the dialog in the user's face,
	// or pop up a small icon in the status bar which they can click to see the errors.
	// How do we decide? Well, until we get UserPreferences, let's just pop up the icon
	// on warnings, and show the whole dialog on any kind of real error.
	GPlatesFileIO::ReadErrors::Severity severity = new_read_errors.most_severe_error_type();
	if (severity > GPlatesFileIO::ReadErrors::Warning) {
		d_read_errors_dialog_ptr->show();
	} else {
		d_trinket_area_ptr->read_errors_trinket().setVisible(true);
	}
}


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
}


// ViewportWindow constructor
GPlatesQtWidgets::ViewportWindow::ViewportWindow(
		GPlatesPresentation::Application &application) :
	d_application(application),
	d_animation_controller(get_application_state()),
	d_full_screen_mode(*this),
	d_trinket_area_ptr(
			new GPlatesGui::TrinketArea(*this)),
	d_unsaved_changes_tracker_ptr(
			new GPlatesGui::UnsavedChangesTracker(
				*this,
				get_application_state().get_feature_collection_file_state(),
				get_application_state().get_feature_collection_file_io(),
				this)),
	d_file_io_feedback_ptr(
			new GPlatesGui::FileIOFeedback(
				get_application_state(),
				*this,
				get_view_state().get_feature_focus(),
				this)),
	d_session_menu_ptr(
			new GPlatesGui::SessionMenu(
				get_application_state(),
				*d_file_io_feedback_ptr,
				*d_unsaved_changes_tracker_ptr,
				this)),
	d_reconstruction_view_widget(
			d_animation_controller,
			*this,
			get_view_state(),
			this),
	d_about_dialog_ptr(NULL),
	d_animate_dialog_ptr(NULL),
	d_assign_recon_plate_ids_dialog_ptr(
			new AssignReconstructionPlateIdsDialog(
				get_application_state(), get_view_state(), this)),
	d_calculate_reconstruction_pole_dialog_ptr(NULL),
	d_colouring_dialog_ptr(NULL),
	d_create_vgp_dialog_ptr(NULL),
	d_export_animation_dialog_ptr(NULL),
	d_feature_properties_dialog_ptr(
			new FeaturePropertiesDialog(
				get_view_state(),
				this)),
	d_manage_feature_collections_dialog_ptr(
			new ManageFeatureCollectionsDialog(
				get_application_state().get_feature_collection_file_state(),
				get_application_state().get_feature_collection_file_io(),
				*d_file_io_feedback_ptr,
				get_view_state(),
				this)),
	d_mesh_dialog_ptr(NULL),
	d_read_errors_dialog_ptr(
			new ReadErrorAccumulationDialog(this)),
	d_set_camera_viewpoint_dialog_ptr(NULL),
	d_set_projection_dialog_ptr(NULL),
	d_set_vgp_visibility_dialog_ptr(NULL),
	d_shapefile_attribute_viewer_dialog_ptr(
			new ShapefileAttributeViewerDialog(
				get_application_state().get_feature_collection_file_state(),
				this)),
	d_small_circle_manager_ptr(NULL),
	d_specify_anchored_plate_id_dialog_ptr(
			new SpecifyAnchoredPlateIdDialog(
				this)),
	d_total_reconstruction_poles_dialog_ptr(
			new TotalReconstructionPolesDialog(
				get_view_state(),
				this)),
	d_total_reconstruction_sequences_dialog_ptr(
			new TotalReconstructionSequencesDialog(
				get_application_state().get_feature_collection_file_state(),
				get_view_state(),
				this)),
	d_layering_dialog_ptr(NULL),
	d_choose_canvas_tool(
			new GPlatesGui::ChooseCanvasTool(*this)),
	d_digitise_geometry_builder(
			new GPlatesViewOperations::GeometryBuilder()),
	d_focused_feature_geometry_builder(
			new GPlatesViewOperations::GeometryBuilder()),
	d_geometry_operation_target(
			new GPlatesViewOperations::GeometryOperationTarget(
				*d_digitise_geometry_builder,
				*d_focused_feature_geometry_builder,
				get_view_state().get_feature_focus(),
				*d_choose_canvas_tool)),
	d_clone_operation_ptr(
			new GPlatesViewOperations::CloneOperation(
				d_choose_canvas_tool.get(),
				d_digitise_geometry_builder.get(),
				d_focused_feature_geometry_builder.get(),
				get_view_state())),
	d_delete_feature_operation_ptr(
			new GPlatesViewOperations::DeleteFeatureOperation(
				get_view_state().get_feature_focus(),
				get_application_state())),
	d_active_geometry_operation(
			new GPlatesViewOperations::ActiveGeometryOperation()),
	d_focused_feature_geom_manipulator(
			new GPlatesViewOperations::FocusedFeatureGeometryManipulator(
				*d_focused_feature_geometry_builder,
				get_view_state())),
	d_enable_canvas_tool(
			new GPlatesGui::EnableCanvasTool(
				*this,
				get_view_state().get_feature_focus(),
				*d_geometry_operation_target,
				*d_choose_canvas_tool)),
	d_measure_distance_state_ptr(
			new GPlatesCanvasTools::MeasureDistanceState(
				get_view_state().get_rendered_geometry_collection(),
				*d_geometry_operation_target)),
	d_topology_sections_container_ptr(
			new GPlatesGui::TopologySectionsContainer()),
	d_feature_table_model_ptr(
			new GPlatesGui::FeatureTableModel(
				get_view_state())),
	d_task_panel_ptr(NULL),
	d_canvas_tool_last_chosen_by_user(
			GPlatesCanvasTools::CanvasToolType::DRAG_GLOBE),
	d_action_show_bottom_panel(NULL)
{
	setupUi(this);

	// FIXME: remove this when all non Qt widget state has been moved into ViewState.
	// This is a temporary solution to avoiding passing ViewportWindow references around
	// when only non Qt widget related view state is needed - currently ViewportWindow contains
	// some of this state so just get the ViewportWindow reference from ViewState -
	// the method will eventually get removed when the state has been moved over.
	get_view_state().set_other_view_state(*this);

	// Initialise the Shapefile property mapper before we start reading.
	// FIXME: Not sure where this should go since it involves qt widgets (logical place is
	// in FeatureCollectionFileIO but that is application state and shouldn't know about
	// qt widgets).
	boost::shared_ptr<GPlatesQtWidgets::ShapefilePropertyMapper> shapefile_property_mapper(
			new GPlatesQtWidgets::ShapefilePropertyMapper(this));
	GPlatesFileIO::ShapefileReader::set_property_mapper(shapefile_property_mapper);

	std::auto_ptr<TaskPanel> task_panel_auto_ptr(new TaskPanel(
			get_view_state(),
			*d_digitise_geometry_builder,
			*d_geometry_operation_target,
			*d_active_geometry_operation,
			*d_measure_distance_state_ptr,
			*this,
			*d_choose_canvas_tool,
			this));
	d_task_panel_ptr = task_panel_auto_ptr.get();

	// Connect all the Signal/Slot relationships of ViewportWindow's
	// toolbar buttons and menu items.
	connect_menu_actions();

	if(enable_data_mining)//Data-mining temporary code
	{
		install_data_mining_menu();
	}
	
	// Duplicate the menu structure for the full-screen-mode GMenu.
	populate_gmenu_from_menubar();
	// Initialise various elements for full-screen-mode that must wait until after setupUi().
	d_full_screen_mode.init();
	// Initialise the Recent Session menu (that must wait until after setupUi()).
	d_session_menu_ptr->init(*menu_Open_Recent_Session);
	
	// Set up an emergency context menu to control QDockWidgets even if
	// they're no longer behaving properly.
	set_up_dock_context_menus();
	
	// FIXME: Set up the Task Panel in a more detailed fashion here.
#if 1
	d_reconstruction_view_widget.insert_task_panel(task_panel_auto_ptr);
#else
	// Stretchable Task Panel hack for testing: Make the Task Panel
	// into a QDockWidget, undocked by default.
	QDockWidget *task_panel_dock = new QDockWidget(tr("Task Panel"), this);
	task_panel_dock->setObjectName("TaskPanelDock");
	task_panel_dock->setWidget(task_panel_auto_ptr.release());
	task_panel_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, task_panel_dock);
	task_panel_dock->setFloating(true);
#endif
	set_up_task_panel_actions();

	// Enable/disable canvas tools according to the current state.
	d_enable_canvas_tool->initialise();
	// Disable the feature-specific Actions as there is no currently focused feature to act on.
	enable_or_disable_feature_actions(get_view_state().get_feature_focus());
	QObject::connect(&get_view_state().get_feature_focus(), SIGNAL(focus_changed(
					GPlatesGui::FeatureFocus &)),
			this, SLOT(enable_or_disable_feature_actions(GPlatesGui::FeatureFocus &)));

	// Set up the Specify Anchored Plate ID dialog.
	// Perform a reconstruction when the anchor plate id changes.
	QObject::connect(
			d_specify_anchored_plate_id_dialog_ptr.get(),
			SIGNAL(value_changed(GPlatesModel::integer_plate_id_type)),
			&get_application_state(),
			SLOT(set_anchored_plate_id(GPlatesModel::integer_plate_id_type)));

	// Set up the Reconstruction View widget.
	setCentralWidget(&d_reconstruction_view_widget);

	connect_feature_collection_file_io_signals();

	// Set up the Clicked table (which is now actually a tree in disguise).
	// FIXME: All the Clicked Table stuff (the below, the d_feature_table_model_ptr,
	//        FeatureTableModel::handle_selection_change,
	//        ViewportWindow::highlight_first_clicked_feature_table_row() ) should
	//        be put in their own class.
	tree_view_clicked_geometries->setRootIsDecorated(false);
	tree_view_clicked_geometries->setModel(d_feature_table_model_ptr.get());
	GPlatesGui::FeatureTableModel::set_default_resize_modes(*tree_view_clicked_geometries->header());
	tree_view_clicked_geometries->header()->setMinimumSectionSize(60);
	tree_view_clicked_geometries->header()->setMovable(true);
	// When the user selects a row of the table, we should focus that feature.
	// RJW: This is what triggers the highlighting of the geometry on the canvas.
	QObject::connect(tree_view_clicked_geometries->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			d_feature_table_model_ptr.get(),
			SLOT(handle_selection_change(const QItemSelection &, const QItemSelection &)));
	// And if the focus is changed programatically, from e.g. Clone Feature, ensure the Clicked
	// Table still displays it
	QObject::connect(&get_view_state().get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(highlight_focused_feature_in_table()));

	// Set up the Topology Sections Table, now that the table widget has been created.
	d_topology_sections_table_ptr.reset(
			new GPlatesGui::TopologySectionsTable(
					*table_widget_topology_sections,
					*d_topology_sections_container_ptr,
					get_view_state().get_feature_focus()));

	// If the focused feature is modified, we may need to update the ShapefileAttributeViewerDialog.
	QObject::connect(&get_view_state().get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			d_shapefile_attribute_viewer_dialog_ptr.get(), SLOT(update()));

	// Create canvas tools.
	GPlatesGui::Globe &globe = globe_canvas().globe();
	MapCanvas &map_canvas = map_view().map_canvas();
	GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection =
		get_view_state().get_rendered_geometry_collection();
	GPlatesGui::FeatureFocus &feature_focus = get_view_state().get_feature_focus();
	GlobeAndMapWidget &globe_and_map_widget = d_reconstruction_view_widget.globe_and_map_widget();
	GPlatesGui::MapTransform &map_transform = get_view_state().get_map_transform();
	GPlatesCanvasTools::CanvasTool::status_bar_callback_type status_bar_callback =
		boost::bind(
				&::canvas_tool_status_message,
				boost::ref(*this),
				_1);

	GPlatesCanvasTools::ClickGeometry::non_null_ptr_type click_geometry_tool =
		GPlatesCanvasTools::ClickGeometry::create(
				status_bar_callback,
				rendered_geom_collection,
				*this,
				*d_feature_table_model_ptr,
				*d_feature_properties_dialog_ptr,
				feature_focus,
				get_application_state());
	GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type digitise_polyline_tool =
		GPlatesCanvasTools::DigitiseGeometry::create(
				status_bar_callback,
				GPlatesViewOperations::GeometryType::POLYLINE,
				*d_geometry_operation_target,
				*d_active_geometry_operation,
				rendered_geom_collection,
				*d_choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYLINE,
				globe_and_map_widget);
	GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type digitise_multipoint_tool =
		GPlatesCanvasTools::DigitiseGeometry::create(
				status_bar_callback,
				GPlatesViewOperations::GeometryType::MULTIPOINT,
				*d_geometry_operation_target,
				*d_active_geometry_operation,
				rendered_geom_collection,
				*d_choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_MULTIPOINT,
				globe_and_map_widget);
	GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type digitise_polygon_tool =
		GPlatesCanvasTools::DigitiseGeometry::create(
				status_bar_callback,
				GPlatesViewOperations::GeometryType::POLYGON,
				*d_geometry_operation_target,
				*d_active_geometry_operation,
				rendered_geom_collection,
				*d_choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYGON,
				globe_and_map_widget);
	GPlatesCanvasTools::MoveVertex::non_null_ptr_type move_vertex_tool =
		GPlatesCanvasTools::MoveVertex::create(
				status_bar_callback,
				*d_geometry_operation_target,
				*d_active_geometry_operation,
				rendered_geom_collection,
				*d_choose_canvas_tool,
				globe_and_map_widget,
				this);
	GPlatesCanvasTools::DeleteVertex::non_null_ptr_type delete_vertex_tool =
		GPlatesCanvasTools::DeleteVertex::create(
				status_bar_callback,
				*d_geometry_operation_target,
				*d_active_geometry_operation,
				rendered_geom_collection,
				*d_choose_canvas_tool,
				globe_and_map_widget);
	GPlatesCanvasTools::InsertVertex::non_null_ptr_type insert_vertex_tool =
		GPlatesCanvasTools::InsertVertex::create(
				status_bar_callback,
				*d_geometry_operation_target,
				*d_active_geometry_operation,
				rendered_geom_collection,
				*d_choose_canvas_tool,
				globe_and_map_widget);
	GPlatesCanvasTools::SplitFeature::non_null_ptr_type split_feature_tool =
		GPlatesCanvasTools::SplitFeature::create(
				status_bar_callback,
				feature_focus,
				get_view_state(),
				*d_geometry_operation_target,
				*d_active_geometry_operation,
				rendered_geom_collection,
				*d_choose_canvas_tool,
				globe_and_map_widget);
	GPlatesCanvasTools::ManipulatePole::non_null_ptr_type manipulate_pole_tool =
		GPlatesCanvasTools::ManipulatePole::create(
				status_bar_callback,
				rendered_geom_collection,
				d_task_panel_ptr->modify_reconstruction_pole_widget());
	GPlatesCanvasTools::BuildTopology::non_null_ptr_type build_topology_tool =
		GPlatesCanvasTools::BuildTopology::create(
				status_bar_callback,
				get_view_state(),
				*this,
				*d_feature_table_model_ptr,
				*d_topology_sections_container_ptr,
				d_task_panel_ptr->topology_tools_widget(),
				get_application_state());
	GPlatesCanvasTools::EditTopology::non_null_ptr_type edit_topology_tool =
		GPlatesCanvasTools::EditTopology::create(
				status_bar_callback,
				get_view_state(),
				*this,
				*d_feature_table_model_ptr,
				*d_topology_sections_container_ptr,
				d_task_panel_ptr->topology_tools_widget(),
				get_application_state());
	GPlatesCanvasTools::MeasureDistance::non_null_ptr_type measure_distance_tool =
		GPlatesCanvasTools::MeasureDistance::create(
				status_bar_callback,
				rendered_geom_collection,
				*d_measure_distance_state_ptr);

	// Set up the Map and Globe Canvas Tools Choices.
	d_globe_canvas_tool_choice_ptr.reset(
			new GPlatesGui::GlobeCanvasToolChoice(
				globe,
				globe_canvas(),
				*this,
				get_view_state(),
				click_geometry_tool,
				digitise_polyline_tool,
				digitise_multipoint_tool,
				digitise_polygon_tool,
				move_vertex_tool,
				delete_vertex_tool,
				insert_vertex_tool,
				split_feature_tool,
				manipulate_pole_tool,
				build_topology_tool,
				edit_topology_tool,
				measure_distance_tool));
	d_map_canvas_tool_choice_ptr.reset(
			new GPlatesGui::MapCanvasToolChoice(
				map_canvas,
				map_view(),
				*this,
				map_transform,
				get_view_state().get_viewport_zoom(),
				click_geometry_tool,
				digitise_polyline_tool,
				digitise_multipoint_tool,
				digitise_polygon_tool,
				move_vertex_tool,
				delete_vertex_tool,
				insert_vertex_tool,
				split_feature_tool,
				manipulate_pole_tool,
				build_topology_tool,
				edit_topology_tool,
				measure_distance_tool));

	// Set up the Map and Globe Canvas Tool Adapters for handling globe click and drag events.
	// FIXME:  This is, of course, very exception-unsafe.  This whole class needs to be nuked.
	d_globe_canvas_tool_adapter_ptr.reset(new GPlatesGui::GlobeCanvasToolAdapter(
			*d_globe_canvas_tool_choice_ptr));
	d_map_canvas_tool_adapter_ptr.reset(new GPlatesGui::MapCanvasToolAdapter(
			*d_map_canvas_tool_choice_ptr));

	connect_canvas_tools();


	// If the user creates a new feature with the DigitisationWidget, we need to reconstruct to
	// make sure everything is displayed properly.
	QObject::connect(&(d_task_panel_ptr->digitisation_widget().get_create_feature_dialog()),
			SIGNAL(feature_created(GPlatesModel::FeatureHandle::weak_ref)),
			&get_application_state(),
			SLOT(reconstruct()));


	// After modifying the move-nearby-vertices widget, this will reset the focus to the globe/map. 
	QObject::connect(d_task_panel_ptr,
					SIGNAL(vertex_data_changed(bool,double,bool,GPlatesModel::integer_plate_id_type)),
					&d_reconstruction_view_widget,SLOT(setFocus()));
	// Initialise the "Trinket Area", a class which manages the various icons present in the
	// status bar. This must occur after ViewportWindow::setupUi().
	d_trinket_area_ptr->init();

	// Initialise the "Unsaved Changes" tracking aspect of the GUI, now that setupUi() has
	// been called and all the widgets that are used to notify the user are in place.
	d_unsaved_changes_tracker_ptr->init();

	// Registered a slot to be called when a new reconstruction is generated.
	QObject::connect(
			&get_application_state(),
			SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(handle_reconstruction()));

	// Render everything on the screen in present-day positions.
	get_application_state().reconstruct();

	// Initialise the default range of the animation slider based on UserPreferences.
	d_animation_controller.init_default_time_range();

	// Synchronise the "Show X Features" menu items with RenderSettings.
	GPlatesGui::RenderSettings &render_settings = get_view_state().get_render_settings();
	action_Show_Point_Features->setChecked(render_settings.show_points());
	action_Show_Line_Features->setChecked(render_settings.show_lines());
	action_Show_Polygon_Features->setChecked(render_settings.show_polygons());
	action_Show_Multipoint_Features->setChecked(render_settings.show_multipoints());
	action_Show_Arrow_Decorations->setChecked(render_settings.show_arrows());
	action_Show_Strings->setChecked(render_settings.show_strings());

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

	set_internal_release_window_title();
}


GPlatesQtWidgets::ViewportWindow::~ViewportWindow()
{
	// boost::scoped_ptr destructors need complete type.
}


void	
GPlatesQtWidgets::ViewportWindow::connect_menu_actions()
{
	// If you want to add a new menu action, the steps are:
	// 0. Open ViewportWindowUi.ui in the Designer.
	// 1. Create a QAction in the Designer's Action Editor, called action_Something.
	// 2. Assign icons, tooltips, and shortcuts as necessary.
	//    2a. Canvas Tools must have the 'checkable' property set.
	// 3. Drag this action to a menu.
	//    3a. If it's a canvas tool, drag it to the toolbar instead. It will appear on the
	//        Tools menu automatically.
	// 4. Add code for the triggered() signal your action generates here.
	//    Please keep this function sorted in the same order as menu items appear.

	// Canvas Tools:
	// The canvas tools are special; we only want one of them to be checked at any time.
	// We can do this quite easily by adding all the actions on the toolbar to a QActionGroup.
	QList<QAction *> canvas_tools_actions_list = toolbar_canvas_tools->actions();
	QActionGroup* canvas_tools_group = new QActionGroup(this);	// parented to ViewportWindow.
	Q_FOREACH(QAction *tool_action, canvas_tools_actions_list) {
		canvas_tools_group->addAction(tool_action);
	}
	// We also want to populate the Tools menu based on the actions added to the toolbar.
	Q_FOREACH(QAction *tool_action, canvas_tools_actions_list) {
		menu_Tools->addAction(tool_action);
	}

	// Canvas Tool connections:
	QObject::connect(action_Drag_Globe, SIGNAL(triggered()),
			this, SLOT(handle_drag_globe_triggered()));
	QObject::connect(action_Zoom_Globe, SIGNAL(triggered()),
			this, SLOT(handle_zoom_globe_triggered()));
	QObject::connect(action_Click_Geometry, SIGNAL(triggered()),
			this, SLOT(handle_click_geometry_triggered()));
	QObject::connect(action_Digitise_New_Polyline, SIGNAL(triggered()),
			this, SLOT(handle_digitise_new_polyline_triggered()));
	QObject::connect(action_Digitise_New_MultiPoint, SIGNAL(triggered()),
			this, SLOT(handle_digitise_new_multipoint_triggered()));
	QObject::connect(action_Digitise_New_Polygon, SIGNAL(triggered()),
			this, SLOT(handle_digitise_new_polygon_triggered()));
#if 0
	QObject::connect(action_Move_Geometry, SIGNAL(triggered()),
			this, SLOT(choose_move_geometry_tool()));
#else
	// FIXME: The Move Geometry tool, although it has an awesome icon,
	// is to be disabled until it can be implemented.
	action_Move_Geometry->setVisible(false);
#endif
	QObject::connect(action_Move_Vertex, SIGNAL(triggered()),
			this, SLOT(handle_move_vertex_triggered()));
	QObject::connect(action_Delete_Vertex, SIGNAL(triggered()),
			this, SLOT(handle_delete_vertex_triggered()));
	QObject::connect(action_Insert_Vertex, SIGNAL(triggered()),
			this, SLOT(handle_insert_vertex_triggered()));
	QObject::connect(action_Split_Feature, SIGNAL(triggered()),
			this, SLOT(handle_split_feature_triggered()));
	QObject::connect(action_Manipulate_Pole, SIGNAL(triggered()),
			this, SLOT(handle_manipulate_pole_triggered()));
	QObject::connect(action_Build_Topology, SIGNAL(triggered()),
			this, SLOT(handle_build_topology_triggered()));
	QObject::connect(action_Edit_Topology, SIGNAL(triggered()),
			this, SLOT(handle_edit_topology_triggered()));
	QObject::connect(action_Measure_Distance, SIGNAL(triggered()),
			this, SLOT(handle_measure_distance_triggered()));

	// File Menu:
	QObject::connect(action_New_Window, SIGNAL(triggered()),
			this, SLOT(open_new_window()));
	QObject::connect(action_Open_Feature_Collection, SIGNAL(triggered()),
			d_file_io_feedback_ptr, SLOT(open_files()));
	QObject::connect(action_Import_Raster, SIGNAL(triggered()),
			this, SLOT(pop_up_import_raster_dialog()));
	QObject::connect(action_Import_Time_Dependent_Raster, SIGNAL(triggered()),
			this, SLOT(pop_up_import_time_dependent_raster_dialog()));
	QObject::connect(action_File_Errors, SIGNAL(triggered()),
			this, SLOT(pop_up_read_errors_dialog()));
	// ---
	QObject::connect(action_Manage_Feature_Collections, SIGNAL(triggered()),
			this, SLOT(pop_up_manage_feature_collections_dialog()));
	QObject::connect(action_View_Total_Reconstruction_Sequences, SIGNAL(triggered()),
			this, SLOT(pop_up_total_reconstruction_sequences_dialog()));
	QObject::connect(action_View_Shapefile_Attributes, SIGNAL(triggered()),
			this, SLOT(pop_up_shapefile_attribute_viewer_dialog()));
	// ----
	QObject::connect(action_Quit, SIGNAL(triggered()),
			this, SLOT(close()));
	
	// Edit Menu:
	QObject::connect(action_Query_Feature, SIGNAL(triggered()),
			d_feature_properties_dialog_ptr.get(), SLOT(choose_query_widget_and_open()));
	QObject::connect(action_Edit_Feature, SIGNAL(triggered()),
			d_feature_properties_dialog_ptr.get(), SLOT(choose_edit_widget_and_open()));
	// ----
	// Unfortunately, the Undo and Redo actions cannot be added in the Designer,
	// or at least, not nicely. We need to ask the QUndoGroup to create some
	// QActions for us, and add them programmatically. To follow the principle
	// of least surprise, placeholder actions are set up in the designer, which
	// this code can use to insert the actions in the correct place with the
	// correct shortcut.
	// The new actions will be linked to the QUndoGroup appropriately.
	QAction *undo_action_ptr =
		GPlatesViewOperations::UndoRedo::instance().get_undo_group().createUndoAction(this, tr("&Undo"));
	QAction *redo_action_ptr =
		GPlatesViewOperations::UndoRedo::instance().get_undo_group().createRedoAction(this, tr("Re&do"));
	undo_action_ptr->setShortcut(action_Undo_Placeholder->shortcut());
	redo_action_ptr->setShortcut(action_Redo_Placeholder->shortcut());
	menu_Edit->insertAction(action_Undo_Placeholder, undo_action_ptr);
	menu_Edit->insertAction(action_Redo_Placeholder, redo_action_ptr);
	menu_Edit->removeAction(action_Undo_Placeholder);
	menu_Edit->removeAction(action_Redo_Placeholder);
	// ----
	QObject::connect(action_Delete_Feature, SIGNAL(triggered()),
			d_delete_feature_operation_ptr.get(), SLOT(delete_focused_feature()));
	// ----
	QObject::connect(action_Clear_Selection, SIGNAL(triggered()),
			&get_view_state().get_feature_focus(), SLOT(unset_focus()));

	QObject::connect(action_Clone_Geometry, SIGNAL(triggered()),
			d_clone_operation_ptr.get(), SLOT(clone_focused_geometry()));
	QObject::connect(action_Clone_Feature, SIGNAL(triggered()),
			d_clone_operation_ptr.get(), SLOT(clone_focused_feature()));

	// Preferences is beta functionality and not on a menu anywhere yet.
	QObject::connect(action_Preferences, SIGNAL(triggered()),
			this, SLOT(pop_up_preferences_dialog()));

	// Reconstruction Menu:
	QObject::connect(action_Reconstruct_to_Time, SIGNAL(triggered()),
			&d_reconstruction_view_widget, SLOT(activate_time_spinbox()));
	QObject::connect(action_Increment_Animation_Time_Forwards, SIGNAL(triggered()),
			&d_animation_controller, SLOT(step_forward()));
	QObject::connect(action_Increment_Animation_Time_Backwards, SIGNAL(triggered()),
			&d_animation_controller, SLOT(step_back()));
	QObject::connect(action_Animate, SIGNAL(triggered()),
			this, SLOT(pop_up_animate_dialog()));
	QObject::connect(action_Reset_Animation, SIGNAL(triggered()),
			&d_animation_controller, SLOT(seek_beginning()));
	QObject::connect(action_Play, SIGNAL(triggered(bool)),
			&d_animation_controller, SLOT(set_play_or_pause(bool)));
	QObject::connect(&d_animation_controller, SIGNAL(animation_state_changed(bool)),
			action_Play, SLOT(setChecked(bool)));
	// ----
	QObject::connect(action_Export, SIGNAL(triggered()),
			this, SLOT(pop_up_export_animation_dialog()));
	// ----
	QObject::connect(action_Specify_Anchored_Plate_ID, SIGNAL(triggered()),
			this, SLOT(pop_up_specify_anchored_plate_id_dialog()));
	QObject::connect(action_View_Reconstruction_Poles, SIGNAL(triggered()),
			this, SLOT(pop_up_total_reconstruction_poles_dialog()));
	// ----
	QObject::connect(action_Assign_Plate_IDs, SIGNAL(triggered()),
			this, SLOT(pop_up_assign_reconstruction_plate_ids_dialog()));
	QObject::connect(action_Generate_Mesh_Cap, SIGNAL(triggered()),
			this, SLOT(generate_mesh_cap()));
	
	// Layers Menu:
	QObject::connect(menu_Layers, SIGNAL(aboutToShow()),
			this, SLOT(handle_layers_menu_about_to_show()));
	QObject::connect(action_Show_Layers, SIGNAL(triggered(bool)),
			this, SLOT(set_layering_dialog_visibility(bool)));
	QObject::connect(action_Manage_Colouring, SIGNAL(triggered()),
			this, SLOT(pop_up_colouring_dialog()));
	QObject::connect(action_Manage_Small_Circles, SIGNAL(triggered()),
			this, SLOT(pop_up_small_circle_manager()));
	QObject::connect(action_Generate_Mesh_Cap, SIGNAL(triggered()),
		this, SLOT(generate_mesh_cap()));
	
	// View Menu:
	QObject::connect(action_Show_Point_Features, SIGNAL(triggered()),
			this, SLOT(enable_point_display()));
	QObject::connect(action_Show_Line_Features, SIGNAL(triggered()),
			this, SLOT(enable_line_display()));
	QObject::connect(action_Show_Polygon_Features, SIGNAL(triggered()),
			this, SLOT(enable_polygon_display()));
	QObject::connect(action_Show_Multipoint_Features, SIGNAL(triggered()),
			this, SLOT(enable_multipoint_display()));
	QObject::connect(action_Show_Arrow_Decorations, SIGNAL(triggered()),
			this, SLOT(enable_arrows_display()));
	QObject::connect(action_Show_Strings, SIGNAL(triggered()),
			this, SLOT(enable_strings_display()));
	QObject::connect(action_Show_Stars, SIGNAL(triggered()),
			this, SLOT(enable_stars_display()));
	
	// View Menu:
	QObject::connect(action_Full_Screen, SIGNAL(triggered(bool)),
			&d_full_screen_mode, SLOT(toggle_full_screen(bool)));
	d_action_show_bottom_panel = dock_search_results->toggleViewAction();
	d_action_show_bottom_panel->setText(tr("Show &Bottom Panel"));
	d_action_show_bottom_panel->setObjectName("action_Show_Bottom_Panel");
	menu_View->insertAction(action_Show_Bottom_Panel_Placeholder, d_action_show_bottom_panel);
	menu_View->removeAction(action_Show_Bottom_Panel_Placeholder);
	// ----
	QObject::connect(action_Set_Projection, SIGNAL(triggered()),
			this, SLOT(pop_up_set_projection_dialog()));
	QObject::connect(action_Set_Camera_Viewpoint, SIGNAL(triggered()),
			this, SLOT(pop_up_set_camera_viewpoint_dialog()));
	// ----
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
			&d_reconstruction_view_widget, SLOT(activate_zoom_spinbox()));
	QObject::connect(action_Zoom_In, SIGNAL(triggered()),
			&get_view_state().get_viewport_zoom(), SLOT(zoom_in()));
	QObject::connect(action_Zoom_Out, SIGNAL(triggered()),
			&get_view_state().get_viewport_zoom(), SLOT(zoom_out()));
	QObject::connect(action_Reset_Zoom_Level, SIGNAL(triggered()),
			&get_view_state().get_viewport_zoom(), SLOT(reset_zoom()));

	// Tools menu. This is mostly auto-populated with Canvas Tool actions,
	// which don't need to be hooked up here, however there are a few little
	// extras which aren't regular canvas tools and should be connected here:-
	QObject::connect(action_Use_Small_Icons, SIGNAL(toggled(bool)),
		this, SLOT(use_small_canvas_tool_icons(bool)));
	
	// Paleomagnetism menu	
	QObject::connect(action_Create_VGP, SIGNAL(triggered()),
		this, SLOT(pop_up_create_vgp_dialog()));
	QObject::connect(action_Calculate_Reconstruction_Pole, SIGNAL(triggered()),
		this, SLOT(pop_up_calculate_reconstruction_pole_dialog()));
	QObject::connect(action_Set_VGP_Visibility, SIGNAL(triggered()),
		this, SLOT(pop_up_set_vgp_visibility_dialog()));	
	
	// Help Menu:
	QObject::connect(action_About, SIGNAL(triggered()),
			this, SLOT(pop_up_about_dialog()));
}


void	
GPlatesQtWidgets::ViewportWindow::connect_feature_collection_file_io_signals()
{
	QObject::connect(
			&get_application_state().get_feature_collection_file_io(),
			SIGNAL(handle_read_errors(
					GPlatesAppLogic::FeatureCollectionFileIO &,
					const GPlatesFileIO::ReadErrorAccumulation &)),
			this,
			SLOT(handle_read_errors(
					GPlatesAppLogic::FeatureCollectionFileIO &,
					const GPlatesFileIO::ReadErrorAccumulation &)));
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
	return d_application.get_application_state();
}


GPlatesPresentation::ViewState &
GPlatesQtWidgets::ViewportWindow::get_view_state()
{
	return d_application.get_view_state();
}

void
GPlatesQtWidgets::ViewportWindow::connect_canvas_tools()
{
	GlobeCanvas *globe_canvas_ptr = &globe_canvas();

	QObject::connect(globe_canvas_ptr, SIGNAL(mouse_pressed(const GPlatesMaths::PointOnSphere &,
		const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
		Qt::KeyboardModifiers)),
		d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_press(const GPlatesMaths::PointOnSphere &,
		const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
		Qt::KeyboardModifiers)));


	QObject::connect(globe_canvas_ptr, SIGNAL(mouse_clicked(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)),
			d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_click(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool, Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(globe_canvas_ptr, SIGNAL(mouse_dragged(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)));

	QObject::connect(globe_canvas_ptr, SIGNAL(mouse_released_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_release_after_drag(const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &,
					Qt::MouseButton, Qt::KeyboardModifiers)));

	QObject::connect(globe_canvas_ptr, SIGNAL(mouse_moved_without_drag(
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &)),
					d_globe_canvas_tool_adapter_ptr.get(), SLOT(handle_move_without_drag(
					const GPlatesMaths::PointOnSphere &,
					const GPlatesMaths::PointOnSphere &, bool,
					const GPlatesMaths::PointOnSphere &)));

	QObject::connect(&(d_reconstruction_view_widget.map_view()), SIGNAL(mouse_pressed(const QPointF &,
		bool, Qt::MouseButton,
		Qt::KeyboardModifiers)),
		d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_press(const QPointF &,
		bool, Qt::MouseButton,
		Qt::KeyboardModifiers)));

	QObject::connect(&map_view(), SIGNAL(mouse_clicked(const QPointF &,
					 bool, Qt::MouseButton,
					Qt::KeyboardModifiers)),
			d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_click(const QPointF &,
				    bool, Qt::MouseButton,
					Qt::KeyboardModifiers)));


	QObject::connect(&map_view(), SIGNAL(mouse_dragged(const QPointF &,
					 bool, const QPointF &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers, const QPointF &)),
			d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_drag(const QPointF &,
					 bool, const QPointF &, bool,
					Qt::MouseButton, Qt::KeyboardModifiers, const QPointF &)));

	QObject::connect(&map_view(), SIGNAL(mouse_released_after_drag(const QPointF &,
					 bool, const QPointF &, bool, const QPointF &,
					Qt::MouseButton, Qt::KeyboardModifiers)),
			d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_release_after_drag(const QPointF &,
					 bool, const QPointF &, bool, const QPointF &,
					Qt::MouseButton, Qt::KeyboardModifiers)));


	QObject::connect(&map_view(), SIGNAL(mouse_moved_without_drag(
					const QPointF &,
					bool, const QPointF &)),
					d_map_canvas_tool_adapter_ptr.get(), SLOT(handle_move_without_drag(
					const QPointF &,
					bool, const QPointF &)));

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
#if 0
	// Commenting this out for the time being because we can only fit 5 buttons on one row.
	feature_actions.add_action(action_Clear_Selection);
#endif
	feature_actions.add_action(action_Clone_Geometry);
	feature_actions.add_action(action_Clone_Feature);
	feature_actions.add_action(action_Delete_Feature);
}


void
GPlatesQtWidgets::ViewportWindow::set_up_dock_context_menus()
{
	// Search Results Dock:
	dock_search_results->addAction(action_Information_Dock_At_Top);
	dock_search_results->addAction(action_Information_Dock_At_Bottom);
	QObject::connect(action_Information_Dock_At_Top, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_top()));
	QObject::connect(action_Information_Dock_At_Bottom, SIGNAL(triggered()),
			this, SLOT(dock_search_results_at_bottom()));
}


void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_top()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::TopDockWidgetArea, dock_search_results);
}

void
GPlatesQtWidgets::ViewportWindow::dock_search_results_at_bottom()
{
	dock_search_results->setFloating(false);
	addDockWidget(Qt::BottomDockWidgetArea, dock_search_results);
}


void
GPlatesQtWidgets::ViewportWindow::highlight_first_clicked_feature_table_row() const
{
	QModelIndex idx = d_feature_table_model_ptr->index(0, 0);

	if (idx.isValid()) {
		tree_view_clicked_geometries->selectionModel()->clear();
		tree_view_clicked_geometries->selectionModel()->select(idx,
				QItemSelectionModel::Select |
				QItemSelectionModel::Current |
				QItemSelectionModel::Rows);
	}
	tree_view_clicked_geometries->scrollToTop();
	// The columns of the table (especially the last column) tend not to adjust
	// properly for some reason, unless we force them to:-
	for (int i=0; i<d_feature_table_model_ptr->columnCount(); ++i) {
		tree_view_clicked_geometries->resizeColumnToContents(i);
	}
}


void
GPlatesQtWidgets::ViewportWindow::highlight_focused_feature_in_table()
{
	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type rg_maybe_ptr = 
			get_view_state().get_feature_focus().associated_reconstruction_geometry();
	if ( ! rg_maybe_ptr) {
		return;
	}
	GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type rg_ptr(rg_maybe_ptr.get());
	
	// Check to see if this newly focused feature is in the Clicked table already.
	QModelIndex idx = d_feature_table_model_ptr->get_index_for_geometry(rg_ptr);
	if (idx.isValid()) {
		// It is. Move the highlight to that line (if we've been good, this won't cause
		// an infinite loop of 'change' signals because FeatureFocus won't emit anything
		// if we tell it to focus something that's already focused)
		tree_view_clicked_geometries->selectionModel()->clear();
		tree_view_clicked_geometries->selectionModel()->select(idx,
				QItemSelectionModel::Select |
				QItemSelectionModel::Current |
				QItemSelectionModel::Rows);
	} else {
		// It is not in there. Most likely this is from the Clone Feature action setting
		// the focus directly. 'Unshift' it onto the start of the Clicked list.
		GPlatesGui::add_geometry_to_top_of_feature_table(rg_ptr, *d_feature_table_model_ptr,
				get_application_state().get_reconstruct_graph());
		highlight_first_clicked_feature_table_row();
	}
}

void
GPlatesQtWidgets::ViewportWindow::handle_reconstruction()
{
	// A new reconstruction has just happened so do some updating.
	if (d_total_reconstruction_poles_dialog_ptr->isVisible())
	{
		d_total_reconstruction_poles_dialog_ptr->update();
	}
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_specify_anchored_plate_id_dialog()
{
	// note: this dialog is created in the constructor

	d_specify_anchored_plate_id_dialog_ptr->populate(
			get_application_state().get_current_anchored_plate_id(),
			get_view_state().get_feature_focus().focused_feature());
	d_specify_anchored_plate_id_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_specify_anchored_plate_id_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_specify_anchored_plate_id_dialog_ptr->raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_set_camera_viewpoint_dialog()
{
	if (!d_set_camera_viewpoint_dialog_ptr)
	{
		d_set_camera_viewpoint_dialog_ptr.reset(new SetCameraViewpointDialog(*this, this));
	}

	boost::optional<GPlatesMaths::LatLonPoint> cur_llp_opt = d_reconstruction_view_widget.camera_llp();
	GPlatesMaths::LatLonPoint cur_llp = cur_llp_opt ? *cur_llp_opt : GPlatesMaths::LatLonPoint(0.0, 0.0);

	d_set_camera_viewpoint_dialog_ptr->set_lat_lon(cur_llp.latitude(), cur_llp.longitude());
	if (d_set_camera_viewpoint_dialog_ptr->exec())
	{
		// Note: d_set_camera_viewpoint_dialog_ptr is modal and should not need the 'raise' hacks
		// that the other dialogs use.
		try {
			GPlatesMaths::LatLonPoint desired_centre(
					d_set_camera_viewpoint_dialog_ptr->latitude(),
					d_set_camera_viewpoint_dialog_ptr->longitude());
			
			d_reconstruction_view_widget.active_view().set_camera_viewpoint(desired_centre);

		} catch (GPlatesMaths::InvalidLatLonException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.

			// User somehow managed to specify an invalid lat,lon. Pretend it didn't happen.
		}
	}
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_total_reconstruction_poles_dialog()
{
	// note: this dialog is created in the constructor
	
	d_total_reconstruction_poles_dialog_ptr->update();

	d_total_reconstruction_poles_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_total_reconstruction_poles_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_total_reconstruction_poles_dialog_ptr->raise();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_total_reconstruction_poles_dialog(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	// note: this dialog is created in the constructor
	
	d_total_reconstruction_poles_dialog_ptr->update(visual_layer);

	d_total_reconstruction_poles_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_total_reconstruction_poles_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_total_reconstruction_poles_dialog_ptr->raise();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_animate_dialog()
{
	if (!d_animate_dialog_ptr)
	{
		d_animate_dialog_ptr.reset(new AnimateDialog(d_animation_controller, this));
	}

	d_animate_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_animate_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_animate_dialog_ptr->raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_preferences_dialog()
{
	if (!d_preferences_dialog_ptr)
	{
		d_preferences_dialog_ptr.reset(new PreferencesDialog(this));
	}

	// PreferencesDialog is modal and should not need the 'raise' hack other dialogs use.
	d_preferences_dialog_ptr->exec();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_about_dialog()
{
	if (!d_about_dialog_ptr)
	{
		d_about_dialog_ptr.reset(new AboutDialog(this));
	}

	d_about_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_about_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_about_dialog_ptr->raise();
}


void
GPlatesQtWidgets::ViewportWindow::set_layering_dialog_visibility(
		bool visible)
{
	if (!d_layering_dialog_ptr)
	{
		// Create a new dialog and fill it with a VisualLayersWidget.
		// This doesn't have to be anything fancy, because this widget is only
		// temporarily living in a dialog (it will go into a dock later).
		QDialog *dialog = new QDialog(this, Qt::Window);
		QHBoxLayout *dialog_layout = new QHBoxLayout(dialog);
		dialog_layout->addWidget(
				new VisualLayersWidget(
					get_view_state().get_visual_layers(),
					get_application_state(),
					get_view_state(),
					this,
					dialog));
		dialog->setLayout(dialog_layout);
		dialog->setWindowTitle("Layers");
		dialog->resize(375, 600);

		QtWidgetUtils::reposition_to_side_of_parent(dialog);

		d_layering_dialog_ptr.reset(dialog);
	}

	if (visible)
	{
		d_layering_dialog_ptr->show();
		// In most cases, 'show()' is sufficient. However, selecting the menu entry
		// a second time, when the dialog is still open, should make the dialog 'active'
		// and return keyboard focus to it.
		d_layering_dialog_ptr->activateWindow();
		// On platforms which do not keep dialogs on top of their parent, a call to
		// raise() may also be necessary to properly 're-pop-up' the dialog.
		d_layering_dialog_ptr->raise();
	}
	else
	{
		d_layering_dialog_ptr->hide();
	}
}


void
GPlatesQtWidgets::ViewportWindow::handle_layers_menu_about_to_show()
{
	action_Show_Layers->setChecked(d_layering_dialog_ptr && d_layering_dialog_ptr->isVisible());
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_colouring_dialog()
{
	if (!d_colouring_dialog_ptr)
	{
		d_colouring_dialog_ptr.reset(
				new ColouringDialog(
					get_view_state(),
					d_reconstruction_view_widget.globe_and_map_widget(),
					*d_read_errors_dialog_ptr,
					this));
	}

	d_colouring_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_colouring_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_colouring_dialog_ptr->raise();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_data_association_dialog()
{
	if (!d_data_association_dialog_ptr)
	{
		d_data_association_dialog_ptr.reset(
				new DataAssociationDialog(
						get_application_state(),
						get_view_state(),
						this));
	}
	d_data_association_dialog_ptr->pop_up_dialog();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_export_animation_dialog()
{
	if (!d_export_animation_dialog_ptr)
	{
		d_export_animation_dialog_ptr.reset(new ExportAnimationDialog(d_animation_controller, get_view_state(), *this, this));
	}

	// FIXME: Should Export Animation be modal?
	d_export_animation_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_export_animation_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_export_animation_dialog_ptr->raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_assign_reconstruction_plate_ids_dialog()
{
	// d_assign_recon_plate_ids_dialog_ptr is modal and should not need the 'raise' hack
	// other dialogs use.
	d_assign_recon_plate_ids_dialog_ptr->exec_partition_features_dialog();
}


void
GPlatesQtWidgets::ViewportWindow::enable_drag_globe_tool(
		bool enable)
{
	action_Drag_Globe->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_zoom_globe_tool(
		bool enable)
{
	action_Zoom_Globe->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_click_geometry_tool(
		bool enable)
{
	action_Click_Geometry->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_digitise_polyline_tool(
		bool enable)
{
	action_Digitise_New_Polyline->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_digitise_multipoint_tool(
		bool enable)
{
	action_Digitise_New_MultiPoint->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_digitise_polygon_tool(
		bool enable)
{
	action_Digitise_New_Polygon->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_move_geometry_tool(
		bool enable)
{
	action_Move_Geometry->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_move_vertex_tool(
		bool enable)
{
	action_Move_Vertex->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_delete_vertex_tool(
		bool enable)
{
	action_Delete_Vertex->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::enable_insert_vertex_tool(
		bool enable)
{
	action_Insert_Vertex->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_split_feature_tool(
	bool enable)
{
	action_Split_Feature->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_manipulate_pole_tool(
		bool enable)
{
	action_Manipulate_Pole->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_build_topology_tool(
		bool enable)
{
	action_Build_Topology->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_edit_topology_tool(
		bool enable)
{
	action_Edit_Topology->setEnabled(enable);
}

void
GPlatesQtWidgets::ViewportWindow::enable_measure_distance_tool(
		bool enable)
{
	action_Measure_Distance->setEnabled(enable);
}


void
GPlatesQtWidgets::ViewportWindow::handle_drag_globe_triggered()
{
	d_choose_canvas_tool->choose_drag_globe_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::DRAG_GLOBE;
}


void
GPlatesQtWidgets::ViewportWindow::choose_drag_globe_tool()
{
	action_Drag_Globe->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_reorient_globe_tool();
	d_map_canvas_tool_choice_ptr->choose_pan_map_tool();
}


void
GPlatesQtWidgets::ViewportWindow::handle_zoom_globe_triggered()
{
	d_choose_canvas_tool->choose_zoom_globe_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::ZOOM_GLOBE;
}


void
GPlatesQtWidgets::ViewportWindow::choose_zoom_globe_tool()
{
	action_Zoom_Globe->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_zoom_globe_tool();
	d_map_canvas_tool_choice_ptr->choose_zoom_map_tool();
}


void
GPlatesQtWidgets::ViewportWindow::handle_click_geometry_triggered()
{
	d_choose_canvas_tool->choose_click_geometry_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::CLICK_GEOMETRY;
}


void
GPlatesQtWidgets::ViewportWindow::choose_click_geometry_tool()
{
	action_Click_Geometry->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_click_geometry_tool();
	d_map_canvas_tool_choice_ptr->choose_click_geometry_tool();

	d_task_panel_ptr->choose_feature_tab();
}


void
GPlatesQtWidgets::ViewportWindow::handle_digitise_new_polyline_triggered()
{
	d_choose_canvas_tool->choose_digitise_polyline_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYLINE;
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polyline_tool()
{
	action_Digitise_New_Polyline->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_digitise_polyline_tool();
	d_map_canvas_tool_choice_ptr->choose_digitise_polyline_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::handle_digitise_new_multipoint_triggered()
{
	d_choose_canvas_tool->choose_digitise_multipoint_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::DIGITISE_MULTIPOINT;
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_multipoint_tool()
{
	action_Digitise_New_MultiPoint->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_digitise_multipoint_tool();
	d_map_canvas_tool_choice_ptr->choose_digitise_multipoint_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::handle_digitise_new_polygon_triggered()
{
	d_choose_canvas_tool->choose_digitise_polygon_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYGON;
}


void
GPlatesQtWidgets::ViewportWindow::choose_digitise_polygon_tool()
{
	action_Digitise_New_Polygon->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_digitise_polygon_tool();
	d_map_canvas_tool_choice_ptr->choose_digitise_polygon_tool();
	d_task_panel_ptr->choose_digitisation_tab();
}


void
GPlatesQtWidgets::ViewportWindow::choose_move_geometry_tool()
{
	// The MoveGeometry tool is not yet implemented.
#if 0
	action_Move_Geometry->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_move_geometry_tool();
	d_task_panel_ptr->choose_feature_tab();
#endif
}


void
GPlatesQtWidgets::ViewportWindow::handle_move_vertex_triggered()
{
	d_choose_canvas_tool->choose_move_vertex_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::MOVE_VERTEX;
}


void
GPlatesQtWidgets::ViewportWindow::choose_move_vertex_tool()
{
	action_Move_Vertex->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_move_vertex_tool();
	d_map_canvas_tool_choice_ptr->choose_move_vertex_tool();
	d_task_panel_ptr->choose_modify_geometry_tab();
	d_task_panel_ptr->enable_move_nearby_vertices_widget(true);
}


void
GPlatesQtWidgets::ViewportWindow::handle_delete_vertex_triggered()
{
	d_choose_canvas_tool->choose_delete_vertex_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::DELETE_VERTEX;
}


void
GPlatesQtWidgets::ViewportWindow::choose_delete_vertex_tool()
{
	action_Delete_Vertex->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_delete_vertex_tool();
	d_map_canvas_tool_choice_ptr->choose_delete_vertex_tool();

	d_task_panel_ptr->choose_modify_geometry_tab();
	d_task_panel_ptr->enable_move_nearby_vertices_widget(false);	
}


void
GPlatesQtWidgets::ViewportWindow::handle_insert_vertex_triggered()
{
	d_choose_canvas_tool->choose_insert_vertex_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::INSERT_VERTEX;
}


void
GPlatesQtWidgets::ViewportWindow::choose_insert_vertex_tool()
{
	action_Insert_Vertex->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_insert_vertex_tool();
	d_map_canvas_tool_choice_ptr->choose_insert_vertex_tool();

	d_task_panel_ptr->choose_modify_geometry_tab();
	d_task_panel_ptr->enable_move_nearby_vertices_widget(false);	
}


void
GPlatesQtWidgets::ViewportWindow::handle_split_feature_triggered()
{
	d_choose_canvas_tool->choose_split_feature_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::SPLIT_FEATURE;
}


void
GPlatesQtWidgets::ViewportWindow::choose_split_feature_tool()
{
	action_Split_Feature->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_split_feature_tool();
	d_map_canvas_tool_choice_ptr->choose_split_feature_tool();

	d_task_panel_ptr->choose_modify_geometry_tab();
}


void
GPlatesQtWidgets::ViewportWindow::handle_measure_distance_triggered()
{
	d_choose_canvas_tool->choose_measure_distance_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::MEASURE_DISTANCE;
}


void
GPlatesQtWidgets::ViewportWindow::choose_measure_distance_tool()
{
	action_Measure_Distance->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_measure_distance_tool();
	d_map_canvas_tool_choice_ptr->choose_measure_distance_tool();

	d_task_panel_ptr->choose_measure_distance_tab();
}


void
GPlatesQtWidgets::ViewportWindow::handle_manipulate_pole_triggered()
{
	d_choose_canvas_tool->choose_manipulate_pole_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::MANIPULATE_POLE;
}


void
GPlatesQtWidgets::ViewportWindow::choose_manipulate_pole_tool()
{
	action_Manipulate_Pole->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_manipulate_pole_tool();
	d_map_canvas_tool_choice_ptr->choose_manipulate_pole_tool();

	d_task_panel_ptr->choose_modify_pole_tab();
}


void
GPlatesQtWidgets::ViewportWindow::handle_build_topology_triggered()
{
	d_choose_canvas_tool->choose_build_topology_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::BUILD_TOPOLOGY;
}


void
GPlatesQtWidgets::ViewportWindow::choose_build_topology_tool()
{
	action_Build_Topology->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_build_topology_tool();
	d_map_canvas_tool_choice_ptr->choose_build_topology_tool();

	d_task_panel_ptr->choose_topology_tools_tab();
}


void
GPlatesQtWidgets::ViewportWindow::handle_edit_topology_triggered()
{
	d_choose_canvas_tool->choose_edit_topology_tool();
	d_canvas_tool_last_chosen_by_user = GPlatesCanvasTools::CanvasToolType::EDIT_TOPOLOGY;
}


void
GPlatesQtWidgets::ViewportWindow::choose_edit_topology_tool()
{
	action_Edit_Topology->setChecked(true);
	d_globe_canvas_tool_choice_ptr->choose_edit_topology_tool();
	d_map_canvas_tool_choice_ptr->choose_edit_topology_tool();

	d_task_panel_ptr->choose_topology_tools_tab();
}


void
GPlatesQtWidgets::ViewportWindow::enable_or_disable_feature_actions(
		GPlatesGui::FeatureFocus &feature_focus)
{
	// Note: enabling/disabling canvas tools is now done in class 'EnableCanvasTool'.
	bool enable_canvas_tool_actions = feature_focus.focused_feature().is_valid();
	
	action_Query_Feature->setEnabled(enable_canvas_tool_actions);
	action_Edit_Feature->setEnabled(enable_canvas_tool_actions);
	action_Delete_Feature->setEnabled(enable_canvas_tool_actions);
	action_Clear_Selection->setEnabled(enable_canvas_tool_actions);
	action_Clone_Feature->setEnabled(enable_canvas_tool_actions);

	if (enable_canvas_tool_actions && d_focused_feature_geometry_builder->has_geometry())
	{
		action_Clone_Geometry->setEnabled(true);
	}
	else
	{
		action_Clone_Geometry->setEnabled(false);
	}

#if 0
	// FIXME: Add to Selection is unimplemented and should stay disabled for now.
	// FIXME: To handle the "Remove from Selection", "Clear Selection" actions,
	// we may want to modify this method to also test for a nonempty selection of features.
	action_Add_Feature_To_Selection->setEnabled(enable_canvas_tool_actions);
#endif
}


void
GPlatesQtWidgets::ViewportWindow::use_small_canvas_tool_icons(
		bool use_small_icons)
{
	int s = use_small_icons ? 16 : 35;
	toolbar_canvas_tools->setIconSize(QSize(s, s));
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_read_errors_dialog()
{
	// note: this dialog is created in the constructor
	
	d_read_errors_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_read_errors_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_read_errors_dialog_ptr->raise();

	// Finally, if we're showing the Read Errors dialog, the user already knows about
	// the errors and doesn't need to see the reminder in the status bar.
	d_trinket_area_ptr->read_errors_trinket().setVisible(false);
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_manage_feature_collections_dialog()
{
	// note: this dialog is created in the constructor
	
	d_manage_feature_collections_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_manage_feature_collections_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_manage_feature_collections_dialog_ptr->raise();
}

#if 0
void
GPlatesQtWidgets::ViewportWindow::pop_up_export_geometry_snapshot_dialog()
{
	if (!d_export_geometry_snapshot_dialog_ptr)
	{
		SaveFileDialog::filter_list_type filters;
		filters.push_back(std::make_pair("SVG file (*.svg)", "svg"));
		d_export_geometry_snapshot_dialog_ptr = SaveFileDialog::get_save_file_dialog(
				this,
				"Export Geometry Snapshot",
				filters);
	}

	boost::optional<QString> filename = d_export_geometry_snapshot_dialog_ptr->get_file_name();
	if (filename)
	{
		create_svg_file(*filename);
	}
}
#endif 
void
GPlatesQtWidgets::ViewportWindow::create_svg_file(
		const QString &filename)
{
#if 0
	bool result = GPlatesGui::SvgExport::create_svg_output(filename,d_globe_canvas_ptr);
	if (!result){
		std::cerr << "Error creating SVG output.." << std::endl;
	}
#endif
	try{
		d_reconstruction_view_widget.active_view().create_svg_output(filename);
	}
	catch(...)
	{
		std::cerr << "Caught exception creating SVG output." << std::endl;
	}
}


void
GPlatesQtWidgets::ViewportWindow::close_all_dialogs()
{
	if (d_about_dialog_ptr)
	{
		d_about_dialog_ptr->reject();
	}
	if (d_animate_dialog_ptr)
	{
		d_animate_dialog_ptr->reject();
	}
	if (d_assign_recon_plate_ids_dialog_ptr)
	{
		d_assign_recon_plate_ids_dialog_ptr->reject();
	}
	if (d_calculate_reconstruction_pole_dialog_ptr)
	{
		d_calculate_reconstruction_pole_dialog_ptr->reject();
	}
	if (d_colouring_dialog_ptr)
	{
		d_colouring_dialog_ptr->reject();
	}
	if (d_create_vgp_dialog_ptr)
	{
		d_create_vgp_dialog_ptr->reject();
	}
	if (d_mesh_dialog_ptr)
	{
		d_mesh_dialog_ptr->reject();
	}
	if (d_export_animation_dialog_ptr)
	{
		d_export_animation_dialog_ptr->reject();
	}
	if (d_feature_properties_dialog_ptr)
	{
		d_feature_properties_dialog_ptr->reject();
	}
	if (d_manage_feature_collections_dialog_ptr)
	{
		d_manage_feature_collections_dialog_ptr->reject();
	}
	if (d_read_errors_dialog_ptr)
	{
		d_read_errors_dialog_ptr->reject();
	}
	if (d_set_camera_viewpoint_dialog_ptr)
	{
		d_set_camera_viewpoint_dialog_ptr->reject();
	}
	if (d_set_projection_dialog_ptr)
	{
		d_set_projection_dialog_ptr->reject();
	}
	if (d_set_vgp_visibility_dialog_ptr)
	{
		d_set_vgp_visibility_dialog_ptr->reject();
	}
	if (d_shapefile_attribute_viewer_dialog_ptr)
	{
		d_shapefile_attribute_viewer_dialog_ptr->reject();
	}
	if (d_specify_anchored_plate_id_dialog_ptr)
	{
		d_specify_anchored_plate_id_dialog_ptr->reject();
	}
	if (d_total_reconstruction_poles_dialog_ptr)
	{
		d_total_reconstruction_poles_dialog_ptr->reject();
	}
	if (d_total_reconstruction_sequences_dialog_ptr)
	{
		d_total_reconstruction_sequences_dialog_ptr->reject();
	}
	if(d_data_association_dialog_ptr)
	{
		d_data_association_dialog_ptr->reject();
	}
	
}

void
GPlatesQtWidgets::ViewportWindow::closeEvent(
		QCloseEvent *close_event)
{
	// Remember the current set of loaded files for next time.
	get_application_state().get_session_management().close_event_hook();

	// Check for unsaved changes and potentially give the user a chance to save/abort/etc.
	bool close_ok = d_unsaved_changes_tracker_ptr->close_event_hook();
	if (close_ok) {
		// User is OK with quitting GPlates at this point.
		close_event->accept();
		// If we decide to accept the close event, we should also tidy up after ourselves.
		close_all_dialogs();
	} else {
		// User is Not OK with quitting GPlates at this point.
		close_event->ignore();
	}
}


void
GPlatesQtWidgets::ViewportWindow::dragEnterEvent(
		QDragEnterEvent *ev)
{
	if (ev->mimeData()->hasUrls()) {
		ev->acceptProposedAction();
	} else {
		ev->ignore();
	}
}

void
GPlatesQtWidgets::ViewportWindow::dropEvent(
		QDropEvent *ev)
{
	if (ev->mimeData()->hasUrls()) {
		ev->acceptProposedAction();
		d_file_io_feedback_ptr->open_urls(ev->mimeData()->urls());
	} else {
		ev->ignore();
	}
}


void
GPlatesQtWidgets::ViewportWindow::enable_point_display()
{
	get_view_state().get_render_settings().set_show_points(
			action_Show_Point_Features->isChecked());
}


void
GPlatesQtWidgets::ViewportWindow::enable_line_display()
{
	get_view_state().get_render_settings().set_show_lines(
			action_Show_Line_Features->isChecked());
}


void
GPlatesQtWidgets::ViewportWindow::enable_polygon_display()
{
	get_view_state().get_render_settings().set_show_polygons(
			action_Show_Polygon_Features->isChecked());
}


void
GPlatesQtWidgets::ViewportWindow::enable_multipoint_display()
{
	get_view_state().get_render_settings().set_show_multipoints(
			action_Show_Multipoint_Features->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_arrows_display()
{
	get_view_state().get_render_settings().set_show_arrows(
			action_Show_Arrow_Decorations->isChecked());
}

void
GPlatesQtWidgets::ViewportWindow::enable_strings_display()
{
	get_view_state().get_render_settings().set_show_strings(
			action_Show_Strings->isChecked());
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
GPlatesQtWidgets::ViewportWindow::pop_up_import_raster_dialog(
		bool time_dependent_raster)
{
	// Note: the ImportRasterDialog needs to be reconstructed each time we want to
	// use it, unlike the other dialogs, otherwise the pages are incorrectly initialised.
	ImportRasterDialog import_raster_dialog(
			get_application_state(),
			get_view_state(),
			d_unsaved_changes_tracker_ptr.data(),
			d_file_io_feedback_ptr.data(),
			this);

	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog_ptr->read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();

	import_raster_dialog.display(time_dependent_raster, &read_errors);

	d_read_errors_dialog_ptr->update();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors)
	{
		d_read_errors_dialog_ptr->show();
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
GPlatesQtWidgets::ViewportWindow::update_tools_and_status_message()
{	
	bool globe_is_active = d_reconstruction_view_widget.globe_is_active();
	action_Show_Arrow_Decorations->setEnabled(globe_is_active);
	action_Show_Stars->setEnabled(globe_is_active);
}


void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_up()
{
	d_reconstruction_view_widget.active_view().move_camera_up();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_down()
{
	d_reconstruction_view_widget.active_view().move_camera_down();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_left()
{
	d_reconstruction_view_widget.active_view().move_camera_left();
}

void
GPlatesQtWidgets::ViewportWindow::handle_move_camera_right()
{
	d_reconstruction_view_widget.active_view().move_camera_right();
}

void
GPlatesQtWidgets::ViewportWindow::handle_rotate_camera_clockwise()
{
	d_reconstruction_view_widget.active_view().rotate_camera_clockwise();
}

void
GPlatesQtWidgets::ViewportWindow::handle_rotate_camera_anticlockwise()
{
	d_reconstruction_view_widget.active_view().rotate_camera_anticlockwise();
}

void
GPlatesQtWidgets::ViewportWindow::handle_reset_camera_orientation()
{
	d_reconstruction_view_widget.active_view().reset_camera_orientation();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_set_projection_dialog()
{
	if (!d_set_projection_dialog_ptr)
	{
		d_set_projection_dialog_ptr.reset(new SetProjectionDialog(*this,this));
	}

	d_set_projection_dialog_ptr->setup();

	if (d_set_projection_dialog_ptr->exec())
	{
		try {
			// Notify the view state of the projection change.
			// It will handle the rest.
			GPlatesGui::ViewportProjection &viewport_projection =
					get_view_state().get_viewport_projection();
			viewport_projection.set_projection_type(
					d_set_projection_dialog_ptr->get_projection_type());
			viewport_projection.set_central_meridian(
					d_set_projection_dialog_ptr->central_meridian());
		}
		catch(GPlatesGui::ProjectionException &e)
		{
			std::cerr << e << std::endl;
		}
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
	static GPlatesGui::GuiDebug *gui_debug = new GPlatesGui::GuiDebug(*this,
			get_application_state(), this);
	gui_debug->setObjectName("GuiDebug");
}

void
GPlatesQtWidgets::ViewportWindow::install_data_mining_menu()
{
	QMenu *data_mining_menu = new QMenu(tr("&Data Mining"), this);
	menuBar()->addMenu(data_mining_menu);
	data_mining_menu->setTearOffEnabled(true);

	// Add and connect actions to the menu.
	QAction *actionData_Association = new QAction(tr("Data Association"), this);
	data_mining_menu->addAction(actionData_Association);
	QObject::connect(actionData_Association, SIGNAL(triggered()),
		this, SLOT(pop_up_data_association_dialog()));
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_shapefile_attribute_viewer_dialog()
{
	// note: this dialog is created in the constructor
	
	d_shapefile_attribute_viewer_dialog_ptr->show();
	d_shapefile_attribute_viewer_dialog_ptr->update(
			get_application_state().get_feature_collection_file_state());
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_shapefile_attribute_viewer_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_shapefile_attribute_viewer_dialog_ptr->raise();
}


void
GPlatesQtWidgets::ViewportWindow::pop_up_total_reconstruction_sequences_dialog()
{
	// note: this dialog is created in the constructor
	
	d_total_reconstruction_sequences_dialog_ptr->show();
	d_total_reconstruction_sequences_dialog_ptr->update();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_total_reconstruction_sequences_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_total_reconstruction_sequences_dialog_ptr->raise();
}


void
GPlatesQtWidgets::ViewportWindow::generate_mesh_cap()
{
	if (!d_mesh_dialog_ptr)
	{
		d_mesh_dialog_ptr.reset(
				new MeshDialog(
				get_view_state(),
				*d_manage_feature_collections_dialog_ptr,
				this));
	}

	d_mesh_dialog_ptr->show();
	d_mesh_dialog_ptr->activateWindow();
	d_mesh_dialog_ptr->raise();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_create_vgp_dialog()
{
	if (!d_create_vgp_dialog_ptr)
	{
		d_create_vgp_dialog_ptr.reset(new CreateVGPDialog(get_view_state(),this));
	}

	d_create_vgp_dialog_ptr->reset();
	d_create_vgp_dialog_ptr->exec();

}

void
GPlatesQtWidgets::ViewportWindow::pop_up_calculate_reconstruction_pole_dialog()
{
	if (!d_calculate_reconstruction_pole_dialog_ptr)
	{
		d_calculate_reconstruction_pole_dialog_ptr.reset(
			new CalculateReconstructionPoleDialog(get_view_state(),this));
	}

	d_calculate_reconstruction_pole_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_calculate_reconstruction_pole_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_calculate_reconstruction_pole_dialog_ptr->raise();
}

void
GPlatesQtWidgets::ViewportWindow::pop_up_set_vgp_visibility_dialog()
{

	if (!d_set_vgp_visibility_dialog_ptr)
	{
		d_set_vgp_visibility_dialog_ptr.reset(new SetVGPVisibilityDialog(get_view_state(),this));
	}

	// SetVGPVisbilityDialog is modal. 
	d_set_vgp_visibility_dialog_ptr->exec();
}

void
GPlatesQtWidgets::ViewportWindow::handle_colour_scheme_delegator_changed()
{
	d_reconstruction_view_widget.globe_and_map_widget().update_canvas();
}


void
GPlatesQtWidgets::ViewportWindow::set_internal_release_window_title()
{
	QString subversion_branch_name = GPlatesGlobal::SubversionInfo::get_working_copy_branch_name();

	// If the subversion branch name is the empty string, that should mean that
	// GPLATES_SOURCE_CODE_CONTROL_VERSION_STRING is set in ConfigDefault.cmake,
	// which should mean that this is a public release.
	if (!subversion_branch_name.isEmpty())
	{
		QString subversion_version_number = GPlatesGlobal::SubversionInfo::get_working_copy_version_number();
		QString window_title = windowTitle();

		if (subversion_version_number.isEmpty())
		{
			static const QString FORMAT = " (%1)";
			window_title.append(FORMAT.arg(subversion_branch_name));
		}
		else
		{
			static const QString FORMAT = " (%1, r%2)";

			window_title.append(FORMAT.arg(subversion_branch_name, subversion_version_number));
		}

		setWindowTitle(window_title);
	}
}


void
GPlatesQtWidgets::ViewportWindow::restore_canvas_tool_last_chosen_by_user()
{
	using namespace GPlatesCanvasTools::CanvasToolType;

	switch (d_canvas_tool_last_chosen_by_user)
	{
		case DRAG_GLOBE:
			d_choose_canvas_tool->choose_drag_globe_tool();
			break;

		case ZOOM_GLOBE:
			d_choose_canvas_tool->choose_zoom_globe_tool();
			break;

		case CLICK_GEOMETRY:
			d_choose_canvas_tool->choose_click_geometry_tool();
			break;

		case DIGITISE_POLYLINE:
			d_choose_canvas_tool->choose_digitise_polyline_tool();
			break;

		case DIGITISE_MULTIPOINT:
			d_choose_canvas_tool->choose_digitise_multipoint_tool();
			break;

		case DIGITISE_POLYGON:
			d_choose_canvas_tool->choose_digitise_polygon_tool();
			break;

		case MOVE_GEOMETRY:
			d_choose_canvas_tool->choose_move_geometry_tool();
			break;

		case MOVE_VERTEX:
			d_choose_canvas_tool->choose_move_vertex_tool();
			break;

		case DELETE_VERTEX:
			d_choose_canvas_tool->choose_delete_vertex_tool();
			break;

		case INSERT_VERTEX:
			d_choose_canvas_tool->choose_insert_vertex_tool();
			break;

		case SPLIT_FEATURE:
			d_choose_canvas_tool->choose_split_feature_tool();
			break;

		case MANIPULATE_POLE:
			d_choose_canvas_tool->choose_manipulate_pole_tool();
			break;

		case BUILD_TOPOLOGY:
			d_choose_canvas_tool->choose_build_topology_tool();
			break;

		case EDIT_TOPOLOGY:
			d_choose_canvas_tool->choose_edit_topology_tool();
			break;

		case MEASURE_DISTANCE:
			d_choose_canvas_tool->choose_measure_distance_tool();
			break;

		default:
			// Do nothing.
			break;
	}
}


void
GPlatesQtWidgets::ViewportWindow::handle_visual_layer_added(
		size_t added)
{
	set_layering_dialog_visibility(true);

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
GPlatesQtWidgets::ViewportWindow::pop_up_small_circle_manager()
{
	if (!d_small_circle_manager_ptr)
	{
		d_small_circle_manager_ptr.reset(
			new SmallCircleManager(
				get_view_state().get_rendered_geometry_collection(),
				get_application_state(),
				this));
	}

	d_small_circle_manager_ptr->show();
	d_small_circle_manager_ptr->activateWindow();
	d_small_circle_manager_ptr->raise();
}

void
GPlatesQtWidgets::ViewportWindow::status_message(
		const QString &message,
		int timeout)
{
#ifdef Q_WS_MAC
	QString cloverleaf(QChar(0x2318));
	QString fixed_message = message;
	fixed_message.replace(QString("ctrl"), cloverleaf, Qt::CaseInsensitive);
	statusBar()->showMessage(fixed_message, timeout);
#else
	statusBar()->showMessage(message, timeout);
#endif
}

