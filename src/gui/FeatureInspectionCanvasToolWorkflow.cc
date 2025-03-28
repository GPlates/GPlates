/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "FeatureInspectionCanvasToolWorkflow.h"

#include "AddClickedGeometriesToFeatureTable.h"
#include "Dialogs.h"
#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ScalarCoverageFeatureProperties.h"
#include "app-logic/TopologyReconstructedFeatureGeometry.h"
#include "app-logic/TopologyUtils.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/DeleteVertex.h"
#include "canvas-tools/GeometryOperationState.h"
#include "canvas-tools/InsertVertex.h"
#include "canvas-tools/MeasureDistance.h"
#include "canvas-tools/MoveVertex.h"
#include "canvas-tools/SplitFeature.h"

#include "global/GPlatesAssert.h"

#include "gui/CanvasToolWorkflows.h"
#include "gui/GeometryFocusHighlight.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/GeometryBuilder.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryParameters.h"


namespace GPlatesGui
{
	namespace
	{
		/**
		 * The main rendered layer used by this canvas tool workflow.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType WORKFLOW_RENDER_LAYER =
				GPlatesViewOperations::RenderedGeometryCollection::FEATURE_INSPECTION_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}

GPlatesGui::FeatureInspectionCanvasToolWorkflow::FeatureInspectionCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_canvas(),
			viewport_window.map_view(),
			CanvasToolWorkflows::WORKFLOW_FEATURE_INSPECTION,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_CLICK_GEOMETRY),
	d_canvas_tool_workflows(canvas_tool_workflows),
	d_feature_focus(view_state.get_feature_focus()),
	d_focused_feature_geometry_builder(view_state.get_focused_feature_geometry_builder()),
	d_geometry_operation_state(geometry_operation_state),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection()),
	d_rendered_geometry_parameters(view_state.get_rendered_geometry_parameters()),
	d_render_settings(view_state.get_render_settings()),
	d_symbol_map(view_state.get_feature_type_symbol_map()),
	d_application_state(view_state.get_application_state()),
	d_viewport_window(viewport_window)
{
	create_canvas_tools(
			canvas_tool_workflows,
			geometry_operation_state,
			modify_geometry_state,
			measure_distance_state,
			status_bar_callback,
			view_state,
			viewport_window);

	// Listen for focus feature signals.
	QObject::connect(
		&d_feature_focus,
		SIGNAL(focus_changed(
				GPlatesGui::FeatureFocus &)),
		this,
		SLOT(update_enable_state()));

	// Listen for focused feature geometry changes.
	// We use this to determine if a geometry that's being operated on, or will potentially
	// be operated on, has got vertices or not.
	QObject::connect(
			&d_focused_feature_geometry_builder,
			SIGNAL(stopped_updating_geometry_excluding_intermediate_moves()),
			this,
			SLOT(update_enable_state()));
}


void
GPlatesGui::FeatureInspectionCanvasToolWorkflow::create_canvas_tools(
		CanvasToolWorkflows &canvas_tool_workflows,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
	//
	// Measure distance canvas tool.
	//
	// NOTE: There's also a Measure Distance tool in the Digitisation workflow, but we also have one
	// in the Feature Inspection workflow because it is hooked up to the focused feature geometry.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type measure_distance_tool =
		GPlatesCanvasTools::MeasureDistance::create(
				status_bar_callback,
				view_state.get_focused_feature_geometry_builder(),
				geometry_operation_state,
				view_state.get_rendered_geometry_collection(),
				WORKFLOW_RENDER_LAYER,
				measure_distance_state);
	// For the globe view.
	d_globe_measure_distance_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					measure_distance_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_measure_distance_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					measure_distance_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Click geometry canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type click_geometry_tool =
			GPlatesCanvasTools::ClickGeometry::create(
					status_bar_callback,
					view_state.get_focused_feature_geometry_builder(),
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					viewport_window,
					view_state.get_feature_table_model(),
					viewport_window.dialogs().feature_properties_dialog(),
					view_state.get_feature_focus(),
					view_state.get_application_state());
	// For the globe view.
	d_globe_click_geometry_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					click_geometry_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_click_geometry_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					click_geometry_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Move vertex canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type move_vertex_tool =
			GPlatesCanvasTools::MoveVertex::create(
					status_bar_callback,
					view_state.get_focused_feature_geometry_builder(),
					geometry_operation_state,
					modify_geometry_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget(),
					view_state.get_feature_focus());
	// For the globe view.
	d_globe_move_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					move_vertex_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_move_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					move_vertex_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Delete vertex canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type delete_vertex_tool =
			GPlatesCanvasTools::DeleteVertex::create(
					status_bar_callback,
					view_state.get_focused_feature_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_delete_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					delete_vertex_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_delete_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					delete_vertex_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Insert vertex canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type insert_vertex_tool =
			GPlatesCanvasTools::InsertVertex::create(
					status_bar_callback,
					view_state.get_focused_feature_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_insert_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					insert_vertex_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_insert_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					insert_vertex_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Split feature canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type split_feature_tool =
			GPlatesCanvasTools::SplitFeature::create(
					status_bar_callback,
					view_state.get_feature_focus(),
					view_state.get_application_state().get_model_interface(),
					view_state.get_focused_feature_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_split_feature_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					split_feature_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_split_feature_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					split_feature_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));
}


void
GPlatesGui::FeatureInspectionCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.

	// The measure distance tool can do measurements without a focused feature so we leave it enabled always.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MEASURE_DISTANCE, true);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_CLICK_GEOMETRY, true);

	update_enable_state();
}


void
GPlatesGui::FeatureInspectionCanvasToolWorkflow::activate_workflow()
{
	// Let others know the currently activated GeometryBuilder.
	d_geometry_operation_state.set_active_geometry_builder(&d_focused_feature_geometry_builder);

	// Activate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, true/*active*/);

