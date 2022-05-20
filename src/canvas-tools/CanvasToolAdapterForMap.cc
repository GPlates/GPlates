/* $Id$ */

/**
 * @file 
 * Contains implementation of CanvasToolAdapterForMap
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

#include "CanvasToolAdapterForMap.h"

#include "qt-widgets/MapCanvas.h"


GPlatesCanvasTools::CanvasToolAdapterForMap::CanvasToolAdapterForMap(
		const CanvasTool::non_null_ptr_type &canvas_tool_ptr,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesViewOperations::MapViewOperation &map_view_operation_) :
	MapCanvasTool(map_canvas_, map_view_operation_),
	d_canvas_tool_ptr(canvas_tool_ptr)
{  }


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_activation()
{
	if (map_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_activation();
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_deactivation()
{
	if (map_canvas().isVisible()) // Avoid deactivating twice (in globe and map adaptor)
	{
		d_canvas_tool_ptr->handle_deactivation();
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_press(
		int screen_width,
		int screen_height,
		const QPointF &press_screen_position,
		const boost::optional<QPointF> &press_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &press_position_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		press_position_on_globe)
	{
		d_canvas_tool_ptr->handle_left_press(
				press_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(press_position_on_globe.get()));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		click_position_on_globe)
	{
		d_canvas_tool_ptr->handle_left_click(
				click_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(click_position_on_globe.get()));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		initial_position_on_globe && current_position_on_globe)
	{
		d_canvas_tool_ptr->handle_left_drag(
				initial_position_on_globe.get(),
				true/*was_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(initial_position_on_globe.get()),
				current_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(current_position_on_globe.get()),
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		initial_position_on_globe && current_position_on_globe)
	{
		d_canvas_tool_ptr->handle_left_release_after_drag(
				initial_position_on_globe.get(),
				true/*was_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(initial_position_on_globe.get()),
				current_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(current_position_on_globe.get()),
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		click_position_on_globe)
	{
		d_canvas_tool_ptr->handle_shift_left_click(
				click_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(click_position_on_globe.get()));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		initial_position_on_globe && current_position_on_globe)
	{
		d_canvas_tool_ptr->handle_shift_left_drag(
				initial_position_on_globe.get(),
				true/*was_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(initial_position_on_globe.get()),
				current_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(current_position_on_globe.get()),
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		initial_position_on_globe && current_position_on_globe)
	{
		d_canvas_tool_ptr->handle_shift_left_release_after_drag(
				initial_position_on_globe.get(),
				true/*was_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(initial_position_on_globe.get()),
				current_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(current_position_on_globe.get()),
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_alt_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		click_position_on_globe)
	{
		d_canvas_tool_ptr->handle_alt_left_click(
				click_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(click_position_on_globe.get()));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_alt_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		initial_position_on_globe && current_position_on_globe)
	{
		d_canvas_tool_ptr->handle_alt_left_drag(
				initial_position_on_globe.get(),
				true/*was_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(initial_position_on_globe.get()),
				current_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(current_position_on_globe.get()),
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_alt_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		initial_position_on_globe && current_position_on_globe)
	{
		d_canvas_tool_ptr->handle_alt_left_release_after_drag(
				initial_position_on_globe.get(),
				true/*was_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(initial_position_on_globe.get()),
				current_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(current_position_on_globe.get()),
				centre_of_viewport_on_globe);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_ctrl_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		click_position_on_globe)
	{
		d_canvas_tool_ptr->handle_ctrl_left_click(
				click_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(click_position_on_globe.get()));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_ctrl_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	// Perform default action.
	MapCanvasTool::handle_ctrl_left_drag(
			screen_width, screen_height,
			initial_screen_position, initial_map_position, initial_position_on_globe,
			current_screen_position, current_map_position, current_position_on_globe,
			centre_of_viewport_on_globe);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_ctrl_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	// Perform default action.
	MapCanvasTool::handle_ctrl_left_release_after_drag(
			screen_width, screen_height,
			initial_screen_position, initial_map_position, initial_position_on_globe,
			current_screen_position, current_map_position, current_position_on_globe,
			centre_of_viewport_on_globe);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_ctrl_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		click_position_on_globe)
	{
		d_canvas_tool_ptr->handle_shift_ctrl_left_click(
				click_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(click_position_on_globe.get()));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_ctrl_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	// Perform default action.
	MapCanvasTool::handle_shift_ctrl_left_drag(
			screen_width, screen_height,
			initial_screen_position, initial_map_position, initial_position_on_globe,
			current_screen_position, current_map_position, current_position_on_globe,
			centre_of_viewport_on_globe);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_ctrl_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	// Perform default action.
	MapCanvasTool::handle_shift_ctrl_left_release_after_drag(
			screen_width, screen_height,
			initial_screen_position, initial_map_position, initial_position_on_globe,
			current_screen_position, current_map_position, current_position_on_globe,
			centre_of_viewport_on_globe);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_alt_ctrl_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		click_position_on_globe)
	{
		d_canvas_tool_ptr->handle_alt_ctrl_left_click(
				click_position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(click_position_on_globe.get()));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_alt_ctrl_left_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	// Perform default action.
	MapCanvasTool::handle_alt_ctrl_left_drag(
			screen_width, screen_height,
			initial_screen_position, initial_map_position, initial_position_on_globe,
			current_screen_position, current_map_position, current_position_on_globe,
			centre_of_viewport_on_globe);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_alt_ctrl_left_release_after_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	// Perform default action.
	MapCanvasTool::handle_alt_ctrl_left_release_after_drag(
			screen_width, screen_height,
			initial_screen_position, initial_map_position, initial_position_on_globe,
			current_screen_position, current_map_position, current_position_on_globe,
			centre_of_viewport_on_globe);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_move_without_drag(
		int screen_width,
		int screen_height,
		const QPointF &screen_position,
		const boost::optional<QPointF> &map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (map_canvas().isVisible() &&
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap to the edge of the map,
		// much like how it snaps to the horizon of the globe if you click outside of the globe...
		position_on_globe)
	{
		d_canvas_tool_ptr->handle_move_without_drag(
				position_on_globe.get(),
				true/*is_on_earth*/,
				map_canvas().current_proximity_inclusion_threshold(position_on_globe.get()));
	}
}
