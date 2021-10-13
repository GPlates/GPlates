/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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



namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
}

namespace GPlatesGui
{
	// Forward declarations.
	class Globe;
	class GlobeCanvasTool;

	/**
	 * This class contains the current choice of GlobeCanvasTool.
	 *
	 * It also provides slots to choose the GlobeCanvasTool.
	 *
	 * This serves the role of the Context class in the State Pattern in Gamma et al.
	 */
	class GlobeCanvasToolChoice :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * Construct a GlobeCanvasToolChoice instance.
		 */
		GlobeCanvasToolChoice(
				Globe &globe,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas,
				GPlatesQtWidgets::ViewportWindow &viewport_window,
				GPlatesPresentation::ViewState &view_state,
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

		~GlobeCanvasToolChoice();

		GlobeCanvasTool &
		tool_choice() const
		{
			return *d_tool_choice_ptr;
		}

	public slots:
		void
		choose_reorient_globe_tool()
		{
			change_tool_if_necessary(d_reorient_globe_tool_ptr.get());
		}

		void
		choose_zoom_globe_tool()
		{
			change_tool_if_necessary(d_zoom_globe_tool_ptr.get());
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
			change_tool_if_necessary(d_move_vertex_tool_ptr.get());
		}

		void
		choose_delete_vertex_tool()
		{
			change_tool_if_necessary(d_delete_vertex_tool_ptr.get());
		}

		void
		choose_insert_vertex_tool()
		{
			change_tool_if_necessary(d_insert_vertex_tool_ptr.get());
		}

		void
		choose_split_feature_tool()
		{
			change_tool_if_necessary(d_split_feature_tool_ptr.get());
		}

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
		 * This is the ReorientGlobe tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_reorient_globe_tool_ptr;

		/**
		 * This is the ZoomGlobe tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_zoom_globe_tool_ptr;

		/**
		 * This is the ClickGeometry tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_click_geometry_tool_ptr;

		/**
		 * This is the GlobeDigitiseGeometry (Polyline) tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_digitise_polyline_tool_ptr;

		/**
		 * This is the GlobeDigitiseGeometry (MultiPoint) tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_digitise_multipoint_tool_ptr;

		/**
		 * This is the GlobeDigitiseGeometry (Polygon) tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_digitise_polygon_tool_ptr;
#if 0
		/**
		 * This is the MoveGeometry tool which the user may choose.
		 */
		GlobeCanvasTool::non_null_ptr_type d_move_geometry_tool_ptr;
#endif
		/**
		 * This is the GlobeMoveVertex tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_move_vertex_tool_ptr;

		/**
		 * This is the DeleteVertex tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_delete_vertex_tool_ptr;

		/**
		 * This is the InsertVertex tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_insert_vertex_tool_ptr;

		/**
		 * This is the SplitFeature tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_split_feature_tool_ptr;

		/**
		 * This is the ManipulatePole tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_manipulate_pole_tool_ptr;

		/**
		 * This is the BuildTopology Canvas tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_build_topology_tool_ptr;

		/**
		 * This is the EditTopology Canvas tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_edit_topology_tool_ptr;

		/**
		 * This is the Measure Distance canvas tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_measure_distance_tool_ptr;

		/**
		 * This is the Create Small Circle canvas tool which the user may choose.
		 */
		boost::scoped_ptr<GlobeCanvasTool> d_create_small_circle_tool_ptr;

		/**
		 * The current choice of GlobeCanvasTool.
		 */
		GlobeCanvasTool *d_tool_choice_ptr;

		void
		change_tool_if_necessary(
				GlobeCanvasTool *new_tool_choice);
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOLCHOICE_H
