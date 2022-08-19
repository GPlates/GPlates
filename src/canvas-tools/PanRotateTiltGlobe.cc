/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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


#include "PanRotateTiltGlobe.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapCanvas.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::PanRotateTiltGlobe::PanRotateTiltGlobe(
		GPlatesQtWidgets::GlobeAndMapCanvas &globe_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_) :
	GlobeCanvasTool(globe_canvas_, viewport_window_.get_view_state().get_globe_view_operation()),
	d_viewport_window_ptr(&viewport_window_)
{
}

void
GPlatesCanvasTools::PanRotateTiltGlobe::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		d_viewport_window_ptr->status_message(QObject::tr(
				"Drag to pan. "
				"Shift+drag to rotate/tilt."));
	}
}


void
GPlatesCanvasTools::PanRotateTiltGlobe::handle_deactivation()
{
}


void
GPlatesCanvasTools::PanRotateTiltGlobe::handle_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		pan_globe_by_drag_update(
				screen_width, screen_height,
				initial_screen_position,
				initial_pos_on_globe, was_on_globe,
				current_screen_position,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}


void
GPlatesCanvasTools::PanRotateTiltGlobe::handle_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		pan_globe_by_drag_release(
				screen_width, screen_height,
				initial_screen_position,
				initial_pos_on_globe, was_on_globe,
				current_screen_position,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}


void
GPlatesCanvasTools::PanRotateTiltGlobe::handle_shift_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		rotate_and_tilt_globe_by_drag_update(
				screen_width, screen_height,
				initial_screen_position,
				initial_pos_on_globe, was_on_globe,
				current_screen_position,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}


void
GPlatesCanvasTools::PanRotateTiltGlobe::handle_shift_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		bool was_on_globe,
		const QPointF &current_screen_position,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		rotate_and_tilt_globe_by_drag_release(
				screen_width, screen_height,
				initial_screen_position,
				initial_pos_on_globe, was_on_globe,
				current_screen_position,
				current_pos_on_globe, is_on_globe,
				centre_of_viewport);
	}
}
