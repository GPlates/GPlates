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

#ifndef GPLATES_CANVASTOOLS_DIGITISATIONCANVASTOOLWORKFLOW_H
#define GPLATES_CANVASTOOLS_DIGITISATIONCANVASTOOLWORKFLOW_H

#include <boost/scoped_ptr.hpp>
#include <utility>

#include "CanvasToolWorkflow.h"
#include "GlobeCanvasTool.h"
#include "MapCanvasTool.h"

#include "GlobeCanvasToolAdapter.h"

#include "canvas-tools/CanvasTool.h"

#include "view-operations/GeometryType.h"


namespace GPlatesCanvasTools
{
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
	/**
	 * The canvas tool workflow for digitising new features as point/multipoint/polyline/polygon.
	 */
	class DigitisationCanvasToolWorkflow :
			public CanvasToolWorkflow
	{
		Q_OBJECT

	public:

		DigitisationCanvasToolWorkflow(
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

	private slots:

		/**
		 * Focused feature geometry changes.
		 */
		void
		geometry_builder_stopped_updating_geometry_excluding_intermediate_moves();

	private:

		GPlatesViewOperations::GeometryBuilder &d_digitise_geometry_builder;

		GPlatesCanvasTools::GeometryOperationState &d_geometry_operation_state;

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		//! For measuring distance in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_measure_distance_tool;
		//! For measuring distance in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_measure_distance_tool;

		//! For digitising multipoints in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_digitise_multipoint_tool;
		//! For digitising multipoints in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_digitise_multipoint_tool;

		//! For digitising polylines in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_digitise_polyline_tool;
		//! For digitising polylines in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_digitise_polyline_tool;

		//! For digitising polygons in the 3D globe view.
		boost::scoped_ptr<GlobeCanvasTool> d_globe_digitise_polygon_tool;
		//! For digitising polygons in the 2D map view.
		boost::scoped_ptr<MapCanvasTool> d_map_digitise_polygon_tool;

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


		void
		create_canvas_tools(
				CanvasToolWorkflows &canvas_tool_workflows,
				GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
				GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window);

		void
		update_enable_state();

		std::pair<unsigned int, GPlatesViewOperations::GeometryType::Value>
		get_geometry_builder_parameters() const;
	};
}

#endif // GPLATES_CANVASTOOLS_DIGITISATIONCANVASTOOLWORKFLOW_H
