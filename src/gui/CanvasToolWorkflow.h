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
		 * Returns the currently selected tool in the workflow.
		 *
		 * NOTE: This can be called even if the workflow is not active in which case
		 * it returns the tool that was last active in this workflow.
		 */
		CanvasToolWorkflows::ToolType
		get_selected_tool() const;

	signals:

		/**
		 * Emitted when a canvas tool is enabled/disabled.
		 */
		void
		canvas_tool_enabled(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				bool enable);

	protected:

		CanvasToolWorkflow(
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				GPlatesQtWidgets::MapView &map_view);

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
		 * Implemented by derived class to return the specified globe and map canvas tool, or none on error.
		 */
		virtual
		boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
		get_selected_globe_and_map_canvas_tools(
					CanvasToolWorkflows::ToolType selected_tool) const = 0;

	private:

		/**
		 * Feeds mouse events from @a GlobeCanvas to our selected *globe-view* tool.
		 */
		GlobeCanvasToolAdapter d_globe_canvas_tool_adapter;

		/**
		 * Feeds mouse events from @a MapView to our selected *map-view* tool.
		 */
		MapCanvasToolAdapter d_map_canvas_tool_adapter;

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


		void
		activate_selected_tool();

		void
		deactivate_selected_tool();
	};
}

#endif // GPLATES_GUI_CANVASTOOLWORKFLOW_H
