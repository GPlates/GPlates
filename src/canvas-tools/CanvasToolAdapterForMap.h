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

#include "CanvasTool.h"

#include "gui/MapCanvasTool.h"


namespace GPlatesQtWidgets
{
	class GlobeAndMapCanvas;
}

namespace  GPlatesViewOperations
{
	class MapViewOperation;
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
				GPlatesQtWidgets::GlobeAndMapCanvas &map_canvas_,
				GPlatesViewOperations::MapViewOperation &map_view_operation_);


		void
		handle_activation() override;

		void
		handle_deactivation() override;

		void
		handle_left_press(
				int screen_width,
				int screen_height,
				const QPointF &press_screen_position,
				const boost::optional<QPointF> &press_map_position,
				const GPlatesMaths::PointOnSphere &press_position_on_globe,
				bool is_on_globe) override;

		void
		handle_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;

		void
		handle_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_shift_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;

		void
		handle_shift_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_shift_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_alt_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;

		void
		handle_alt_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_alt_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;

		void
		handle_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_shift_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;

		void
		handle_shift_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_shift_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_alt_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;

		void
		handle_alt_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_alt_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				const QPointF &screen_position,
				const boost::optional<QPointF> &map_position,
				const GPlatesMaths::PointOnSphere &position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;
	
	private:
		//! A pointer to the CanvasTool instance that we wrap.
		CanvasTool::non_null_ptr_type d_canvas_tool_ptr;
	};
}

#endif  // GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H
