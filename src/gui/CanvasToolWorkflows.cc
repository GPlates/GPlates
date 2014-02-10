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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "CanvasToolWorkflows.h"

#include "CanvasToolWorkflow.h"
#include "DigitisationCanvasToolWorkflow.h"
#include "FeatureInspectionCanvasToolWorkflow.h"
#include "HellingerCanvasToolWorkflow.h"
#include "PoleManipulationCanvasToolWorkflow.h"
#include "SmallCircleCanvasToolWorkflow.h"
#include "TopologyCanvasToolWorkflow.h"
#include "ViewCanvasToolWorkflow.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "presentation/ViewState.h"

#include "qt-widgets/ViewportWindow.h"

GPlatesGui::CanvasToolWorkflows::CanvasToolWorkflows() :
	d_active_workflow(WORKFLOW_VIEW)
{
}


void
GPlatesGui::CanvasToolWorkflows::initialise(
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
	create_canvas_tool_workflows(
			geometry_operation_state,
			modify_geometry_state,
			measure_distance_state,
			status_bar_callback,
			view_state,
			viewport_window);

	// Listen for enable/disable of individual canvas tools in the workflows.
	BOOST_FOREACH(
			const boost::shared_ptr<CanvasToolWorkflow> &canvas_tool_workflow,
			d_canvas_tool_workflows)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				canvas_tool_workflow,
				GPLATES_ASSERTION_SOURCE);

		QObject::connect(
				canvas_tool_workflow.get(),
				SIGNAL(canvas_tool_enabled(
						GPlatesGui::CanvasToolWorkflows::WorkflowType,
						GPlatesGui::CanvasToolWorkflows::ToolType,
						bool)),
				this,
				SLOT(handle_canvas_tool_enabled(
						GPlatesGui::CanvasToolWorkflows::WorkflowType,
						GPlatesGui::CanvasToolWorkflows::ToolType,
						bool)));
	}

	// Initialise each workflow.
	BOOST_FOREACH(
			const boost::shared_ptr<CanvasToolWorkflow> &canvas_tool_workflow,
			d_canvas_tool_workflows)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				canvas_tool_workflow,
				GPLATES_ASSERTION_SOURCE);

		canvas_tool_workflow->initialise();
	}
}


void
GPlatesGui::CanvasToolWorkflows::activate()
{
	// Starts things off by activating the default workflow and its default tool.
	d_canvas_tool_workflows[d_active_workflow]->activate();

	// Let clients know of the initial workflow/tool.
	Q_EMIT canvas_tool_activated(d_active_workflow, d_canvas_tool_workflows[d_active_workflow]->get_selected_tool());
}


std::pair<GPlatesGui::CanvasToolWorkflows::WorkflowType, GPlatesGui::CanvasToolWorkflows::ToolType>
GPlatesGui::CanvasToolWorkflows::get_active_canvas_tool() const
{
	// Make sure 'initialise()' has been called.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_canvas_tool_workflows.empty(),
			GPLATES_ASSERTION_SOURCE);

	return std::make_pair(
			d_active_workflow,
			d_canvas_tool_workflows[d_active_workflow]->get_selected_tool());
}


GPlatesGui::CanvasToolWorkflows::ToolType
GPlatesGui::CanvasToolWorkflows::get_selected_canvas_tool_in_workflow(
		WorkflowType workflow) const
{
	// Make sure 'initialise()' has been called.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_canvas_tool_workflows.empty(),
			GPLATES_ASSERTION_SOURCE);

	return d_canvas_tool_workflows[workflow]->get_selected_tool();
}


bool
GPlatesGui::CanvasToolWorkflows::is_canvas_tool_enabled(
		WorkflowType workflow,
		ToolType tool) const
{
	// Make sure 'initialise()' has been called.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_canvas_tool_workflows.empty(),
			GPLATES_ASSERTION_SOURCE);

	return d_canvas_tool_workflows[workflow]->is_tool_enabled(tool);
}


bool
GPlatesGui::CanvasToolWorkflows::does_workflow_contain_tool(
		WorkflowType workflow,
		ToolType tool) const
{
	// Make sure 'initialise()' has been called.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_canvas_tool_workflows.empty(),
			GPLATES_ASSERTION_SOURCE);

	return d_canvas_tool_workflows[workflow]->contains_tool(tool);
}


