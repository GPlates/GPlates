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
#include "app-logic/Reconstruction.h"
#include "app-logic/TopologyUtils.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/ManipulatePole.h"
#include "canvas-tools/MovePoleGlobe.h"
#include "canvas-tools/MovePoleMap.h"

#include "global/GPlatesAssert.h"

#include "gui/GeometryFocusHighlight.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/TaskPanel.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/MovePoleOperation.h"
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
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}

GPlatesGui::PoleManipulationCanvasToolWorkflow::PoleManipulationCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_canvas(),
			viewport_window.map_view(),
			CanvasToolWorkflows::WORKFLOW_POLE_MANIPULATION,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_MANIPULATE_POLE),
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
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::create_canvas_tools(
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
	// Manipulate pole canvas tool.
	//

	GPlatesCanvasTools::CanvasTool::non_null_ptr_type manipulate_pole_tool =
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

	//
	// Move pole canvas tool.
	//

	const GPlatesViewOperations::MovePoleOperation::non_null_ptr_type move_pole_operation =
			GPlatesViewOperations::MovePoleOperation::create(
					view_state.get_viewport_zoom(),
					view_state.get_rendered_geometry_collection(),
					WORKFLOW_RENDER_LAYER,
					viewport_window.task_panel_ptr()->move_pole_widget());

	// For the globe view.
	d_globe_move_pole_tool.reset(
			new GPlatesCanvasTools::MovePoleGlobe(
					move_pole_operation,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas(),
					viewport_window));
	// For the map view.
	d_map_move_pole_tool.reset(
			new GPlatesCanvasTools::MovePoleMap(
					move_pole_operation,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					viewport_window,
					view_state));
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
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_CLICK_GEOMETRY, true);

	update_enable_state();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::activate_workflow()
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
GPlatesGui::PoleManipulationCanvasToolWorkflow::deactivate_workflow()
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
GPlatesGui::PoleManipulationCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_CLICK_GEOMETRY:
		return std::make_pair(d_globe_click_geometry_tool.get(), d_map_click_geometry_tool.get());

	case CanvasToolWorkflows::TOOL_MANIPULATE_POLE:
		return std::make_pair(d_globe_manipulate_pole_tool.get(), d_map_manipulate_pole_tool.get());

	case CanvasToolWorkflows::TOOL_MOVE_POLE:
		return std::make_pair(d_globe_move_pole_tool.get(), d_map_move_pole_tool.get());

	default:
		break;
	}

	return boost::none;
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::draw_feature_focus()
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
GPlatesGui::PoleManipulationCanvasToolWorkflow::update_enable_state()
{
	update_manipulate_pole_tool();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::update_manipulate_pole_tool()
{
	const GPlatesModel::FeatureHandle::const_weak_ref focused_feature = d_feature_focus.focused_feature();

	bool enable_manipulate_pole_tool = false;

	// Enable pole manipulation if there's a focused feature that is not topological.
	if (focused_feature.is_valid())
	{
		if (!GPlatesAppLogic::TopologyUtils::is_topological_feature(focused_feature))
		{
			enable_manipulate_pole_tool = true;
		}
	}

	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MANIPULATE_POLE, enable_manipulate_pole_tool);
	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MOVE_POLE, true);
}
