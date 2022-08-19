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

#include "DigitisationCanvasToolWorkflow.h"

#include "Dialogs.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyUtils.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/DeleteVertex.h"
#include "canvas-tools/DigitiseGeometry.h"
#include "canvas-tools/GeometryOperationState.h"
#include "canvas-tools/InsertVertex.h"
#include "canvas-tools/MeasureDistance.h"
#include "canvas-tools/MoveVertex.h"

#include "global/GPlatesAssert.h"

#include "maths/GeometryType.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapCanvas.h"
#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/GeometryBuilder.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesGui
{
	namespace
	{
		/**
		 * The main rendered layer used by this canvas tool workflow.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType WORKFLOW_RENDER_LAYER =
				GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}

GPlatesGui::DigitisationCanvasToolWorkflow::DigitisationCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_and_map_canvas(),
			CanvasToolWorkflows::WORKFLOW_DIGITISATION,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYLINE),
	d_digitise_geometry_builder(view_state.get_digitise_geometry_builder()),
	d_geometry_operation_state(geometry_operation_state),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection())
{
	create_canvas_tools(
			canvas_tool_workflows,
			geometry_operation_state,
			modify_geometry_state,
			measure_distance_state,
			status_bar_callback,
			view_state,
			viewport_window);

	// Listen for digitised geometry changes.
	QObject::connect(
			&d_digitise_geometry_builder,
			SIGNAL(stopped_updating_geometry_excluding_intermediate_moves()),
			this,
			SLOT(geometry_builder_stopped_updating_geometry_excluding_intermediate_moves()));
}


void
GPlatesGui::DigitisationCanvasToolWorkflow::create_canvas_tools(
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
	// NOTE: There's also a Measure Distance tool in the Feature Inspection workflow, but we
	// also have one in the Digitisation workflow because it is hooked up to the digitised geometry.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type measure_distance_tool =
		GPlatesCanvasTools::MeasureDistance::create(
				status_bar_callback,
				view_state.get_digitise_geometry_builder(),
				geometry_operation_state,
				view_state.get_rendered_geometry_collection(),
				WORKFLOW_RENDER_LAYER,
				measure_distance_state);
	// For the globe view.
	d_globe_measure_distance_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					measure_distance_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_globe_view_operation()));
	// For the map view.
	d_map_measure_distance_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					measure_distance_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_map_view_operation()));

	//
	// Digitise multipoint canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type digitise_multipoint_tool =
			GPlatesCanvasTools::DigitiseGeometry::create(
					status_bar_callback,
					GPlatesMaths::GeometryType::MULTIPOINT,
					view_state.get_digitise_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_digitise_multipoint_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					digitise_multipoint_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_globe_view_operation()));
	// For the map view.
	d_map_digitise_multipoint_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					digitise_multipoint_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_map_view_operation()));

	//
	// Digitise polyline canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type digitise_polyline_tool =
			GPlatesCanvasTools::DigitiseGeometry::create(
					status_bar_callback,
					GPlatesMaths::GeometryType::POLYLINE,
					view_state.get_digitise_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_digitise_polyline_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					digitise_polyline_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_globe_view_operation()));
	// For the map view.
	d_map_digitise_polyline_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					digitise_polyline_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_map_view_operation()));

	//
	// Digitise polygon canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type digitise_polygon_tool =
			GPlatesCanvasTools::DigitiseGeometry::create(
					status_bar_callback,
					GPlatesMaths::GeometryType::POLYGON,
					view_state.get_digitise_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_digitise_polygon_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					digitise_polygon_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_globe_view_operation()));
	// For the map view.
	d_map_digitise_polygon_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					digitise_polygon_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_map_view_operation()));

	//
	// Move vertex canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type move_vertex_tool =
			GPlatesCanvasTools::MoveVertex::create(
					status_bar_callback,
					view_state.get_digitise_geometry_builder(),
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
					viewport_window.globe_and_map_canvas(),
					view_state.get_globe_view_operation()));
	// For the map view.
	d_map_move_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					move_vertex_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_map_view_operation()));

	//
	// Delete vertex canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type delete_vertex_tool =
			GPlatesCanvasTools::DeleteVertex::create(
					status_bar_callback,
					view_state.get_digitise_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_delete_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					delete_vertex_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_globe_view_operation()));
	// For the map view.
	d_map_delete_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					delete_vertex_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_map_view_operation()));

	//
	// Insert vertex canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type insert_vertex_tool =
			GPlatesCanvasTools::InsertVertex::create(
					status_bar_callback,
					view_state.get_digitise_geometry_builder(),
					geometry_operation_state,
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					canvas_tool_workflows,
					viewport_window.reconstruction_view_widget().globe_and_map_widget());
	// For the globe view.
	d_globe_insert_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					insert_vertex_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_globe_view_operation()));
	// For the map view.
	d_map_insert_vertex_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					insert_vertex_tool,
					viewport_window.globe_and_map_canvas(),
					view_state.get_map_view_operation()));
}


void
GPlatesGui::DigitisationCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.

	// The measure distance tool can do measurements without a digitised geometry so we leave it enabled always.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MEASURE_DISTANCE, true);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DIGITISE_NEW_MULTIPOINT, true);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYLINE, true);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYGON, true);

	update_enable_state();
}


void
GPlatesGui::DigitisationCanvasToolWorkflow::activate_workflow()
{
	// Let others know the currently activated GeometryBuilder.
	d_geometry_operation_state.set_active_geometry_builder(&d_digitise_geometry_builder);

	// Activate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, true/*active*/);
}


