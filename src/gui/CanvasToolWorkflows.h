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

#ifndef GPLATES_GUI_CANVASTOOLWORKFLOWS_H
#define GPLATES_GUI_CANVASTOOLWORKFLOWS_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QString>
#include <QUndoCommand>

#include "canvas-tools/CanvasTool.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
	class MeasureDistanceState;
	class ModifyGeometryState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesGui
{
	class CanvasToolWorkflow;

	/**
	 * Manages the canvas tool 'workflows'.
	 *
	 * Each workflow has its own active canvas tool independent of other workflows.
	 * This enables the user to perform relatively independent tasks such as digitising geometry in
	 * one workflow while moving vertices of the focused feature in another workflow at the same time.
	 * The workflows correspond to the tabs on the tabbed canvas tool bar widget.
	 */
	class CanvasToolWorkflows :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * The canvas tool work - corresponds to tabs on the tabbed canvas tool bar widget.
		 */
		enum WorkflowType
		{
			WORKFLOW_FEATURE_INSPECTION,
			WORKFLOW_DIGITISATION,
			WORKFLOW_TOPOLOGY,
			WORKFLOW_POLE_MANIPULATION,

			NUM_WORKFLOWS // NOTE: This must be last.
		};

		/**
		 * The type of canvas tool.
		 *
		 * NOTE: The same tool type can be used in multiple workflows.
		 * NOTE: Each workflow only uses a subset of all available tool types.
		 */
		enum ToolType
		{
			TOOL_DRAG_GLOBE,
			TOOL_ZOOM_GLOBE,
			TOOL_MEASURE_DISTANCE,
			TOOL_CLICK_GEOMETRY,
			TOOL_DIGITISE_NEW_POLYLINE,
			TOOL_DIGITISE_NEW_MULTIPOINT,
			TOOL_DIGITISE_NEW_POLYGON,
			TOOL_MOVE_VERTEX,
			TOOL_DELETE_VERTEX,
			TOOL_INSERT_VERTEX,
			TOOL_SPLIT_FEATURE,
			TOOL_MANIPULATE_POLE,
			TOOL_BUILD_TOPOLOGY,
			TOOL_EDIT_TOPOLOGY,
			TOOL_CREATE_SMALL_CIRCLE,

			NUM_TOOLS // NOTE: This must be last.
		};


		CanvasToolWorkflows();


		/**
		 * Call once everything is setup (including the GUI).
		 */
		void
		initialise(
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);


		/**
		 * Returns the currently active canvas tool workflow/tool.
		 */
		std::pair<WorkflowType, ToolType>
		get_active_canvas_tool() const;


		/**
		 * Returns true if the specified workflow contains the specified tool.
		 *
		 * Not all workflows support all tools (in fact no workflow supports all tools).
		 * Invalid combinations will return false.
		 */
		bool
		does_workflow_contain_tool(
				WorkflowType workflow,
				ToolType tool) const;

	signals:

		/**
		 * Emitted when a canvas tool in a workflow is enabled/disabled.
		 */
		void
		canvas_tool_enabled(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				bool enable);

		/**
		 * Emitted when a canvas tool in a workflow is activated.
		 *
		 * This happens whenever @a choose_canvas_tool is called.
		 * This can happen when the user explicitly triggers the tool
		 * (see signal CanvasToolBarDockWidget::canvas_tool_triggered_by_user) or
		 * when GPlates automatically chooses a tool (such as during undo).
		 */
		void
		canvas_tool_activated(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool);

	public slots:

		/**
		 * Makes the specified canvas workflow/tool the currently active workflow/tool.
		 *
		 * If @a tool is not specified then the currently selected tool in @a workflow is used.
		 *
		 * NOTE: Not all workflows support all tools (in fact no workflow supports all tools).
		 * Invalid combinations will results in a thrown exception.
		 */
		void
		choose_canvas_tool(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				boost::optional<GPlatesGui::CanvasToolWorkflows::ToolType> tool = boost::none);

	private slots:

		/**
		 * This handler is connected to each individual workflow.
		 */
		void
		handle_canvas_tool_enabled(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool,
				bool enable);

	private:

		//! Typedef for a sequence of canvas tool workflows.
		typedef std::vector< boost::shared_ptr<CanvasToolWorkflow> > canvas_tool_workflow_seq_type;


		canvas_tool_workflow_seq_type d_canvas_tool_workflows;

		//! The currently active workflow.
		WorkflowType d_active_workflow;


		void
		create_canvas_tool_workflows(
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);
	};
}

#endif // GPLATES_GUI_CANVASTOOLWORKFLOWS_H
