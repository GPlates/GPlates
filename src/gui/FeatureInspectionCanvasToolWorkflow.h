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

#ifndef GPLATES_GUI_FEATUREINSPECTIONCANVASTOOLWORKFLOW_H
#define GPLATES_GUI_FEATUREINSPECTIONCANVASTOOLWORKFLOW_H

#include <utility>
#include <vector>
#include <boost/scoped_ptr.hpp>

#include "CanvasToolWorkflow.h"
#include "GlobeCanvasTool.h"
#include "MapCanvasTool.h"

#include "GlobeCanvasToolAdapter.h"

#include "canvas-tools/CanvasTool.h"

#include "gui/Symbol.h"

#include "model/FeatureHandle.h"

#include "view-operations/GeometryType.h"


namespace GPlatesCanvasTools
{
	class ClickGeometry;
	class GeometryOperationState;
	class MeasureDistanceState;
	class ModifyGeometryState;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class FeatureFocus;

	/**
	 * The canvas tool workflow for query/editing a feature's properties including modifying
	 * its geometry using the MoveVertex tool, etc.
	 */
	class FeatureInspectionCanvasToolWorkflow :
			public CanvasToolWorkflow
	{
		Q_OBJECT

	public:

		FeatureInspectionCanvasToolWorkflow(
				CanvasToolWorkflows &canvas_tool_workflows,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
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

		void
		draw_feature_focus(
				GPlatesGui::FeatureFocus &feature_focus);

		void
		update_enable_state();

	private:

		//! For determining the curently active workflow/tool.
		CanvasToolWorkflows &d_canvas_tool_workflows;

		/**
		 * The focused feature, in part, determines which tools are enabled.
		 */
		FeatureFocus &d_feature_focus;

		GPlatesViewOperations::GeometryBuilder &d_focused_feature_geometry_builder;

		GPlatesCanvasTools::GeometryOperationState &d_geometry_operation_state;

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		const GPlatesGui::symbol_map_type &d_symbol_map;

		//! Used when restoring the clicked geometries on workflow activation.
		GPlatesQtWidgets::ViewportWindow &d_viewport_window;

		//! For measuring distance in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_measure_distance_tool;
		//! For measuring distance in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_measure_distance_tool;

		//! For clicking geometries in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_click_geometry_tool;
		//! For clicking geometries in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_click_geometry_tool;

		//! For moving geometry vertices in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_move_vertex_tool;
		//! For moving geometry vertices in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_move_vertex_tool;

		//! For deleting geometry vertices in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_delete_vertex_tool;
		//! For deleting geometry vertices in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_delete_vertex_tool;

		//! For inserting geometry vertices in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_insert_vertex_tool;
		//! For inserting geometry vertices in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_insert_vertex_tool;

		//! For splitting features in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_split_feature_tool;
		//! For splitting features in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_split_feature_tool;


		void
		create_canvas_tools(
				CanvasToolWorkflows &canvas_tool_workflows,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);

		std::pair<unsigned int, GPlatesViewOperations::GeometryType::Value>
		get_geometry_builder_parameters() const;
	};
}

#endif // GPLATES_GUI_FEATUREINSPECTIONCANVASTOOLWORKFLOW_H
