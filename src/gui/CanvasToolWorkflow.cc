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
		GPlatesQtWidgets::MapView &map_view,
		CanvasToolWorkflows::WorkflowType workflow,
		CanvasToolWorkflows::ToolType selected_tool) :
	d_globe_canvas_tool_adapter(globe_canvas),
	d_map_canvas_tool_adapter(map_view),
	d_workflow(workflow),
	d_selected_tool(selected_tool),
	d_is_workflow_active(false),
	d_is_selected_tool_active(false),
	// All tools disabled by default...
	d_enabled_tools(CanvasToolWorkflows::NUM_TOOLS)
{
}


void
GPlatesGui::CanvasToolWorkflow::activate(
		boost::optional<CanvasToolWorkflows::ToolType> select_tool_opt)
{
	// The newly selected tool.
	const GPlatesGui::CanvasToolWorkflows::ToolType select_tool =
			select_tool_opt ? select_tool_opt.get() : d_selected_tool;

	// If the selected tool is the same and its workflow (us) is already active then there's nothing to do.
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


GPlatesGui::CanvasToolWorkflows::WorkflowType
GPlatesGui::CanvasToolWorkflow::get_workflow() const
{
	return d_workflow;
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


bool
GPlatesGui::CanvasToolWorkflow::is_tool_enabled(
		CanvasToolWorkflows::ToolType tool) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			static_cast<unsigned int>(tool) < d_enabled_tools.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_enabled_tools[tool];
}


void
GPlatesGui::CanvasToolWorkflow::emit_canvas_tool_enabled(
		GPlatesGui::CanvasToolWorkflows::ToolType tool,
		bool enable)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			static_cast<unsigned int>(tool) < d_enabled_tools.size(),
			GPLATES_ASSERTION_SOURCE);

	const bool previous_enable_state = d_enabled_tools[tool];

	// Record which tools are enabled for this workflow.
	d_enabled_tools[tool] = enable;

	// If this workflow is currently active and the tool is the selected tool and it has just
	// been enabled then activate the canvas tool.
	if (d_is_workflow_active && (tool == d_selected_tool))
	{
		if (enable && !previous_enable_state)
		{
			activate_selected_tool();
		}
		else if (!enable && previous_enable_state)
		{
			deactivate_selected_tool();
		}
	}

	emit canvas_tool_enabled(d_workflow, tool, enable);
}


void
GPlatesGui::CanvasToolWorkflow::activate_selected_tool()
{
	// If the selected tool has been disabled then we don't activate it.
	//
	// This can happen when switching between canvas tool tabs (in the GUI) - it's possible for
	// the previous tool (in a different canvas tool workflow/tab) to have entered a state that
	// disables the current tool (in the current workflow/tab) - when the user switches to the
	// current tab the selected tool is disabled.
	if (!is_tool_enabled(d_selected_tool))
	{
		return;
	}

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

	// The should not be a currently active selected tool.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_is_selected_tool_active,
			GPLATES_ASSERTION_SOURCE);

	// Activate the selected globe canvas tool.
	globe_map_canvas_tool->first->handle_activation();
	d_globe_canvas_tool_adapter.activate_canvas_tool(*globe_map_canvas_tool->first);

	// Activate the selected map canvas tool.
	globe_map_canvas_tool->second->handle_activation();
	d_map_canvas_tool_adapter.activate_canvas_tool(*globe_map_canvas_tool->second);

	// Record that we activated the currently selected tool so we know to deactivate when the time comes.
	d_is_selected_tool_active = true;
}


void
GPlatesGui::CanvasToolWorkflow::deactivate_selected_tool()
{
	// If the selected tool has been activated then we deactivate it.
	// It's possible for the selected tool to be inactive if it was disabled when we tried to activate it.
	if (!d_is_selected_tool_active)
	{
		return;
	}

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

	// Record that we deactivated the currently selected tool.
	d_is_selected_tool_active = false;

	// Notify derived class that the currently selected canvas tool has just been deactivated.
	deactivated_selected_tool();
}
