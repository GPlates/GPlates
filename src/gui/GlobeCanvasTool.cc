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

#include "Globe.h"

#include "maths/PointOnSphere.h"

#include "qt-widgets/GlobeCanvas.h"


GPlatesGui::GlobeCanvasTool::GlobeCanvasTool(
		Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesViewOperations::GlobeViewOperation &globe_view_operation_) :
	d_globe_ptr(&globe_),
	d_globe_canvas_ptr(&globe_canvas_),
	d_globe_view_operation(globe_view_operation_)
{  }


GPlatesGui::GlobeCanvasTool::~GlobeCanvasTool()
{  }


void
GPlatesGui::GlobeCanvasTool::reorient_globe_by_drag_update(
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
				GPlatesViewOperations::GlobeViewOperation::DRAG_NORMAL,
				initial_pos_on_globe,
				initial_screen_x, initial_screen_y,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_pos_on_globe,
			current_screen_x, current_screen_y,
			screen_width, screen_height);
}


void
GPlatesGui::GlobeCanvasTool::reorient_globe_by_drag_release(
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
	reorient_globe_by_drag_update(
			screen_width, screen_height,
			initial_screen_x, initial_screen_y,
			initial_pos_on_globe, was_on_globe,
			current_screen_x, current_screen_y,
			current_pos_on_globe, is_on_globe,
			centre_of_viewport);

	d_globe_view_operation.end_drag();
}


void
GPlatesGui::GlobeCanvasTool::rotate_globe_by_drag_update(
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
				GPlatesViewOperations::GlobeViewOperation::DRAG_ROTATE,
				initial_pos_on_globe,
				initial_screen_x, initial_screen_y,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_pos_on_globe,
			current_screen_x, current_screen_y,
			screen_width, screen_height);
}


void
GPlatesGui::GlobeCanvasTool::rotate_globe_by_drag_release(
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
	rotate_globe_by_drag_update(
			screen_width, screen_height,
			initial_screen_x, initial_screen_y,
			initial_pos_on_globe, was_on_globe,
			current_screen_x, current_screen_y,
			current_pos_on_globe, is_on_globe,
			centre_of_viewport);

	d_globe_view_operation.end_drag();
}


void
GPlatesGui::GlobeCanvasTool::tilt_globe_by_drag_update(
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
				GPlatesViewOperations::GlobeViewOperation::DRAG_TILT,
				initial_pos_on_globe,
				initial_screen_x, initial_screen_y,
				screen_width, screen_height);
	}

	d_globe_view_operation.update_drag(
			current_pos_on_globe,
			current_screen_x, current_screen_y,
			screen_width, screen_height);
}


void
GPlatesGui::GlobeCanvasTool::tilt_globe_by_drag_release(
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
	tilt_globe_by_drag_update(
			screen_width, screen_height,
			initial_screen_x, initial_screen_y,
			initial_pos_on_globe, was_on_globe,
			current_screen_x, current_screen_y,
			current_pos_on_globe, is_on_globe,
			centre_of_viewport);

	d_globe_view_operation.end_drag();
}
