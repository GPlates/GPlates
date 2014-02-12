/**
 * \file
 * $Revision: 14446 $
 * $Date: 2013-08-13 14:37:12 +0200 (Tue, 13 Aug 2013) $
 *
 * Copyright (C) 2014 Geological Survey of Norway
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

#include "canvas-tools/AdjustFittedPoleEstimate.h"
#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/SelectHellingerGeometries.h"
#include "presentation/ViewState.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ViewportWindow.h"
#include "view-operations/RenderedGeometryCollection.h"

#include "Dialogs.h"
#include "HellingerCanvasToolWorkflow.h"

namespace GPlatesGui
{
	namespace
	{
		/**
		 * The main rendered layer used by this canvas tool workflow.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType WORKFLOW_RENDER_LAYER =
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}

GPlatesGui::HellingerCanvasToolWorkflow::HellingerCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_canvas(),
			viewport_window.map_view(),
			CanvasToolWorkflows::WORKFLOW_HELLINGER,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_SELECT_HELLINGER_GEOMETRIES),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection())
{
	create_canvas_tools(
			canvas_tool_workflows,
			status_bar_callback,
			view_state,
			viewport_window);

}

void
GPlatesGui::HellingerCanvasToolWorkflow::create_canvas_tools(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
	//
	// Create select-hellinger-geometries canvas tool.
	//

	GPlatesCanvasTools::SelectHellingerGeometries::non_null_ptr_type select_hellinger_geometries_tool =
		GPlatesCanvasTools::SelectHellingerGeometries::create(
				status_bar_callback,
				view_state.get_rendered_geometry_collection(),
				WORKFLOW_RENDER_LAYER,
				//NOTE: this tool uses a stand-alone dialog rather than
				//a task-panel widget.
				viewport_window.dialogs().hellinger_dialog());
	// For the globe view.
	d_globe_select_hellinger_geometries_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					select_hellinger_geometries_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_select_hellinger_geometries_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					select_hellinger_geometries_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));


	//
	// Create adjust-pole-estimate canvas tool.
	//
	GPlatesCanvasTools::AdjustFittedPoleEstimate::non_null_ptr_type adjust_pole_estimate_tool =
			GPlatesCanvasTools::AdjustFittedPoleEstimate::create(
				status_bar_callback,
				view_state.get_rendered_geometry_collection(),
				WORKFLOW_RENDER_LAYER,
				//NOTE: this tool uses a stand-alone dialog rather than
				//a task-panel widget.
				viewport_window.dialogs().hellinger_dialog());
	// For the globe view.
	d_globe_adjust_pole_estimate_tool.reset(
				new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					adjust_pole_estimate_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_adjust_pole_estimate_tool.reset(
				new GPlatesCanvasTools::CanvasToolAdapterForMap(
					adjust_pole_estimate_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

}

void
GPlatesGui::HellingerCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.

	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_SELECT_HELLINGER_GEOMETRIES, true);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_ADJUST_FITTED_POLE_ESTIMATE, true);
}

void
GPlatesGui::HellingerCanvasToolWorkflow::activate_workflow()
{
	// Activate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, true/*active*/);
}

void
GPlatesGui::HellingerCanvasToolWorkflow::deactivate_workflow()
{
	// Deactivate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, false/*active*/);
}

boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
GPlatesGui::HellingerCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_SELECT_HELLINGER_GEOMETRIES:
		return std::make_pair(d_globe_select_hellinger_geometries_tool.get(), d_map_select_hellinger_geometries_tool.get());
	case CanvasToolWorkflows::TOOL_ADJUST_FITTED_POLE_ESTIMATE:
		return std::make_pair(d_globe_adjust_pole_estimate_tool.get(), d_map_adjust_pole_estimate_tool.get());

	default:
		break;
	}

	return boost::none;
}
