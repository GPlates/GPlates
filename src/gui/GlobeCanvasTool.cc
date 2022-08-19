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

#include "qt-widgets/GlobeAndMapCanvas.h"

#include "view-operations/GlobeViewOperation.h"


GPlatesGui::GlobeCanvasTool::GlobeCanvasTool(
		GPlatesQtWidgets::GlobeAndMapCanvas &globe_canvas_,
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
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (globe_canvas().isVisible())
	{
		pan_globe_by_drag_update(
				screen_width, screen_height,
				initial_screen_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}


void
GPlatesGui::GlobeCanvasTool::handle_ctrl_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (globe_canvas().isVisible())
	{
		pan_globe_by_drag_release(
				screen_width, screen_height,
				initial_screen_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}


void
GPlatesGui::GlobeCanvasTool::handle_shift_ctrl_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (globe_canvas().isVisible())
	{
		rotate_and_tilt_globe_by_drag_update(
				screen_width, screen_height,
				initial_screen_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}


void
GPlatesGui::GlobeCanvasTool::handle_shift_ctrl_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (globe_canvas().isVisible())
	{
		rotate_and_tilt_globe_by_drag_release(
				screen_width, screen_height,
				initial_screen_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}


void
GPlatesGui::GlobeCanvasTool::pan_globe_by_drag_update(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_PAN,
				initial_position_on_globe,
				initial_screen_position,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_position_on_globe,
			current_screen_position,
			screen_width, screen_height,
			false/*end_of_drag*/);
}


void
GPlatesGui::GlobeCanvasTool::pan_globe_by_drag_release(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_PAN,
				initial_position_on_globe,
				initial_screen_position,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_position_on_globe,
			current_screen_position,
			screen_width, screen_height,
			true/*end_of_drag*/);
}


void
GPlatesGui::GlobeCanvasTool::rotate_and_tilt_globe_by_drag_update(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_ROTATE_AND_TILT,
				initial_position_on_globe,
				initial_screen_position,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_position_on_globe,
			current_screen_position,
			screen_width, screen_height,
			false/*end_of_drag*/);
}


void
GPlatesGui::GlobeCanvasTool::rotate_and_tilt_globe_by_drag_release(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_position_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_position_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (!d_globe_view_operation.in_drag())
	{
		d_globe_view_operation.start_drag(
				GPlatesViewOperations::GlobeViewOperation::DRAG_ROTATE_AND_TILT,
				initial_position_on_globe,
				initial_screen_position,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_position_on_globe,
			current_screen_position,
			screen_width, screen_height,
			true/*end_of_drag*/);
}
