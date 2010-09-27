/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "GlobeCanvasToolChoice.h"

#include "canvas-tools/CanvasToolType.h"
#include "canvas-tools/ReorientGlobe.h"
#include "canvas-tools/ZoomGlobe.h"

#include "canvas-tools/BuildTopology.h"   // Not Globe/Mapified yet!
#include "canvas-tools/EditTopology.h"    // Not Globe/Mapified yet!

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/MeasureDistance.h"
#include "canvas-tools/DigitiseGeometry.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/DeleteVertex.h"
#include "canvas-tools/InsertVertex.h"
#include "canvas-tools/MoveVertex.h"
#include "canvas-tools/SplitFeature.h"

#include "canvas-tools/GlobeManipulatePole.h"

#include "qt-widgets/DigitisationWidget.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::GlobeCanvasToolChoice::GlobeCanvasToolChoice(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		Globe &globe,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		const GPlatesQtWidgets::ViewportWindow &viewport_window,
		GPlatesPresentation::ViewState &view_state,
		FeatureTableModel &clicked_table_model,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget,
		GPlatesGui::TopologySectionsContainer &topology_sections_container,
		GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state):
	d_reorient_globe_tool_ptr(GPlatesCanvasTools::ReorientGlobe::create(
			globe,
			globe_canvas,
			viewport_window)),
	d_zoom_globe_tool_ptr(GPlatesCanvasTools::ZoomGlobe::create(
			globe,
			globe_canvas,
			viewport_window,
			view_state)),
	d_click_geometry_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::ClickGeometry(
				rendered_geom_collection,
				viewport_window,
				clicked_table_model,
				fp_dialog,
				feature_focus,
				view_state.get_application_state()),
			globe,
			globe_canvas,
			viewport_window)),
	d_digitise_polyline_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::DigitiseGeometry(
				GPlatesViewOperations::GeometryType::POLYLINE,
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYLINE,
				query_proximity_threshold),
			globe,
			globe_canvas,
			viewport_window)),
	d_digitise_multipoint_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::DigitiseGeometry(
				GPlatesViewOperations::GeometryType::MULTIPOINT,
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_MULTIPOINT,
				query_proximity_threshold),
			globe,
			globe_canvas,
			viewport_window)),
	d_digitise_polygon_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::DigitiseGeometry(
				GPlatesViewOperations::GeometryType::POLYGON,
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYGON,
				query_proximity_threshold),
			globe,
			globe_canvas,
			viewport_window)),
	d_move_vertex_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::MoveVertex(
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				query_proximity_threshold,
				&viewport_window),
			globe,
			globe_canvas,
			viewport_window)),
	d_delete_vertex_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::DeleteVertex(
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				query_proximity_threshold),
			globe,
			globe_canvas,
			viewport_window)),
	d_insert_vertex_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::InsertVertex(
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				query_proximity_threshold),
			globe,
			globe_canvas,
			viewport_window)),
	d_split_feature_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::SplitFeature(
				feature_focus,
				view_state,
				geometry_operation_target,
				active_geometry_operation,
				rendered_geom_collection,
				choose_canvas_tool,
				query_proximity_threshold),
			globe,
			globe_canvas,
			viewport_window)),
	d_manipulate_pole_tool_ptr(GPlatesCanvasTools::GlobeManipulatePole::create(
			rendered_geom_collection,
			globe,
			globe_canvas,
			viewport_window,
			pole_widget)),
	d_build_topology_tool_ptr(GPlatesCanvasTools::BuildTopology::create(
			globe, 
			globe_canvas, 
			view_state,
			viewport_window, 
			clicked_table_model, 
			topology_sections_container,
			topology_tools_widget,
			view_state.get_application_state())),
	d_edit_topology_tool_ptr(GPlatesCanvasTools::EditTopology::create(
			globe, 
			globe_canvas, 
			view_state,
			viewport_window, 
			clicked_table_model, 
			topology_sections_container,
			topology_tools_widget,
			view_state.get_application_state())),
	d_measure_distance_tool_ptr(GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
			new GPlatesCanvasTools::MeasureDistance(
				rendered_geom_collection,
				measure_distance_state),
			globe,
			globe_canvas,
			viewport_window)),
	d_tool_choice_ptr(d_reorient_globe_tool_ptr),
	d_tool_choice_as_enum(CanvasToolChoice::REORIENT_GLOBE_OR_PAN_MAP)
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
GPlatesGui::GlobeCanvasToolChoice::set_tool_choice(
		CanvasToolChoice::Type canvas_tool)
{
	using namespace CanvasToolChoice;

	switch (canvas_tool)
	{
		case REORIENT_GLOBE_OR_PAN_MAP:
			choose_reorient_globe_tool();
			break;

		case ZOOM:
			choose_zoom_globe_tool();
			break;

		case CLICK_GEOMETRY:
			choose_click_geometry_tool();
			break;

		case DIGITISE_POLYLINE:
			choose_digitise_polyline_tool();
			break;

		case DIGITISE_MULTIPOINT:
			choose_digitise_multipoint_tool();
			break;

		case DIGITISE_POLYGON:
			choose_digitise_polygon_tool();
			break;

		case MOVE_VERTEX:
			choose_move_vertex_tool();
			break;

		case DELETE_VERTEX:
			choose_delete_vertex_tool();
			break;

		case INSERT_VERTEX:
			choose_insert_vertex_tool();
			break;

		case SPLIT_FEATURE:
		 	choose_split_feature_tool();
		 	break;

		case MANIPULATE_POLE:
			choose_manipulate_pole_tool();
			break;

		case MEASURE_DISTANCE:
			choose_measure_distance_tool();
			break;

		case BUILD_TOPOLOGY:
			choose_build_topology_tool();
			break;

		case EDIT_TOPOLOGY:
		 	choose_edit_topology_tool();
		 	break;
	}
}


void
GPlatesGui::GlobeCanvasToolChoice::change_tool_if_necessary(
		GlobeCanvasTool::non_null_ptr_type new_tool_choice)
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


void
GPlatesGui::GlobeCanvasToolChoice::set_enum_and_emit_signal(
		CanvasToolChoice::Type canvas_tool)
{
	d_tool_choice_as_enum = canvas_tool;
	emit tool_choice_changed(canvas_tool);
}

