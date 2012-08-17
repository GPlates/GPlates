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

#include "CanvasToolWorkflow.h"

#include "GlobeCanvasTool.h"
#include "MapCanvasTool.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesGui::CanvasToolWorkflow::CanvasToolWorkflow(
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		GPlatesQtWidgets::MapView &map_view) :
	d_globe_canvas_tool_adapter(globe_canvas),
	d_map_canvas_tool_adapter(map_view),
	d_selected_tool(CanvasToolWorkflows::TOOL_DRAG_GLOBE),
	d_is_workflow_active(false)
{
}


void
GPlatesGui::CanvasToolWorkflow::activate(
		boost::optional<CanvasToolWorkflows::ToolType> select_tool_opt)
{
	// The newly selected tool.
	const GPlatesGui::CanvasToolWorkflows::ToolType select_tool =
			select_tool_opt ? select_tool_opt.get() : d_selected_tool;

	// If the selected tool is the same and its already active then there's nothing to do.
	if ((select_tool == d_selected_tool) && d_is_workflow_active)
	{
		return;
	}

	// Deactivate the previously selected tool first if workflow is active, otherwise activate workflow.
	if (d_is_workflow_active)
	{
		deactivate_selected_tool();
		d_selected_tool = select_tool;
		activate_selected_tool();
	}
	else // workflow not active...
	{
		// Get derived class to activate itself.
		activate_workflow();
		d_selected_tool = select_tool;
		activate_selected_tool();

		d_is_workflow_active = true;
	}
}


void
GPlatesGui::CanvasToolWorkflow::deactivate()
{
	if (d_is_workflow_active)
	{
		// Deactivate the currently selected tool if the workflow is active.
		deactivate_selected_tool();

		// Get derived class to deactivate itself.
		deactivate_workflow();

		d_is_workflow_active = false;
	}
}


void
GPlatesGui::CanvasToolWorkflow::activate_selected_tool()
{
	// Notify derived class about to activate the currently selected canvas tool.
	activating_selected_tool();

	// Ask derived class for the selected globe and map canvas tools.
	boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
			globe_map_canvas_tool =
					get_selected_globe_and_map_canvas_tools(d_selected_tool);
	// The derived class (workflow) maybe not have the selected tool in which case it's an assertion failure.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			globe_map_canvas_tool,
			GPLATES_ASSERTION_SOURCE);

	// Activate the selected globe canvas tool.
	globe_map_canvas_tool->first->handle_activation();
	d_globe_canvas_tool_adapter.activate_canvas_tool(*globe_map_canvas_tool->first);

	// Activate the selected map canvas tool.
	globe_map_canvas_tool->second->handle_activation();
	d_map_canvas_tool_adapter.activate_canvas_tool(*globe_map_canvas_tool->second);
}


void
GPlatesGui::CanvasToolWorkflow::deactivate_selected_tool()
{
	// Ask derived class for the selected globe and map canvas tools.
	boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> > globe_map_canvas_tool =
			get_selected_globe_and_map_canvas_tools(d_selected_tool);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			globe_map_canvas_tool,
			GPLATES_ASSERTION_SOURCE);

	// Deactivate the selected globe canvas tool.
	globe_map_canvas_tool->first->handle_deactivation();
	d_globe_canvas_tool_adapter.deactivate_canvas_tool();

	// Deactivate the selected map canvas tool.
	globe_map_canvas_tool->second->handle_deactivation();
	d_map_canvas_tool_adapter.deactivate_canvas_tool();

	// Notify derived class that the currently selected canvas tool has just been deactivated.
	deactivated_selected_tool();
}


GPlatesGui::CanvasToolWorkflows::ToolType
GPlatesGui::CanvasToolWorkflow::get_selected_tool() const
{
	return d_selected_tool;
}


bool
GPlatesGui::CanvasToolWorkflow::contains_tool(
		CanvasToolWorkflows::ToolType tool) const
{
	return static_cast<bool>(get_selected_globe_and_map_canvas_tools(tool));
}
