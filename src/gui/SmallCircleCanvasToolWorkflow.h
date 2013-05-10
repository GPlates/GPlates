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

#ifndef GPLATES_GUI_SMALLCIRCLECANVASTOOLWORKFLOW_H
#define GPLATES_GUI_SMALLCIRCLECANVASTOOLWORKFLOW_H

#include <boost/scoped_ptr.hpp>
#include <utility>

#include "CanvasToolWorkflow.h"
#include "GlobeCanvasTool.h"
#include "MapCanvasTool.h"

#include "GlobeCanvasToolAdapter.h"

#include "canvas-tools/CanvasTool.h"

#include "maths/GeometryType.h"


namespace GPlatesCanvasTools
{
	class GeometryOperationState;
	class MeasureDistanceState;
}

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
	/**
	 * The canvas tool workflow for creating small circles.
	 */
	class SmallCircleCanvasToolWorkflow :
			public CanvasToolWorkflow
	{
		Q_OBJECT

	public:

		SmallCircleCanvasToolWorkflow(
				CanvasToolWorkflows &canvas_tool_workflows,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
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

	private:

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		//! For creating small circles in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_create_small_circle_tool;
		//! For creating small circles in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_create_small_circle_tool;


		void
		create_canvas_tools(
				CanvasToolWorkflows &canvas_tool_workflows,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);
	};
}

#endif // GPLATES_GUI_SMALLCIRCLECANVASTOOLWORKFLOW_H
