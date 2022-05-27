/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2010 The University of Sydney, Australia
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

#include "ZoomGlobe.h"

#include "gui/GlobeCamera.h"
#include "gui/ViewportZoom.h"

#include "maths/LatLonPoint.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::ZoomGlobe::ZoomGlobe(
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_) :
	GlobeCanvasTool(globe_canvas_, viewport_window_.get_view_state().get_globe_view_operation()),
	d_viewport_window(&viewport_window_),
	d_view_state(viewport_window_.get_view_state())
{
}


void
GPlatesCanvasTools::ZoomGlobe::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		d_viewport_window->status_message(QObject::tr(
				"Click to zoom in. "
				"Shift+click to zoom out. "
				"Ctrl+drag to pan. "
				"Ctrl+Shift+drag to rotate/tilt."));
	}
}


void
GPlatesCanvasTools::ZoomGlobe::handle_deactivation()
{
}


void
GPlatesCanvasTools::ZoomGlobe::recentre_globe(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe)
{
	d_view_state.get_globe_camera().move_look_at_position_on_globe(click_pos_on_globe);
}


void
GPlatesCanvasTools::ZoomGlobe::handle_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	recentre_globe(click_pos_on_globe);
	d_view_state.get_viewport_zoom().zoom_in();
}


void
GPlatesCanvasTools::ZoomGlobe::handle_shift_left_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		bool is_on_globe)
{
	recentre_globe(click_pos_on_globe);
	d_view_state.get_viewport_zoom().zoom_out();
}