	// Draw the focused feature when it changes feature or is modified.
	QObject::connect(
			&d_feature_focus,
			SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(draw_feature_focus()));
	QObject::connect(
			&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(draw_feature_focus()));

	// Re-draw the focused feature when the render geometry parameters change.
	QObject::connect(
			&d_rendered_geometry_parameters,
			SIGNAL(parameters_changed(GPlatesViewOperations::RenderedGeometryParameters &)),
			this,
			SLOT(draw_feature_focus()));

	// Draw the focused feature (or draw nothing) in case the focused feature changed while we were inactive.
	draw_feature_focus();
}


void
GPlatesGui::FeatureInspectionCanvasToolWorkflow::deactivate_workflow()
{
	// Let others know there's no currently activated GeometryBuilder.
	d_geometry_operation_state.set_no_active_geometry_builder();

	// Deactivate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, false/*active*/);

	// Don't draw the focused feature anymore.
	QObject::disconnect(
			&d_feature_focus,
			SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(draw_feature_focus()));
	QObject::disconnect(
			&d_feature_focus,
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(draw_feature_focus()));
	QObject::disconnect(
			&d_rendered_geometry_parameters,
			SIGNAL(parameters_changed(GPlatesViewOperations::RenderedGeometryParameters &)),
			this,
			SLOT(draw_feature_focus()));
}


boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
GPlatesGui::FeatureInspectionCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_MEASURE_DISTANCE:
		return std::make_pair(d_globe_measure_distance_tool.get(), d_map_measure_distance_tool.get());

	case CanvasToolWorkflows::TOOL_CLICK_GEOMETRY:
		return std::make_pair(d_globe_click_geometry_tool.get(), d_map_click_geometry_tool.get());

	case CanvasToolWorkflows::TOOL_MOVE_VERTEX:
		return std::make_pair(d_globe_move_vertex_tool.get(), d_map_move_vertex_tool.get());

	case CanvasToolWorkflows::TOOL_DELETE_VERTEX:
		return std::make_pair(d_globe_delete_vertex_tool.get(), d_map_delete_vertex_tool.get());

	case CanvasToolWorkflows::TOOL_INSERT_VERTEX:
		return std::make_pair(d_globe_insert_vertex_tool.get(), d_map_insert_vertex_tool.get());

	case CanvasToolWorkflows::TOOL_SPLIT_FEATURE:
		return std::make_pair(d_globe_split_feature_tool.get(), d_map_split_feature_tool.get());

	default:
		break;
	}

	return boost::none;
}


void
GPlatesGui::FeatureInspectionCanvasToolWorkflow::draw_feature_focus()
{
	GeometryFocusHighlight::draw_focused_geometry(
			d_feature_focus,
			*d_rendered_geom_collection.get_main_rendered_layer(WORKFLOW_RENDER_LAYER),
			d_rendered_geom_collection,
			d_rendered_geometry_parameters,
			d_render_settings,
			d_application_state.get_current_topological_sections(),
			d_application_state.get_current_reconstruction().get_all_resolved_topological_shared_sub_segments(),
			d_symbol_map);
}


