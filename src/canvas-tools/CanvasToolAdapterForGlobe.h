/* $Id$ */

/**
 * @file 
 * Contains definition of CanvasToolAdapterForGlobe
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

#ifndef GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H
#define GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H

#include "CanvasTool.h"

#include "gui/GlobeCanvasTool.h"


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
}

namespace GPlatesViewOperations
{
	class GlobeViewOperation;
}

namespace GPlatesCanvasTools
{

	/**
	 * Adapter class for CanvasTool -> GlobeCanvasTool
	 */
	class CanvasToolAdapterForGlobe:
			public GPlatesGui::GlobeCanvasTool
	{

	public:

		/**
		 * Create a CanvasToolAdapterForGlobe instance.
		 */
		CanvasToolAdapterForGlobe(
				const CanvasTool::non_null_ptr_type &canvas_tool_ptr,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesViewOperations::GlobeViewOperation &globe_view_operation_);
		

		void
		handle_activation() override;

		void
		handle_deactivation() override;

		void
		handle_left_press(
				int screen_width,
				int screen_height,
				double press_screen_x,
				double press_screen_y,
				const GPlatesMaths::PointOnSphere &press_pos_on_globe,
				bool is_on_globe) override;


		void
		handle_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

		void
		handle_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_shift_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

		void
		handle_shift_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_shift_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_alt_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

		void
		handle_alt_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_alt_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_ctrl_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

		void
		handle_ctrl_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_shift_ctrl_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

		void
		handle_shift_ctrl_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_shift_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_alt_ctrl_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

		void
		handle_alt_ctrl_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_alt_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &_centre_of_viewport) override;
		
	private:

		//! A pointer to the CanvasTool instance that we wrap around
		CanvasTool::non_null_ptr_type d_canvas_tool_ptr;
	};
}

#endif  // GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H
