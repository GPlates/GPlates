/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "GlobeCanvasTool.h"

#include "qt-widgets/GlobeCanvas.h"

#include "view-operations/GlobeViewOperation.h"


GPlatesGui::GlobeCanvasTool::GlobeCanvasTool(
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesViewOperations::GlobeViewOperation &globe_view_operation_) :
	d_globe_canvas(globe_canvas_),
	d_globe_view_operation(globe_view_operation_)
{  }


GPlatesGui::GlobeCanvasTool::~GlobeCanvasTool()
{  }


void
GPlatesGui::GlobeCanvasTool::handle_ctrl_left_drag(
		int screen_width,
		int screen_height,
		double initial_screen_x,
		double initial_screen_y,
		const GPlatesMaths::PointOnSphere& initial_pos_on_globe,
		bool was_on_globe,
		double current_screen_x,
		double current_screen_y,
		const GPlatesMaths::PointOnSphere& current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere& centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		pan_globe_by_drag_update(
				screen_width, screen_height,
				initial_screen_x, initial_screen_y,
				initial_pos_on_globe, was_on_globe,
				current_screen_x, current_screen_y,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}


void
GPlatesGui::GlobeCanvasTool::handle_ctrl_left_release_after_drag(
		int screen_width,
		int screen_height,
		double initial_screen_x,
		double initial_screen_y,
		const GPlatesMaths::PointOnSphere& initial_pos_on_globe,
		bool was_on_globe,
		double current_screen_x,
		double current_screen_y,
		const GPlatesMaths::PointOnSphere& current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere& centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		pan_globe_by_drag_release(
				screen_width, screen_height,
				initial_screen_x, initial_screen_y,
				initial_pos_on_globe, was_on_globe,
				current_screen_x, current_screen_y,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}


void
GPlatesGui::GlobeCanvasTool::handle_shift_ctrl_left_drag(
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
		rotate_and_tilt_globe_by_drag_update(
				screen_width, screen_height,
				initial_screen_x, initial_screen_y,
				initial_pos_on_globe, was_on_globe,
				current_screen_x, current_screen_y,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}


void
GPlatesGui::GlobeCanvasTool::handle_shift_ctrl_left_release_after_drag(
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
		rotate_and_tilt_globe_by_drag_release(
				screen_width, screen_height,
				initial_screen_x, initial_screen_y,
				initial_pos_on_globe, was_on_globe,
				current_screen_x, current_screen_y,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}


void
GPlatesGui::GlobeCanvasTool::pan_globe_by_drag_update(
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
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_PAN,
				initial_pos_on_globe,
				initial_screen_x, initial_screen_y,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_pos_on_globe,
			current_screen_x, current_screen_y,
			screen_width, screen_height,
			false/*end_of_drag*/);
}


void
GPlatesGui::GlobeCanvasTool::pan_globe_by_drag_release(
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
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_PAN,
				initial_pos_on_globe,
				initial_screen_x, initial_screen_y,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_pos_on_globe,
			current_screen_x, current_screen_y,
			screen_width, screen_height,
			true/*end_of_drag*/);
}


void
GPlatesGui::GlobeCanvasTool::rotate_and_tilt_globe_by_drag_update(
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
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_ROTATE_AND_TILT,
				initial_pos_on_globe,
				initial_screen_x, initial_screen_y,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_pos_on_globe,
			current_screen_x, current_screen_y,
			screen_width, screen_height,
			false/*end_of_drag*/);
}


void
GPlatesGui::GlobeCanvasTool::rotate_and_tilt_globe_by_drag_release(
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
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_ROTATE_AND_TILT,
				initial_pos_on_globe,
				initial_screen_x, initial_screen_y,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_pos_on_globe,
			current_screen_x, current_screen_y,
			screen_width, screen_height,
			true/*end_of_drag*/);
}
