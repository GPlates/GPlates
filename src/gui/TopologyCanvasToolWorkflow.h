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

#ifndef GPLATES_GUI_TOPOLOGYCANVASTOOLWORKFLOW_H
#define GPLATES_GUI_TOPOLOGYCANVASTOOLWORKFLOW_H

#include <boost/scoped_ptr.hpp>
#include <utility>

#include "CanvasToolWorkflow.h"
#include "GlobeCanvasTool.h"
#include "MapCanvasTool.h"

#include "GlobeCanvasToolAdapter.h"

#include "canvas-tools/CanvasTool.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class FeatureFocus;

	/**
	 * The canvas tool workflow for building/editing topological features.
	 */
	class TopologyCanvasToolWorkflow :
			public CanvasToolWorkflow
	{
		Q_OBJECT

	public:

		TopologyCanvasToolWorkflow(
				CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);

		virtual
		void
		initialise();

	protected:

		virtual
		void
		activate_workflow();

		virtual
		void
		deactivate_workflow();

		virtual
		boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
		get_selected_globe_and_map_canvas_tools(
					CanvasToolWorkflows::ToolType selected_tool) const;

	private Q_SLOTS:

		/**
		 * Changed which reconstruction geometry is currently focused.
		 */
		void
		feature_focus_changed(
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Changed the selected canvas tool.
		 */
		void
		handle_canvas_tool_activated(
				GPlatesGui::CanvasToolWorkflows::WorkflowType workflow,
				GPlatesGui::CanvasToolWorkflows::ToolType tool);

	private:

		//! For determining the curently active workflow/tool.
		CanvasToolWorkflows &d_canvas_tool_workflows;

		/**
		 * The focused feature, in part, determines which tools are enabled.
		 */
		FeatureFocus &d_feature_focus;

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		//! For building line topologies in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_build_line_topology_tool;
		//! For building line topologies in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_build_line_topology_tool;

		//! For building boundary topologies in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_build_boundary_topology_tool;
		//! For building boundary topologies in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_build_boundary_topology_tool;

		//! For building network topologies in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_build_network_topology_tool;
		//! For building network topologies in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_build_network_topology_tool;

		//! For editing topologies in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_edit_topology_tool;
		//! For editing topologies in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_edit_topology_tool;


		void
		create_canvas_tools(
				CanvasToolWorkflows &canvas_tool_workflows,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);

		void
		update_enable_state();

		void
		update_build_topology_tools();

		void
		update_edit_topology_tool();
	};
}

#endif // GPLATES_GUI_TOPOLOGYCANVASTOOLWORKFLOW_H
