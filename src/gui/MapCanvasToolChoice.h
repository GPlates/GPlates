/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <boost/scoped_ptr.hpp>
#include <QObject>

#include "canvas-tools/BuildTopology.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/CreateSmallCircle.h"
#include "canvas-tools/DeleteVertex.h"
#include "canvas-tools/DigitiseGeometry.h"
#include "canvas-tools/EditTopology.h"
#include "canvas-tools/InsertVertex.h"
#include "canvas-tools/ManipulatePole.h"
#include "canvas-tools/MeasureDistance.h"
#include "canvas-tools/MoveVertex.h"
#include "canvas-tools/SplitFeature.h"


namespace GPlatesQtWidgets
{
	class MapCanvas;
	class MapView;
	class ViewportWindow;
}

namespace GPlatesGui
{
	// Forward declarations.
	class MapCanvasTool;
	class MapTransform;
	class ViewportZoom;

	/**
	 * This class contains the current choice of MapCanvasTool.
	 *
	 * It also provides slots to choose the MapCanvasTool.
	 *
	 * This serves the role of the Context class in the State Pattern in Gamma et al.
	 */
	class MapCanvasToolChoice :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * Construct a MapCanvasToolChoice instance.
		 */
		MapCanvasToolChoice(
				GPlatesQtWidgets::MapCanvas &map_canvas,
				GPlatesQtWidgets::MapView &map_view,
				GPlatesQtWidgets::ViewportWindow &viewport_window,
				MapTransform &map_transform,
				ViewportZoom &viewport_zoom,
				const GPlatesCanvasTools::ClickGeometry::non_null_ptr_type &click_geometry_tool,
				const GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type &digitise_polyline_tool,
				const GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type &digitise_multipoint_tool,
				const GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type &digitise_polygon_tool,
				const GPlatesCanvasTools::MoveVertex::non_null_ptr_type &move_vertex_tool,
				const GPlatesCanvasTools::DeleteVertex::non_null_ptr_type &delete_vertex_tool,
				const GPlatesCanvasTools::InsertVertex::non_null_ptr_type &insert_vertex_tool,
				const GPlatesCanvasTools::SplitFeature::non_null_ptr_type &split_feature_tool,
				const GPlatesCanvasTools::ManipulatePole::non_null_ptr_type &manipulate_pole_tool,
				const GPlatesCanvasTools::BuildTopology::non_null_ptr_type &build_topology_tool,
				const GPlatesCanvasTools::EditTopology::non_null_ptr_type &edit_topology_tool,
				const GPlatesCanvasTools::MeasureDistance::non_null_ptr_type &measure_distance_tool,
				const GPlatesCanvasTools::CreateSmallCircle::non_null_ptr_type &create_small_circle_tool);

		~MapCanvasToolChoice();

		MapCanvasTool &
		tool_choice() const
		{
			return *d_tool_choice_ptr;
		}

	public slots:
		void
		choose_pan_map_tool()
		{
			change_tool_if_necessary(d_pan_map_tool_ptr.get());
		}

		void
		choose_zoom_map_tool()
		{
			change_tool_if_necessary(d_zoom_map_tool_ptr.get());
		}

		void
		choose_click_geometry_tool()
		{
			change_tool_if_necessary(d_click_geometry_tool_ptr.get());
		}

		void
		choose_digitise_polyline_tool()
		{
			change_tool_if_necessary(d_digitise_polyline_tool_ptr.get());
		}

		void
		choose_digitise_multipoint_tool()
		{
			change_tool_if_necessary(d_digitise_multipoint_tool_ptr.get());
		}

		void
		choose_digitise_polygon_tool()
		{
			change_tool_if_necessary(d_digitise_polygon_tool_ptr.get());
		}

		void
		choose_move_vertex_tool()
		{
			change_tool_if_necessary(d_move_vertex_tool_ptr.get());
		}

		void
		choose_insert_vertex_tool()
		{
			change_tool_if_necessary(d_insert_vertex_tool_ptr.get());
		}

		void
		choose_split_feature_tool()
		{
			change_tool_if_necessary(d_insert_vertex_tool_ptr.get());
		}

		void
		choose_delete_vertex_tool()
		{
			change_tool_if_necessary(d_delete_vertex_tool_ptr.get());
		}

#if 0
		void
		choose_move_geometry_tool()
		{
			change_tool_if_necessary(d_move_geometry_tool_ptr);
		}

#endif

		void
		choose_manipulate_pole_tool()
		{
			change_tool_if_necessary(d_manipulate_pole_tool_ptr.get());
		}

		void
		choose_build_topology_tool()
		{
			change_tool_if_necessary(d_build_topology_tool_ptr.get());
		}

		void
		choose_edit_topology_tool()
		{
			change_tool_if_necessary(d_edit_topology_tool_ptr.get());
		}

		void
		choose_measure_distance_tool()
		{
			change_tool_if_necessary(d_measure_distance_tool_ptr.get());
		}

		void
		choose_create_small_circle_tool()
		{
			change_tool_if_necessary(d_create_small_circle_tool_ptr.get());
		}

	private:
		/**
		 * This is the PanMap tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_pan_map_tool_ptr;

		/**
		 * This is the ZoomMap tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_zoom_map_tool_ptr;

		/**
		 * This is the MapClickGeometry tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_click_geometry_tool_ptr;

		/**
		 * This is the DigitiseGeometry (Polyline) tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_digitise_polyline_tool_ptr;

		/**
		 * This is the DigitiseGeometry (MultiPoint) tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_digitise_multipoint_tool_ptr;

		/**
		 * This is the DigitiseGeometry (Polygon) tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_digitise_polygon_tool_ptr;

		/**
		 * This is the MoveVertex tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_move_vertex_tool_ptr;

		/**
		* This is the DeleteVertex tool which the user may choose.
		*/
		boost::scoped_ptr<MapCanvasTool> d_delete_vertex_tool_ptr;

		/**
		* This is the InsertVertex tool which the user may choose.
		*/
		boost::scoped_ptr<MapCanvasTool> d_insert_vertex_tool_ptr;

#if 0
		/**
		 * This is the MoveGeometry tool which the user may choose.
		 */
		MapCanvasTool::non_null_ptr_type d_move_geometry_tool_ptr;

#endif
		/**
		 * This is the ManipulatePole tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_manipulate_pole_tool_ptr;

		/**
		 * This is the BuildTopology tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_build_topology_tool_ptr;

		/**
		 * This is the EditTopology tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_edit_topology_tool_ptr;

		/**
		 * This is the Measure Distance canvas tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_measure_distance_tool_ptr;

		/**
		 * This is the Create Small Circle canvas tool which the user may choose.
		 */
		boost::scoped_ptr<MapCanvasTool> d_create_small_circle_tool_ptr;

		/**
		 * The current choice of CanvasTool.
		 */
		MapCanvasTool *d_tool_choice_ptr;

		void
		change_tool_if_necessary(
				MapCanvasTool *new_tool_choice);
	};
}

#endif  // GPLATES_GUI_MAPCANVASTOOLCHOICE_H