void
GPlatesGui::CanvasToolWorkflows::choose_canvas_tool(
		ToolType tool)
{
	// Make sure 'initialise()' has been called.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_canvas_tool_workflows.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the workflows and make sure tool only exists in one workflow.
	boost::optional<WorkflowType> workflow;
	for (unsigned int workflow_index = 0; workflow_index < NUM_WORKFLOWS; ++workflow_index)
	{
		const WorkflowType workflow_type = static_cast<WorkflowType>(workflow_index);

		if (d_canvas_tool_workflows[workflow_type]->contains_tool(tool))
		{
			// Make sure tool doesn't exist in multiple workflows.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					!workflow,
					GPLATES_ASSERTION_SOURCE);

			workflow = workflow_type;
		}
	}

	// The tool should exist in one of the workflows.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			workflow,
			GPLATES_ASSERTION_SOURCE);

	choose_canvas_tool(workflow.get(), tool);
}


void
GPlatesGui::CanvasToolWorkflows::choose_canvas_tool(
		WorkflowType workflow,
		boost::optional<ToolType> tool_opt)
{
	// Make sure 'initialise()' has been called.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_canvas_tool_workflows.empty(),
			GPLATES_ASSERTION_SOURCE);

	// The selected tool.
	const ToolType tool = tool_opt
			? tool_opt.get()
			: d_canvas_tool_workflows[workflow]->get_selected_tool();

	// Return early if the canvas workflow/tool has not changed.
	if (workflow == d_active_workflow &&
		tool == d_canvas_tool_workflows[d_active_workflow]->get_selected_tool())
	{
		return;
	}

	// If the workflow has changed then deactivate the current workflow first.
	if (workflow != d_active_workflow)
	{
		d_canvas_tool_workflows[d_active_workflow]->deactivate();
		d_active_workflow = workflow;
	}

	// Activate the specified tool in the workflow.
	d_canvas_tool_workflows[d_active_workflow]->activate(tool);

	Q_EMIT canvas_tool_activated(workflow, tool);
}


void
GPlatesGui::CanvasToolWorkflows::handle_canvas_tool_enabled(
		WorkflowType workflow,
		ToolType tool,
		bool enable)
{
	// Get the canvas tool workflow that emitted the signal.
	CanvasToolWorkflow *canvas_tool_workflow = d_canvas_tool_workflows[workflow].get();

	// Make sure the sender is the canvas tool workflow it says it is.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			qobject_cast<CanvasToolWorkflow *>(sender()) == canvas_tool_workflow,
			GPLATES_ASSERTION_SOURCE);

	// Emit a signal for our clients.
	// We're gathering signals emitted by individual workflows and re-emitting for client's convenience.
	Q_EMIT canvas_tool_enabled(workflow, tool, enable);
}


void
GPlatesGui::CanvasToolWorkflows::create_canvas_tool_workflows(
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
		GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
	// Initialise the individual canvas tool workflows.
	d_canvas_tool_workflows.resize(NUM_WORKFLOWS);

 	d_canvas_tool_workflows[WORKFLOW_VIEW].reset(
 			new ViewCanvasToolWorkflow(
					*this,
					status_bar_callback,
					view_state,
					viewport_window));

 	d_canvas_tool_workflows[WORKFLOW_FEATURE_INSPECTION].reset(
 			new FeatureInspectionCanvasToolWorkflow(
					*this,
					geometry_operation_state,
					modify_geometry_state,
					measure_distance_state,
					status_bar_callback,
					view_state,
					viewport_window));

 	d_canvas_tool_workflows[WORKFLOW_DIGITISATION].reset(
 			new DigitisationCanvasToolWorkflow(
					*this,
					geometry_operation_state,
					modify_geometry_state,
					measure_distance_state,
					status_bar_callback,
					view_state,
					viewport_window));

	d_canvas_tool_workflows[WORKFLOW_TOPOLOGY].reset(
			new TopologyCanvasToolWorkflow(
					*this,
					status_bar_callback,
					view_state,
					viewport_window));

	d_canvas_tool_workflows[WORKFLOW_POLE_MANIPULATION].reset(
			new PoleManipulationCanvasToolWorkflow(
					*this,
					status_bar_callback,
					view_state,
					viewport_window));

	d_canvas_tool_workflows[WORKFLOW_SMALL_CIRCLE].reset(
			new SmallCircleCanvasToolWorkflow(
					*this,
					geometry_operation_state,
					measure_distance_state,
					status_bar_callback,
					view_state,
					viewport_window));

	d_canvas_tool_workflows[WORKFLOW_HELLINGER].reset(
			new HellingerCanvasToolWorkflow(
					*this,
					status_bar_callback,
					view_state,
					viewport_window));
}
