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

#include "TopologyCanvasToolWorkflow.h"

#include "Dialogs.h"
#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/TopologyInternalUtils.h"
#include "app-logic/TopologyUtils.h"

#include "canvas-tools/BuildTopology.h"
#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/EditTopology.h"

#include "global/GPlatesAssert.h"

#include "gui/CanvasToolWorkflows.h"
#include "gui/GeometryFocusHighlight.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/TaskPanel.h"
#include "qt-widgets/ViewportWindow.h"

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
				GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER;


		/**
		 * Returns true if the specified workflow/tool is the currently active tool (and also is enabled).
		 */
		bool
		is_active_and_enabled_tool(
				const CanvasToolWorkflows &canvas_tool_workflows,
				CanvasToolWorkflows::WorkflowType workflow,
				CanvasToolWorkflows::ToolType tool)
		{
			return canvas_tool_workflows.get_active_canvas_tool() == std::make_pair(workflow, tool) &&
				canvas_tool_workflows.is_canvas_tool_enabled(workflow, tool);
		}
	}
}

GPlatesGui::TopologyCanvasToolWorkflow::TopologyCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_canvas(),
			viewport_window.map_view(),
			CanvasToolWorkflows::WORKFLOW_TOPOLOGY,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_BUILD_BOUNDARY_TOPOLOGY),
	d_canvas_tool_workflows(canvas_tool_workflows),
	d_feature_focus(view_state.get_feature_focus()),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection()),
	d_rendered_geometry_parameters(view_state.get_rendered_geometry_parameters()),
	d_render_settings(view_state.get_render_settings()),
	d_symbol_map(view_state.get_feature_type_symbol_map()),
	d_application_state(view_state.get_application_state())
{
	create_canvas_tools(
			canvas_tool_workflows,
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

	// Listen for changes to the canvas tool selection.
	QObject::connect(
			&canvas_tool_workflows,
			SIGNAL(canvas_tool_activated(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)),
			this,
			SLOT(handle_canvas_tool_activated(
					GPlatesGui::CanvasToolWorkflows::WorkflowType,
					GPlatesGui::CanvasToolWorkflows::ToolType)));
}


void
GPlatesGui::TopologyCanvasToolWorkflow::create_canvas_tools(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
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
	// Build line topology canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type build_line_topology_tool =
			GPlatesCanvasTools::BuildTopology::create(
					GPlatesAppLogic::TopologyGeometry::LINE,
					status_bar_callback,
					view_state,
					viewport_window,
					view_state.get_feature_table_model(),
					viewport_window.task_panel_ptr()->topology_tools_widget(),
					view_state.get_application_state());
	// For the globe view.
	d_globe_build_line_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					build_line_topology_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_build_line_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					build_line_topology_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Build boundary topology canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type build_boundary_topology_tool =
			GPlatesCanvasTools::BuildTopology::create(
					GPlatesAppLogic::TopologyGeometry::BOUNDARY,
					status_bar_callback,
					view_state,
					viewport_window,
					view_state.get_feature_table_model(),
					viewport_window.task_panel_ptr()->topology_tools_widget(),
					view_state.get_application_state());
	// For the globe view.
	d_globe_build_boundary_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					build_boundary_topology_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_build_boundary_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					build_boundary_topology_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Build network topology canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type build_network_topology_tool =
			GPlatesCanvasTools::BuildTopology::create(
					GPlatesAppLogic::TopologyGeometry::NETWORK,
					status_bar_callback,
					view_state,
					viewport_window,
					view_state.get_feature_table_model(),
					viewport_window.task_panel_ptr()->topology_tools_widget(),
					view_state.get_application_state());
	// For the globe view.
	d_globe_build_network_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					build_network_topology_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_build_network_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					build_network_topology_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));

	//
	// Edit topology canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type edit_topology_tool =
			GPlatesCanvasTools::EditTopology::create(
					status_bar_callback,
					view_state,
					viewport_window,
					view_state.get_feature_table_model(),
					viewport_window.task_panel_ptr()->topology_tools_widget(),
					view_state.get_application_state());
	// For the globe view.
	d_globe_edit_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					edit_topology_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_edit_topology_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					edit_topology_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));
}


void
GPlatesGui::TopologyCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_CLICK_GEOMETRY, true);

	update_enable_state();
}


