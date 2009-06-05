/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBECANVASTOOLCHOICE_H
#define GPLATES_GUI_GLOBECANVASTOOLCHOICE_H

#include <boost/noncopyable.hpp>
#include <QObject>

#include "gui/ColourTable.h"
#include "gui/FeatureFocus.h"
#include "GlobeCanvasTool.h"
#include "FeatureTableModel.h"

namespace GPlatesGui
{
	class ChooseCanvasTool;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
	class FeaturePropertiesDialog;
	class DigitisationWidget;
	class ReconstructionPoleWidget;
	class TopologyToolsWidget;
}

namespace GPlatesViewOperations
{
	class ActiveGeometryOperation;
	class GeometryOperationTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class GeometryFocusHighlight;

	/**
	 * This class contains the current choice of GlobeCanvasTool.
	 *
	 * It also provides slots to choose the GlobeCanvasTool.
	 *
	 * This serves the role of the Context class in the State Pattern in Gamma et al.
	 */
	class GlobeCanvasToolChoice:
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		/**
		 * Construct a GlobeCanvasToolChoice instance.
		 *
		 * These parameters are needed by various GlobeCanvasTool derivations, which will be
		 * instantiated by this class.
		 */
		GlobeCanvasToolChoice(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				const GPlatesQtWidgets::ViewportWindow &view_state,
				FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesQtWidgets::ReconstructionPoleWidget &pole_widget,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
				GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight);

		~GlobeCanvasToolChoice()
		{  }

		GlobeCanvasTool &
		tool_choice() const
		{
			return *d_tool_choice_ptr;
		}

	public slots:
		void
		choose_reorient_globe_tool()
		{
			change_tool_if_necessary(d_reorient_globe_tool_ptr);
		}

		void
		choose_zoom_globe_tool()
		{
			change_tool_if_necessary(d_zoom_globe_tool_ptr);
		}

		void
		choose_click_geometry_tool()
		{
			change_tool_if_necessary(d_click_geometry_tool_ptr);
		}

		void
		choose_digitise_polyline_tool()
		{
			change_tool_if_necessary(d_digitise_polyline_tool_ptr);
		}

		void
		choose_digitise_multipoint_tool()
		{
			change_tool_if_necessary(d_digitise_multipoint_tool_ptr);
		}

		void
		choose_digitise_polygon_tool()
		{
			change_tool_if_necessary(d_digitise_polygon_tool_ptr);
		}
#if 0
		void
		choose_move_geometry_tool()
		{
			change_tool_if_necessary(d_move_geometry_tool_ptr);
		}
#endif
		void
		choose_move_vertex_tool()
		{
			change_tool_if_necessary(d_move_vertex_tool_ptr);
		}

		void
		choose_delete_vertex_tool()
		{
			change_tool_if_necessary(d_delete_vertex_tool_ptr);
		}

		void
		choose_insert_vertex_tool()
		{
			change_tool_if_necessary(d_insert_vertex_tool_ptr);
		}

		void
		choose_manipulate_pole_tool()
		{
			change_tool_if_necessary(d_manipulate_pole_tool_ptr);
		}

	private:
		/**
		 * This is the ReorientGlobe tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_reorient_globe_tool_ptr;

		/**
		 * This is the ZoomGlobe tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_zoom_globe_tool_ptr;

		/**
		 * This is the ClickGeometry tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_click_geometry_tool_ptr;

		/**
		 * This is the GlobeDigitiseGeometry (Polyline) tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_digitise_polyline_tool_ptr;

		/**
		 * This is the GlobeDigitiseGeometry (MultiPoint) tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_digitise_multipoint_tool_ptr;

		/**
		 * This is the GlobeDigitiseGeometry (Polygon) tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_digitise_polygon_tool_ptr;
#if 0
		/**
		 * This is the MoveGeometry tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_move_geometry_tool_ptr;
#endif
		/**
		 * This is the GlobeMoveVertex tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_move_vertex_tool_ptr;

		/**
		 * This is the DeleteVertex tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_delete_vertex_tool_ptr;

		/**
		 * This is the InsertVertex tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_insert_vertex_tool_ptr;

		/**
		 * This is the ManipulatePole tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_manipulate_pole_tool_ptr;

		/**
		 * The current choice of GlobeCanvasTool.
		 */
		GlobeCanvasTool::non_null_ptr_type d_tool_choice_ptr;

		void
		change_tool_if_necessary(
				GlobeCanvasTool::non_null_ptr_type new_tool_choice);
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOLCHOICE_H