void
GPlatesGui::DigitisationCanvasToolWorkflow::deactivate_workflow()
{
	// Let others know there's no currently activated GeometryBuilder.
	d_geometry_operation_state.set_no_active_geometry_builder();

	// Deactivate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, false/*active*/);
}


boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
GPlatesGui::DigitisationCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_MEASURE_DISTANCE:
		return std::make_pair(d_globe_measure_distance_tool.get(), d_map_measure_distance_tool.get());

	case CanvasToolWorkflows::TOOL_DIGITISE_NEW_MULTIPOINT:
		return std::make_pair(d_globe_digitise_multipoint_tool.get(), d_map_digitise_multipoint_tool.get());

	case CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYLINE:
		return std::make_pair(d_globe_digitise_polyline_tool.get(), d_map_digitise_polyline_tool.get());

	case CanvasToolWorkflows::TOOL_DIGITISE_NEW_POLYGON:
		return std::make_pair(d_globe_digitise_polygon_tool.get(), d_map_digitise_polygon_tool.get());

	case CanvasToolWorkflows::TOOL_MOVE_VERTEX:
		return std::make_pair(d_globe_move_vertex_tool.get(), d_map_move_vertex_tool.get());

	case CanvasToolWorkflows::TOOL_DELETE_VERTEX:
		return std::make_pair(d_globe_delete_vertex_tool.get(), d_map_delete_vertex_tool.get());

	case CanvasToolWorkflows::TOOL_INSERT_VERTEX:
		return std::make_pair(d_globe_insert_vertex_tool.get(), d_map_insert_vertex_tool.get());

	default:
		break;
	}

	return boost::none;
}


void
GPlatesGui::DigitisationCanvasToolWorkflow::geometry_builder_stopped_updating_geometry_excluding_intermediate_moves()
{
	// We use this to determine if a geometry, that's being operated on or will potentially
	// be operated on, has got vertices or not.

	update_enable_state();
}


void
GPlatesGui::DigitisationCanvasToolWorkflow::update_enable_state()
{
	const std::pair<unsigned int, GPlatesMaths::GeometryType::Value> geometry_builder_parameters =
			get_geometry_builder_parameters();
	const unsigned int num_vertices = geometry_builder_parameters.first;
	const GPlatesMaths::GeometryType::Value geometry_type = geometry_builder_parameters.second;

	// Enable the move vertex tool if there's at least one vertex regardless of the geometry type.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MOVE_VERTEX, num_vertices > 0);

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
}


std::pair<unsigned int, GPlatesMaths::GeometryType::Value>
GPlatesGui::DigitisationCanvasToolWorkflow::get_geometry_builder_parameters() const
{
	// See if the geometry builder has more than one vertex.
	if (d_digitise_geometry_builder.get_num_geometries() == 0)
	{
		return std::make_pair(0, GPlatesMaths::GeometryType::NONE);
	}

	// We currently only support a single internal geometry so set geom index to zero.
	const unsigned int num_vertices =
			d_digitise_geometry_builder.get_num_points_in_geometry(0/*geom_index*/);

	const GPlatesMaths::GeometryType::Value geometry_type =
			d_digitise_geometry_builder.get_geometry_build_type();

	return std::make_pair(num_vertices, geometry_type);
}