void
GPlatesGui::FeatureInspectionCanvasToolWorkflow::update_enable_state()
{
	const GPlatesModel::FeatureHandle::const_weak_ref focused_feature = d_feature_focus.focused_feature();

	// If there's no focused feature or it's a topological feature then most of the tools are disabled.
	if (!focused_feature.is_valid() ||
		GPlatesAppLogic::TopologyUtils::is_topological_feature(focused_feature))
	{
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MOVE_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_INSERT_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DELETE_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_SPLIT_FEATURE, false);

		return;
	}
	// ...if we get here then the focused feature is valid and non-topological.

	// If the focused feature is being reconstructed by topologies then disable the edit canvas tools
	// until we implement the ability to edit them, for the following reasons...
	//
	// FIXME: Currently topology-reconstructed feature geometries use the 'gpml:geometryImportTime'
	// feature property as the start time for forward and backward reconstruction by topologies.
	// And in both directions the geometries can have deactivated points (due to subduction going
	// forward in time) and consumption by mid-ocean ridges (going backward in time). So if the user
	// is editing a geometry at a time when some points are de-activated then when the edited geometry
	// gets set back in feature it will essentially lose some points. Also the edited geometries get
	// reverse-reconstructed to present day when stored back in the feature, and if this is done at
	// a time other than the geometry import time then it will not be correct.
	//
	// So for now we just disable all edit tools in this situation by detecting RFGs of type
	// TopologyReconstructedFeatureGeometry.
	//
	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry =
			d_feature_focus.associated_reconstruction_geometry();
	if (focused_geometry &&
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
				const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *>(focused_geometry.get()))
	{
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MOVE_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_INSERT_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DELETE_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_SPLIT_FEATURE, false);

		return;
	}

	const std::pair<unsigned int, GPlatesMaths::GeometryType::Value> geometry_builder_parameters =
			get_geometry_builder_parameters();
	const unsigned int num_vertices = geometry_builder_parameters.first;
	const GPlatesMaths::GeometryType::Value geometry_type = geometry_builder_parameters.second;

	// Enable the move vertex tool if there's at least one vertex regardless of the geometry type.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MOVE_VERTEX, num_vertices > 0);

	// If the focused feature has per-point scalar values then disable all tools that change the
	// number of points since this loses the mapping between the feature's domain points and the
	// feature's scalar values. The only tool that doesn't do this is the move vertex tool.
	//
	// TODO: Implement the ability for the user to:
	//   (1) insert a new scalar values when inserting a new vertex,
	//   (2) delete the appropriate scalar value when deleting a vertex,
	//   (3) divide the scalar values between features when splitting a feature.
	if (GPlatesAppLogic::ScalarCoverageFeatureProperties::is_scalar_coverage_feature(focused_feature))
	{
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_INSERT_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DELETE_VERTEX, false);
		emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_SPLIT_FEATURE, false);

		return;
	}

	// Enable the insert vertex tool if inserting a vertex won't change the type of
	// geometry. In other words disable in the following situations:
	//   * Geometry is a point.
	//
	// Note that upon insertion of a new vertex a polyline stays a polyline and
	// a polygon stays a polygon.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_INSERT_VERTEX,
			(geometry_type == GPlatesMaths::GeometryType::MULTIPOINT ||
			geometry_type == GPlatesMaths::GeometryType::POLYLINE ||
			geometry_type == GPlatesMaths::GeometryType::POLYGON) &&
			(num_vertices > 0));

	// Enable the delete vertex tool if deleting a vertex won't change the type of
	// geometry. In other words disable in the following situations:
	//   * Geometry is a point or multipoint with one vertex.
	//   * Geometry is a polyline with two vertices.
	//   * Geometry is a polygon with three vertices.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DELETE_VERTEX,
			(geometry_type == GPlatesMaths::GeometryType::MULTIPOINT && num_vertices > 1) ||
			(geometry_type == GPlatesMaths::GeometryType::POLYLINE && num_vertices > 2) ||
			(geometry_type == GPlatesMaths::GeometryType::POLYGON && num_vertices > 3));

	// Only enable splitting of a polyline.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_SPLIT_FEATURE,
			geometry_type == GPlatesMaths::GeometryType::POLYLINE && num_vertices > 1);
}


std::pair<unsigned int, GPlatesMaths::GeometryType::Value>
GPlatesGui::FeatureInspectionCanvasToolWorkflow::get_geometry_builder_parameters() const
{
	// See if the geometry builder has more than one vertex.
	if (d_focused_feature_geometry_builder.get_num_geometries() == 0)
	{
		return std::make_pair(0, GPlatesMaths::GeometryType::NONE);
	}

	// We currently only support a single internal geometry so set geom index to zero.
	const unsigned int num_vertices =
			d_focused_feature_geometry_builder.get_num_points_in_geometry(0/*geom_index*/);

	const GPlatesMaths::GeometryType::Value geometry_type =
			d_focused_feature_geometry_builder.get_geometry_build_type();

	return std::make_pair(num_vertices, geometry_type);
}