void
GPlatesGui::TopologyCanvasToolWorkflow::activate_workflow()
{
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
GPlatesGui::TopologyCanvasToolWorkflow::deactivate_workflow()
{
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
GPlatesGui::TopologyCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_CLICK_GEOMETRY:
		return std::make_pair(d_globe_click_geometry_tool.get(), d_map_click_geometry_tool.get());

	case CanvasToolWorkflows::TOOL_BUILD_LINE_TOPOLOGY:
		return std::make_pair(d_globe_build_line_topology_tool.get(), d_map_build_line_topology_tool.get());

	case CanvasToolWorkflows::TOOL_BUILD_BOUNDARY_TOPOLOGY:
		return std::make_pair(d_globe_build_boundary_topology_tool.get(), d_map_build_boundary_topology_tool.get());

	case CanvasToolWorkflows::TOOL_BUILD_NETWORK_TOPOLOGY:
		return std::make_pair(d_globe_build_network_topology_tool.get(), d_map_build_network_topology_tool.get());

	case CanvasToolWorkflows::TOOL_EDIT_TOPOLOGY:
		return std::make_pair(d_globe_edit_topology_tool.get(), d_map_edit_topology_tool.get());

	default:
		break;
	}

	return boost::none;
}


void
GPlatesGui::TopologyCanvasToolWorkflow::handle_canvas_tool_activated(
		GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
		GPlatesGui::CanvasToolWorkflows::ToolType tool)
{
	update_enable_state();
}


void
GPlatesGui::TopologyCanvasToolWorkflow::draw_feature_focus()
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
GPlatesGui::TopologyCanvasToolWorkflow::update_enable_state()
{
	update_build_topology_tools();
	update_edit_topology_tool();
}


void
GPlatesGui::TopologyCanvasToolWorkflow::update_build_topology_tools()
{
	// The build topology tools are always enabled.
	// If feature is focused when a build tool is activated then the build tool will temporarily
	// unfocus the feature while it is active (and return original focus when deactivated).
	// This includes when the edit topology tool is active (in which case the edit topology tool
	// will re-focus the topology feature, that it was editing, on deactivation and the newly activated
	// build tool will then save that focus temporarily, unfocus it and then re-focus on deactivation).
	bool enable_build_topology_tools = true;

	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_BUILD_LINE_TOPOLOGY, enable_build_topology_tools);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_BUILD_BOUNDARY_TOPOLOGY, enable_build_topology_tools);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_BUILD_NETWORK_TOPOLOGY, enable_build_topology_tools);
}


void
GPlatesGui::TopologyCanvasToolWorkflow::update_edit_topology_tool()
{
	bool enable_edit_topology_tool = false;

	// The edit topology tool is enabled whenever it is the currently active (and enabled) tool
	// regardless of whether a feature is focused or not - this is because the feature focus is used
	// to add topology sections so it's always focusing, unfocusing, etc while the tool is being used.
	if (is_active_and_enabled_tool(d_canvas_tool_workflows, get_workflow(), CanvasToolWorkflows::TOOL_EDIT_TOPOLOGY))
	{
		enable_edit_topology_tool = true;
	}
	// If the edit topology tool is not the current tool then it is only enabled if:
	// (1) no build topology tools are the current tool, *and*
	// (2) a feature is focused and that feature is a topological feature.
	// The reason for (1) is the build boundary or network tool could be currently adding a line topology
	// as a boundary section and this would have enabled the edit topology tool.
	else if (!is_active_and_enabled_tool(d_canvas_tool_workflows, get_workflow(), CanvasToolWorkflows::TOOL_BUILD_LINE_TOPOLOGY) &&
			!is_active_and_enabled_tool(d_canvas_tool_workflows, get_workflow(), CanvasToolWorkflows::TOOL_BUILD_BOUNDARY_TOPOLOGY) &&
			!is_active_and_enabled_tool(d_canvas_tool_workflows, get_workflow(), CanvasToolWorkflows::TOOL_BUILD_NETWORK_TOPOLOGY))
	{
		if (d_feature_focus.associated_reconstruction_geometry())
		{
			const GPlatesModel::FeatureHandle::const_weak_ref focused_feature = d_feature_focus.focused_feature();

			if (GPlatesAppLogic::TopologyUtils::is_topological_feature(focused_feature))
			{
				enable_edit_topology_tool = true;
			}
		}
	}

	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_EDIT_TOPOLOGY, enable_edit_topology_tool);
}
