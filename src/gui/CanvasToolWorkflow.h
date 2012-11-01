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

#ifndef GPLATES_GUI_CANVASTOOLWORKFLOW_H
#define GPLATES_GUI_CANVASTOOLWORKFLOW_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <QObject>

#include "CanvasToolWorkflows.h"

#include "GlobeCanvasToolAdapter.h"
#include "MapCanvasToolAdapter.h"


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class MapView;
}

namespace GPlatesGui
{
	/**
	 * Abstract base class for a canvas tool workflow.
	 */
	class CanvasToolWorkflow :
			public QObject
	{
		Q_OBJECT

	public:

		virtual
		~CanvasToolWorkflow()
		{  }


		/**
		 * Initialise the workflow - such as enable/disable canvas tools.
		 */
		virtual
		void
		initialise() = 0;


		/**
		 * Activate the workflow (if not already active) and select the specified tool.
		 *
		 * If this workflow is already active then deactivates the currently active tool in this workflow
		 * before activating the selected tool.
		 *
		 * If @a select_tool is not specified then the currently selected tool in this workflow
		 * (as returned by @a get_selected_tool) remains the selected tool.
		 */
		void
		activate(
				boost::optional<CanvasToolWorkflows::ToolType> select_tool = boost::none);


		/**
		 * De-activates the current workflow.
		 *
		 * Also de-activates the currently active tool in this workflow.
		 */
		void
		deactivate();


		/**
		 * Returns the workflow type of this workflow.
		 */
		CanvasToolWorkflows::WorkflowType
		get_workflow() const;


		/**
		 * Returns the currently selected tool in the workflow.
		 *
		 * NOTE: This can be called even if the workflow is not active in which case
		 * it returns the tool that was last active in this workflow.
		 */
		CanvasToolWorkflows::ToolType
		get_selected_tool() const;


		/**
		 * Returns true if this workflow contains the specified tool.
		 *
		 * Not all workflows support all tools (in fact no workflow supports all tools).
		 */
		bool
		contains_tool(
				CanvasToolWorkflows::ToolType tool) const;


		/**
		 * Returns true if the specified tool is currently enabled.
		 */
		bool
		is_tool_enabled(
				CanvasToolWorkflows::ToolType tool) const;

	signals:

		/**
		 * Emitted when a canvas tool is enabled/disabled.
		 *
		 * NOTE: Derived classes should call @a emit_canvas_tool_enabled instead of directly emitting this signal.
		 */
		void
		canvas_tool_enabled(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				bool enable);

	protected:

		CanvasToolWorkflow(
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				GPlatesQtWidgets::MapView &map_view,
				CanvasToolWorkflows::WorkflowType workflow,
				CanvasToolWorkflows::ToolType selected_tool);


		//! Returns true if this workflow is currently active.
		bool
		is_workflow_active() const
		{
			return d_is_workflow_active;
		}

		//! Emits the @a canvas_tool_enabled signal.
		void
		emit_canvas_tool_enabled(
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				bool enable);


		/**
		 * Implemented by derived class to perform any setup when workflow is activated.
		 */
		virtual
		void
		activate_workflow() = 0;

		/**
		 * Implemented by derived class to perform any cleanup when workflow is deactivated.
		 */
		virtual
		void
		deactivate_workflow() = 0;

		/**
		 * Notifies derived class about to activate the currently selected canvas tool.
		 */
		virtual
		void
		activating_selected_tool()
		{ }

		/**
		 * Notifies derived class that the currently selected canvas tool has just been deactivated.
		 */
		virtual
		void
		deactivated_selected_tool()
		{ }

		/**
		 * Implemented by derived class to return the specified globe and map canvas tool, or none
		 * if the selected tool does not exist in this workflow (ie, if @a contains_tool returns false).
		 */
		virtual
		boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
		get_selected_globe_and_map_canvas_tools(
					CanvasToolWorkflows::ToolType selected_tool) const = 0;

	//
	// NOTE: The following are 'private' to ensure that derived classes cannot directly access them.
	// Use 'protected' methods to provide interface for derived classes.
	//
	private:

		//! Typedef for a sequence of flags specifying which tools are enabled in this workflow.
		typedef std::vector<bool> enabled_tools_seq_type;


		/**
		 * Feeds mouse events from @a GlobeCanvas to our selected *globe-view* tool.
		 */
		GlobeCanvasToolAdapter d_globe_canvas_tool_adapter;

		/**
		 * Feeds mouse events from @a MapView to our selected *map-view* tool.
		 */
		MapCanvasToolAdapter d_map_canvas_tool_adapter;

		/**
		 * The type of this workflow.
		 */
		CanvasToolWorkflows::WorkflowType d_workflow;

		/**
		 * The currently selected tool for this workflow.
		 *
		 * This remains the selected tool even if the workflow is inactive.
		 * In which case its the tool to activate when the workflow becomes active again.
		 */
		CanvasToolWorkflows::ToolType d_selected_tool;

		/**
		 * Is true if this workflow is currently active.
		 *
		 * Also means the currently selected tool (for this workflow) is active.
		 */
		bool d_is_workflow_active;

		/**
		 * Is true if the selected tool is currently active.
		 */
		bool d_is_selected_tool_active;

		/**
		 * Flags recording which tools, in this workflow, are currently enabled.
		 */
		enabled_tools_seq_type d_enabled_tools;


		void
		activate_selected_tool();

		void
		deactivate_selected_tool();
	};
}

#endif // GPLATES_GUI_CANVASTOOLWORKFLOW_H
