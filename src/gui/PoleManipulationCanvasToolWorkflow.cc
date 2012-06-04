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

#include "PoleManipulationCanvasToolWorkflow.h"

#include "Dialogs.h"
#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyUtils.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/GeometryOperationState.h"
#include "canvas-tools/ManipulatePole.h"
#include "canvas-tools/MeasureDistance.h"
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

#include "view-operations/GeometryBuilder.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesGui
{
	namespace
	{
		/**
		 * The type of this canvas tool workflow.
		 */
		const CanvasToolWorkflows::WorkflowType WORKFLOW_TYPE = CanvasToolWorkflows::WORKFLOW_POLE_MANIPULATION;

		/**
		 * The main rendered layer used by this canvas tool workflow.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType WORKFLOW_RENDER_LAYER =
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}

GPlatesGui::PoleManipulationCanvasToolWorkflow::PoleManipulationCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(viewport_window.globe_canvas(), viewport_window.map_view()),
	d_feature_focus(view_state.get_feature_focus()),
	d_focused_feature_geometry_builder(view_state.get_focused_feature_geometry_builder()),
	d_geometry_operation_state(geometry_operation_state),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection()),
	d_clicked_table_model(view_state.get_feature_table_model()),
	d_reconstruct_graph(view_state.get_application_state().get_reconstruct_graph()),
	d_viewport_window(viewport_window),
	d_click_geometry_tool(NULL) // Will be non-null once 'create_canvas_tools()' is called.
{
	create_canvas_tools(
			canvas_tool_workflows,
			geometry_operation_state,
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
		SLOT(feature_focus_changed(
				GPlatesGui::FeatureFocus &)));
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::create_canvas_tools(
		CanvasToolWorkflows &canvas_tool_workflows,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
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
	// Click geometry canvas tool.
	//

	GPlatesCanvasTools::ClickGeometry::non_null_ptr_type click_geometry_tool =
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
	d_click_geometry_tool = click_geometry_tool.get();
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
	// Manipulate pole canvas tool.
	//

	GPlatesCanvasTools::ManipulatePole::non_null_ptr_type manipulate_pole_tool =
			GPlatesCanvasTools::ManipulatePole::create(
					status_bar_callback,
					view_state.get_rendered_geometry_collection(),
					viewport_window.task_panel_ptr()->modify_reconstruction_pole_widget());
	// For the globe view.
	d_globe_manipulate_pole_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					manipulate_pole_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_manipulate_pole_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					manipulate_pole_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.

	emit canvas_tool_enabled(
			WORKFLOW_TYPE,
			CanvasToolWorkflows::TOOL_DRAG_GLOBE,
			true);
	emit canvas_tool_enabled(
			WORKFLOW_TYPE,
			CanvasToolWorkflows::TOOL_ZOOM_GLOBE,
			true);
	emit canvas_tool_enabled(
			WORKFLOW_TYPE,
			CanvasToolWorkflows::TOOL_CLICK_GEOMETRY,
			true);

	update_enable_state();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::activate_workflow()
{
	// Let others know the currently activated GeometryBuilder.
	// This currently enables the Measure Distance tool to target the focused feature.
	d_geometry_operation_state.set_active_geometry_builder(&d_focused_feature_geometry_builder);

	// Activate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, true/*active*/);

	// Restore the focused feature (saved when workflow was deactivated).
	if (d_save_restore_focused_feature.is_valid())
	{
		if (d_save_restore_focused_feature_geometry_property.is_still_valid())
		{
			d_feature_focus.set_focus(
					d_save_restore_focused_feature,
					d_save_restore_focused_feature_geometry_property);
		}
		else // Focused feature but geometry property no longer valid...
		{
			// Set focus to first geometry found within the feature.
			d_feature_focus.set_focus(d_save_restore_focused_feature);
		}
	}
	else // No focused feature...
	{
		d_feature_focus.unset_focus();
	}
	// Restore the sequence of clicked geometries for this workflow (saved when workflow was deactivated).
	// NOTE: We do this *after* focusing the feature so that it can be found in the updated clicked feature table.
	GPlatesGui::add_clicked_geometries_to_feature_table(
			d_save_restore_clicked_geom_seq,
			d_viewport_window,
			d_clicked_table_model,
			d_feature_focus,
			d_reconstruct_graph,
			false/*highlight_first_clicked_feature_in_table*/);
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::deactivate_workflow()
{
	// Let others know there's no currently activated GeometryBuilder.
	d_geometry_operation_state.set_no_active_geometry_builder();

	// Deactivate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, false/*active*/);

	// Save the sequence of clicked geometries for this workflow so we can restore on workflow re-activation.
	d_save_restore_clicked_geom_seq = d_click_geometry_tool->get_clicked_geom_seq();
	// Save the focused feature for this workflow so we can restore on workflow re-activation.
	d_save_restore_focused_feature = d_feature_focus.focused_feature();
	d_save_restore_focused_feature_geometry_property = d_feature_focus.associated_geometry_property();

	// Clear the clicked geometries table and unset the focused feature.
	// If the canvas tool workflow, that's about to be activated, uses the feature focus then it'll
	// populate the table with its own selections (from when the user clicked geometries in that workflow).
	d_clicked_table_model.clear();
	d_feature_focus.unset_focus();
}


boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
GPlatesGui::PoleManipulationCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_DRAG_GLOBE:
		return std::make_pair(d_globe_drag_globe_tool.get(), d_map_drag_globe_tool.get());

	case CanvasToolWorkflows::TOOL_ZOOM_GLOBE:
		return std::make_pair(d_globe_zoom_globe_tool.get(), d_map_zoom_globe_tool.get());

	case CanvasToolWorkflows::TOOL_CLICK_GEOMETRY:
		return std::make_pair(d_globe_click_geometry_tool.get(), d_map_click_geometry_tool.get());

	case CanvasToolWorkflows::TOOL_MANIPULATE_POLE:
		return std::make_pair(d_globe_manipulate_pole_tool.get(), d_map_manipulate_pole_tool.get());

	default:
		break;
	}

	return boost::none;
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::feature_focus_changed(
		GPlatesGui::FeatureFocus &feature_focus)
{
	update_enable_state();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::update_enable_state()
{
	update_manipulate_pole_tool();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::update_manipulate_pole_tool()
{
	const GPlatesModel::FeatureHandle::weak_ref focused_feature = d_feature_focus.focused_feature();

	bool enable_manipulate_pole_tool = false;

	// Enable pole manipulation if there's a focused feature that is not topological.
	if (focused_feature.is_valid())
	{
		if (!GPlatesAppLogic::TopologyUtils::is_topological_closed_plate_boundary_feature(*focused_feature.handle_ptr()) &&
			!GPlatesAppLogic::TopologyUtils::is_topological_network_feature(*focused_feature.handle_ptr()))
		{
			enable_manipulate_pole_tool = true;
		}
	}

	emit canvas_tool_enabled(
			WORKFLOW_TYPE,
			CanvasToolWorkflows::TOOL_MANIPULATE_POLE,
			enable_manipulate_pole_tool);
}
