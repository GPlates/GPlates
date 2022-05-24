/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "MovePoleMap.h"

#include "gui/Map.h"

#include "presentation/ViewState.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::MovePoleMap::MovePoleMap(
		const GPlatesViewOperations::MovePoleOperation::non_null_ptr_type &move_pole_operation,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_) :
	MapCanvasTool(map_canvas_, viewport_window_.get_view_state().get_map_view_operation()),
	d_viewport_window_ptr(&viewport_window_),
	d_move_pole_operation(move_pole_operation),
	d_is_in_drag(false)
{
}


void
GPlatesCanvasTools::MovePoleMap::handle_activation()
{
	if (map_canvas().isVisible())
	{
		// Activate our MovePoleOperation.
		d_move_pole_operation->activate();

		d_viewport_window_ptr->status_message(QObject::tr(
				"Drag pole to move its location. "
				"Ctrl+drag to pan. "
				"Ctrl+Shift+drag to rotate/tilt."));
	}
}


void
GPlatesCanvasTools::MovePoleMap::handle_deactivation()
{
	if (map_canvas().isVisible())
	{
		// Deactivate our MovePoleOperation.
		d_move_pole_operation->deactivate();
	}
}


void
GPlatesCanvasTools::MovePoleMap::handle_left_drag(
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
		if (!d_is_in_drag)
		{
			d_move_pole_operation->start_drag_on_map(
					initial_map_position.get(),  // If initial_position_on_globe is valid then initial_map_position is too.
					initial_position_on_globe.get());

			d_is_in_drag = true;
		}

		d_move_pole_operation->update_drag(current_position_on_globe.get());
	}
}


void
GPlatesCanvasTools::MovePoleMap::handle_left_release_after_drag(
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
		// In case clicked and released at same time.
		if (!d_is_in_drag)
		{
			d_move_pole_operation->start_drag_on_map(
					initial_map_position.get(),  // If initial_position_on_globe is valid then initial_map_position is too.
					initial_position_on_globe.get());

			d_is_in_drag = true;
		}

		d_move_pole_operation->update_drag(current_position_on_globe.get());

		d_move_pole_operation->end_drag(current_position_on_globe.get());
		d_is_in_drag = false;
	}
}


void
GPlatesCanvasTools::MovePoleMap::handle_move_without_drag(
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
		d_move_pole_operation->mouse_move_on_map(
				map_position.get(),  // If position_on_globe is valid then map_position is too.
				position_on_globe.get());
	}
}
