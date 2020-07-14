/* $Id$ */

/**
 * @file 
 * Contains implementation of CanvasToolAdapterForGlobe
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

#include "CanvasToolAdapterForGlobe.h"

#include "maths/PointOnSphere.h"

#include "qt-widgets/GlobeCanvas.h"


GPlatesCanvasTools::CanvasToolAdapterForGlobe::CanvasToolAdapterForGlobe (
		const CanvasTool::non_null_ptr_type &canvas_tool_ptr,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesViewOperations::GlobeViewOperation &globe_view_operation_) :
	GlobeCanvasTool(globe_, globe_canvas_, globe_view_operation_),
	d_canvas_tool_ptr(canvas_tool_ptr)
{  }

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_activation();
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_deactivation()
{
	if (globe_canvas().isVisible()) // Avoid deactivating twice (in globe and map adaptor)
	{
		d_canvas_tool_ptr->handle_deactivation();
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_press(
		int screen_width,
		int screen_height,
		double press_screen_x,
		double press_screen_y,
		const GPlatesMaths::PointOnSphere &press_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_press(
				press_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					press_pos_on_globe));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_click(
				click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_release_after_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_left_click(
				click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_left_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_left_release_after_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_alt_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_alt_left_click(
				click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_alt_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_alt_left_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_alt_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_alt_left_release_after_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_ctrl_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_ctrl_left_click(
				click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_ctrl_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_ctrl_left_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_ctrl_left_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_ctrl_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_ctrl_left_release_after_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_ctrl_left_release_after_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_ctrl_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_ctrl_left_click(
				click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_ctrl_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_shift_ctrl_left_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_shift_ctrl_left_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_ctrl_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_shift_ctrl_left_release_after_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_shift_ctrl_left_release_after_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_alt_ctrl_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_alt_ctrl_left_click(
				click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_alt_ctrl_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_alt_ctrl_left_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_alt_ctrl_left_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_alt_ctrl_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_alt_ctrl_left_release_after_drag(
				initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_alt_ctrl_left_release_after_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_move_without_drag(
		int screen_width,
		int screen_height,
		double current_screen_x,
		double current_screen_y,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_move_without_drag(
				current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe));
	}
}

