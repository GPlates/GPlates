/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_MAPCANVASTOOLCHOICE_H
#define GPLATES_GUI_MAPCANVASTOOLCHOICE_H

#include <boost/noncopyable.hpp>
#include <QObject>

#include "CanvasToolChoice.h"
#include "FeatureFocus.h"
#include "FeatureTableModel.h"
#include "MapCanvasTool.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
	class MapTransform;
	class TopologySectionsContainer;
}

namespace GPlatesQtWidgets
{
	class MapCanvas;
	class MapView;
	class ViewportWindow;
	class FeaturePropertiesDialog;
	class DigitisationWidget;
	class ModifyReconstructionPoleWidget;
	class TopologyToolsWidget;
}

namespace GPlatesCanvasTools
{
	class MeasureDistanceState;
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
	/**
	 * This class contains the current choice of MapCanvasTool.
	 *
	 * It also provides slots to choose the MapCanvasTool.
	 *
	 * This serves the role of the Context class in the State Pattern in Gamma et al.
	 */
	class MapCanvasToolChoice:
			public QObject,
			public boost::noncopyable
	{
		Q_OBJECT

	public:

		/**
		 * Construct a MapCanvasToolChoice instance.
		 *
		 * These parameters are needed by various MapCanvasTool derivations, which will be
		 * instantiated by this class.
		 */
		MapCanvasToolChoice(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
				GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				const GPlatesQtWidgets::ViewportWindow &viewport_window_,
				FeatureTableModel &clicked_table_model,
				GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget,
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
				GPlatesCanvasTools::MeasureDistanceState &measure_distance_state,
				GPlatesGui::MapTransform &map_transform_,
				GPlatesAppLogic::ApplicationState &application_state);

		~MapCanvasToolChoice()
		{  }

		MapCanvasTool &
		tool_choice() const
		{
			return *d_tool_choice_ptr;
		}

		CanvasToolChoice::Type
		tool_choice_as_enum() const
		{
			return d_tool_choice_as_enum;
		}

		void
		set_tool_choice(
				CanvasToolChoice::Type canvas_tool);

	signals:

		void
		tool_choice_changed(
				GPlatesGui::CanvasToolChoice::Type canvas_tool);

	public slots:

		void
		choose_pan_map_tool()
		{
			change_tool_if_necessary(d_pan_map_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::REORIENT_GLOBE_OR_PAN_MAP);
		}

		void
		choose_zoom_map_tool()
		{
			change_tool_if_necessary(d_zoom_map_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::ZOOM);
		}

		void
		choose_click_geometry_tool()
		{
			change_tool_if_necessary(d_click_geometry_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::CLICK_GEOMETRY);
		}

		void
		choose_digitise_polyline_tool()
		{
			change_tool_if_necessary(d_digitise_polyline_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::DIGITISE_POLYLINE);
		}

		void
		choose_digitise_multipoint_tool()
		{
			change_tool_if_necessary(d_digitise_multipoint_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::DIGITISE_MULTIPOINT);
		}

		void
		choose_digitise_polygon_tool()
		{
			change_tool_if_necessary(d_digitise_polygon_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::DIGITISE_POLYGON);
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
			set_enum_and_emit_signal(CanvasToolChoice::MOVE_VERTEX);
		}

		void
		choose_delete_vertex_tool()
		{
			change_tool_if_necessary(d_delete_vertex_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::DELETE_VERTEX);
		}

		void
		choose_insert_vertex_tool()
		{
			change_tool_if_necessary(d_insert_vertex_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::INSERT_VERTEX);
		}

		void
		choose_split_feature_tool()
		{
			change_tool_if_necessary(d_insert_vertex_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::SPLIT_FEATURE);
		}

		void
		choose_manipulate_pole_tool()
		{
			change_tool_if_necessary(d_manipulate_pole_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::MANIPULATE_POLE);
		}

		void
		choose_measure_distance_tool()
		{
			change_tool_if_necessary(d_measure_distance_tool_ptr);
			set_enum_and_emit_signal(CanvasToolChoice::MEASURE_DISTANCE);
		}

	private:
		/**
		 * This is the PanMap tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_pan_map_tool_ptr;

		/**
		 * This is the ZoomMap tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_zoom_map_tool_ptr;

		/**
		 * This is the MapClickGeometry tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_click_geometry_tool_ptr;

		/**
		 * This is the DigitiseGeometry (Polyline) tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_digitise_polyline_tool_ptr;

		/**
		 * This is the DigitiseGeometry (MultiPoint) tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_digitise_multipoint_tool_ptr;

		/**
		 * This is the DigitiseGeometry (Polygon) tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_digitise_polygon_tool_ptr;

		/**
		 * This is the MoveVertex tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_move_vertex_tool_ptr;

		/**
		* This is the DeleteVertex tool which the user may choose.
		*/
		MapCanvasTool::non_null_ptr_type d_delete_vertex_tool_ptr;

		/**
		* This is the InsertVertex tool which the user may choose.
		*/
		MapCanvasTool::non_null_ptr_type d_insert_vertex_tool_ptr;


#if 0
		/**
		 * This is the MoveGeometry tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_move_geometry_tool_ptr;

#endif
		/**
		 * This is the ManipulatePole tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_manipulate_pole_tool_ptr;

		/**
		 * This is the Measure Distance canvas tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_measure_distance_tool_ptr;

		/**
		 * The current choice of CanvasTool.
		 */
		MapCanvasTool::non_null_ptr_type d_tool_choice_ptr;

		/**
		 * An enumerated value that represents the current choice of GlobeCanvasTool.
		 */
		CanvasToolChoice::Type d_tool_choice_as_enum;

		void
		change_tool_if_necessary(
				MapCanvasTool::non_null_ptr_type new_tool_choice);

		void
		set_enum_and_emit_signal(
				CanvasToolChoice::Type canvas_tool);
	};
}

#endif  // GPLATES_GUI_MAPCANVASTOOLCHOICE_H
