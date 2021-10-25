/* $Id$ */

/**
 * @file 
 * Contains definition of CanvasToolAdapterForMap
 *
 * $Revision$
 * $Date$ 
 * 
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

#ifndef GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORMAP_H
#define GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORMAP_H

#include <boost/optional.hpp>
#include <QPointF>
#include <QString>

#include "CanvasTool.h"

#include "gui/MapCanvasTool.h"

#include "maths/PointOnSphere.h"


namespace GPlatesGui
{
	class MapProjection;
	class MapTransform;
}

namespace GPlatesQtWidgets
{
	class MapCanvas;
	class MapView;
}

namespace GPlatesCanvasTools
{

	/**
	 * Adapter class for CanvasTool -> MapCanvasTool
	 */
	class CanvasToolAdapterForMap:
			public GPlatesGui::MapCanvasTool
	{

	public:

		/**
		 * Create a CanvasToolAdapterForMap instance.
		 */
		CanvasToolAdapterForMap(
				const CanvasTool::non_null_ptr_type &canvas_tool_ptr,
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				GPlatesGui::MapTransform &map_transform_);
		
		virtual
		void
		handle_activation();

		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_press(
				const QPointF &click_point_on_scene,
				bool is_on_surface);


		virtual
		void
		handle_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_shift_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_shift_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_shift_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_ctrl_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_ctrl_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_ctrl_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_shift_ctrl_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_shift_ctrl_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

/* FIXME: commented out because it is commented out in MapCanvasTool.h
		virtual
		void
		handle_shift_ctrl_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);
*/

		virtual
		void
		handle_move_without_drag(
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);
	
	private:

		/**
		 * Typedef for a pointer to a member function of CanvasTool used for clicks
		 * and moves without drag.
		 */
		typedef void (CanvasTool::*canvas_tool_click_func)
			(const GPlatesMaths::PointOnSphere &, bool, double);

		/**
		 * Typedef for a pointer to a member function of CanvasTool used for drag
		 * operations that do not have a default action.
		 */
		typedef void (CanvasTool::*canvas_tool_drag_func_without_default)
			(const GPlatesMaths::PointOnSphere &, bool, double,
			 const GPlatesMaths::PointOnSphere &, bool, double,
			 const boost::optional<GPlatesMaths::PointOnSphere> &);

		/**
		 * Typedef for a pointer to a member function of CanvasTool used for drag
		 * operations that have a default action.
		 */
		typedef bool (CanvasTool::*canvas_tool_drag_func_with_default)
			(const GPlatesMaths::PointOnSphere &, bool, double,
			 const GPlatesMaths::PointOnSphere &, bool, double,
			 const boost::optional<GPlatesMaths::PointOnSphere> &);

		void
		invoke_canvas_tool_func(
				const QPointF &click_point_on_scene,
				bool is_on_surface,
				const canvas_tool_click_func &func);

		void
		invoke_canvas_tool_func(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const canvas_tool_drag_func_without_default &func);

		bool
		invoke_canvas_tool_func(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const canvas_tool_drag_func_with_default &func);
				

		//! A pointer to the CanvasTool instance that we wrap around
		CanvasTool::non_null_ptr_type d_canvas_tool_ptr;
	};
}

#endif  // GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H
