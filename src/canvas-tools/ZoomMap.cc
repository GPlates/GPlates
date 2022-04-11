/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#include "ZoomMap.h"

#include "gui/MapCamera.h"
#include "gui/ViewportZoom.h"

#include "presentation/ViewState.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::ZoomMap::ZoomMap(
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_) :
	MapCanvasTool(map_canvas_, viewport_window_.get_view_state().get_map_view_operation()),
	d_viewport_window_ptr(&viewport_window_),
	d_view_state(viewport_window_.get_view_state())
{  }


void
GPlatesCanvasTools::ZoomMap::handle_activation()
{
	if (map_canvas().isVisible())
	{
		d_viewport_window_ptr->status_message(QObject::tr(
					"Click to zoom in."
					" Shift+click to zoom out."
					" Ctrl+drag to pan the map."));
	}
}


void
GPlatesCanvasTools::ZoomMap::handle_deactivation()
{
}


void
GPlatesCanvasTools::ZoomMap::recentre_map(
		const QPointF &map_position)
{
	d_view_state.get_map_camera().move_look_at_position(map_position);
}


void
GPlatesCanvasTools::ZoomMap::handle_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (click_position_on_globe)
	{
		recentre_map(click_map_position.get());  // If click_position_on_globe is valid then click_map_position is too.
		d_view_state.get_viewport_zoom().zoom_in();
	}
}


void
GPlatesCanvasTools::ZoomMap::handle_shift_left_click(
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
{
	if (click_position_on_globe)
	{
		recentre_map(click_map_position.get());  // If click_position_on_globe is valid then click_map_position is too.
		d_view_state.get_viewport_zoom().zoom_out();
	}
}
