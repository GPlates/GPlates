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

#include "SmallCircleCanvasToolWorkflow.h"

#include "Dialogs.h"

#include "app-logic/ApplicationState.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/CreateSmallCircle.h"
#include "canvas-tools/GeometryOperationState.h"
#include "canvas-tools/MeasureDistance.h"

#include "global/GPlatesAssert.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/TaskPanel.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesGui
{
	namespace
	{
		/**
		 * The main rendered layer used by this canvas tool workflow.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType WORKFLOW_RENDER_LAYER =
				GPlatesViewOperations::RenderedGeometryCollection::SMALL_CIRCLE_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}

GPlatesGui::SmallCircleCanvasToolWorkflow::SmallCircleCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_canvas(),
			viewport_window.map_view(),
			CanvasToolWorkflows::WORKFLOW_SMALL_CIRCLE,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_CREATE_SMALL_CIRCLE),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection())
{
	create_canvas_tools(
			canvas_tool_workflows,
			geometry_operation_state,
			measure_distance_state,
			status_bar_callback,
			view_state,
			viewport_window);
}


void
GPlatesGui::SmallCircleCanvasToolWorkflow::create_canvas_tools(
		CanvasToolWorkflows &canvas_tool_workflows,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
	//
	// Create small circle canvas tool.
	//

	GPlatesCanvasTools::CreateSmallCircle::non_null_ptr_type create_small_circle_tool =
		GPlatesCanvasTools::CreateSmallCircle::create(
				status_bar_callback,
				view_state.get_rendered_geometry_collection(),
				WORKFLOW_RENDER_LAYER,
				viewport_window.task_panel_ptr()->small_circle_widget());
	// For the globe view.
	d_globe_create_small_circle_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					create_small_circle_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_create_small_circle_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					create_small_circle_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));
}


void
GPlatesGui::SmallCircleCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.

	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_CREATE_SMALL_CIRCLE, true);
}


void
GPlatesGui::SmallCircleCanvasToolWorkflow::activate_workflow()
{
	// Activate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, true/*active*/);
}


void
GPlatesGui::SmallCircleCanvasToolWorkflow::deactivate_workflow()
{
	// Deactivate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, false/*active*/);
}


boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
GPlatesGui::SmallCircleCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_CREATE_SMALL_CIRCLE:
		return std::make_pair(d_globe_create_small_circle_tool.get(), d_map_create_small_circle_tool.get());

	default:
		break;
	}

	return boost::none;
}
