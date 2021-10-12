/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#include "MapCanvasToolChoice.h"

#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/MeasureDistance.h"
#include "canvas-tools/DigitiseGeometry.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/DeleteVertex.h"
#include "canvas-tools/InsertVertex.h"
#include "canvas-tools/MoveVertex.h"

#include "canvas-tools/MapManipulatePole.h"

#include "canvas-tools/PanMap.h"
#include "canvas-tools/ZoomMap.h"

#include "qt-widgets/DigitisationWidget.h"

#include "view-operations/RenderedGeometryCollection.h"

GPlatesGui::MapCanvasToolChoice::MapCanvasToolChoice(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		FeatureTableModel &clicked_table_model,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget,
		GPlatesGui::TopologySectionsContainer &topology_sections_container,
		GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		GPlatesGui::MapTransform &map_transform_):
	d_pan_map_tool_ptr(GPlatesCanvasTools::PanMap::create(
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_zoom_map_tool_ptr(GPlatesCanvasTools::ZoomMap::create(
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_click_geometry_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::ClickGeometry(
				rendered_geom_collection,
				view_state_,
				clicked_table_model,
				fp_dialog,
				feature_focus),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_digitise_polyline_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::DigitiseGeometry(
				GPlatesViewOperations::GeometryType::POLYLINE,
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYLINE,
				query_proximity_threshold),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_digitise_multipoint_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::DigitiseGeometry(
				GPlatesViewOperations::GeometryType::MULTIPOINT,
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_MULTIPOINT,
				query_proximity_threshold),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_digitise_polygon_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::DigitiseGeometry(
				GPlatesViewOperations::GeometryType::POLYGON,
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYGON,
				query_proximity_threshold),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_move_vertex_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::MoveVertex(
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				query_proximity_threshold),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_delete_vertex_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::DeleteVertex(
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				query_proximity_threshold),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_insert_vertex_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::InsertVertex(
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				query_proximity_threshold),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_manipulate_pole_tool_ptr(GPlatesCanvasTools::MapManipulatePole::create(
			rendered_geom_collection,
			map_canvas_,
			map_view_,
			view_state_,
			pole_widget,
			map_transform_)),
	d_measure_distance_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForMap::create(
			new GPlatesCanvasTools::MeasureDistance(
				rendered_geom_collection,
				measure_distance_state),
			map_canvas_,
			map_view_,
			view_state_,
			map_transform_)),
	d_tool_choice_ptr(d_pan_map_tool_ptr)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	tool_choice().handle_activation();
}


void
GPlatesGui::MapCanvasToolChoice::change_tool_if_necessary(
		MapCanvasTool::non_null_ptr_type new_tool_choice)
{
	if (new_tool_choice == d_tool_choice_ptr) {
		// The specified tool is already chosen.  Nothing to do here.
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	d_tool_choice_ptr->handle_deactivation();
	d_tool_choice_ptr = new_tool_choice;
	d_tool_choice_ptr->handle_activation();
}
