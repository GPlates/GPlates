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

#include "PanRotateTiltMap.h"

#include "presentation/ViewState.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::PanRotateTiltMap::PanRotateTiltMap(
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_):
	MapCanvasTool(map_canvas_, viewport_window_.get_view_state().get_map_view_operation()),
	d_viewport_window_ptr(&viewport_window_)
{
}


void
GPlatesCanvasTools::PanRotateTiltMap::handle_activation()
{
	if (map_canvas().isVisible())
	{
		d_viewport_window_ptr->status_message(QObject::tr(
				"Drag to pan. "
				"Shift+drag to rotate/tilt."));
	}
}


void
GPlatesCanvasTools::PanRotateTiltMap::handle_deactivation()
{
}


void
GPlatesCanvasTools::PanRotateTiltMap::handle_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible())
	{
		pan_map_by_drag_update(
				screen_width, screen_height,
				initial_screen_position, initial_map_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_map_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::PanRotateTiltMap::handle_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible())
	{
		pan_map_by_drag_release(
				screen_width, screen_height,
				initial_screen_position, initial_map_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_map_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::PanRotateTiltMap::handle_shift_left_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible())
	{
		rotate_and_tilt_map_by_drag_update(
				screen_width, screen_height,
				initial_screen_position, initial_map_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_map_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::PanRotateTiltMap::handle_shift_left_release_after_drag(
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
		const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible())
	{
		rotate_and_tilt_map_by_drag_release(
				screen_width, screen_height,
				initial_screen_position, initial_map_position, initial_position_on_globe, was_on_globe,
				current_screen_position, current_map_position, current_position_on_globe, is_on_globe,
				centre_of_viewport_on_globe);
	}
}
