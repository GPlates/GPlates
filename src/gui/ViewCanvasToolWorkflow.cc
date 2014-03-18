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

#include "ViewCanvasToolWorkflow.h"

#include "Dialogs.h"
#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyUtils.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/ChangeLightDirectionGlobe.h"
#include "canvas-tools/ChangeLightDirectionMap.h"
#include "canvas-tools/PanMap.h"
#include "canvas-tools/ReorientGlobe.h"
#include "canvas-tools/ZoomGlobe.h"
#include "canvas-tools/ZoomMap.h"

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
				GPlatesViewOperations::RenderedGeometryCollection::VIEW_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}


GPlatesGui::ViewCanvasToolWorkflow::ViewCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_canvas(),
			viewport_window.map_view(),
			CanvasToolWorkflows::WORKFLOW_VIEW,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_DRAG_GLOBE),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection())
{
	create_canvas_tools(
			canvas_tool_workflows,
			status_bar_callback,
			view_state,
			viewport_window);
}


void
GPlatesGui::ViewCanvasToolWorkflow::create_canvas_tools(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
	//
	// Drag canvas tool.
	//

	d_globe_drag_globe_tool.reset(
			new GPlatesCanvasTools::ReorientGlobe(
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas(),
					view_state.get_rendered_geometry_collection(),
					viewport_window));
	d_map_drag_globe_tool.reset(
			new GPlatesCanvasTools::PanMap(
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_rendered_geometry_collection(),
					viewport_window,
					view_state.get_map_transform()));

	//
	// Zoom canvas tool.
	//

	d_globe_zoom_globe_tool.reset(
			new GPlatesCanvasTools::ZoomGlobe(
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas(),
					view_state.get_rendered_geometry_collection(),
					viewport_window,
					view_state));
	d_map_zoom_globe_tool.reset(
			new GPlatesCanvasTools::ZoomMap(
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_rendered_geometry_collection(),
					viewport_window,
					view_state.get_map_transform(),
					view_state.get_viewport_zoom()));

	//
	// Change lighting canvas tool.
	//

	d_globe_change_lighting_tool.reset(
			new GPlatesCanvasTools::ChangeLightDirectionGlobe(
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas(),
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					viewport_window,
					view_state));
	d_map_change_lighting_tool.reset(
			new GPlatesCanvasTools::ChangeLightDirectionMap(
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_rendered_geometry_collection(),
					viewport_window,
					view_state.get_map_transform()));
}


void
GPlatesGui::ViewCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.

	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_DRAG_GLOBE, true);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_ZOOM_GLOBE, true);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_CHANGE_LIGHTING, true);

	update_enable_state();
}


void
GPlatesGui::ViewCanvasToolWorkflow::activate_workflow()
{
	// Activate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, true/*active*/);
}


void
GPlatesGui::ViewCanvasToolWorkflow::deactivate_workflow()
{
	// Deactivate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, false/*active*/);
}


void
GPlatesGui::ViewCanvasToolWorkflow::update_enable_state()
{
}


boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
GPlatesGui::ViewCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_DRAG_GLOBE:
		return std::make_pair(d_globe_drag_globe_tool.get(), d_map_drag_globe_tool.get());

	case CanvasToolWorkflows::TOOL_ZOOM_GLOBE:
		return std::make_pair(d_globe_zoom_globe_tool.get(), d_map_zoom_globe_tool.get());

	case CanvasToolWorkflows::TOOL_CHANGE_LIGHTING:
		return std::make_pair(d_globe_change_lighting_tool.get(), d_map_change_lighting_tool.get());

	default:
		break;
	}

	return boost::none;
}
