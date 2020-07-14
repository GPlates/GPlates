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


#include "ReorientGlobe.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryCollection.h"


GPlatesCanvasTools::ReorientGlobe::ReorientGlobe(
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesQtWidgets::ViewportWindow &viewport_window_) :
	GlobeCanvasTool(globe_, globe_canvas_, viewport_window_.get_view_state().get_globe_view_operation()),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_viewport_window_ptr(&viewport_window_)
{
}

void
GPlatesCanvasTools::ReorientGlobe::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		d_viewport_window_ptr->status_message(QObject::tr(
			"Drag to re-orient the globe."
			" Shift+drag to rotate the globe."));
	}
}


void
GPlatesCanvasTools::ReorientGlobe::handle_deactivation()
{
}


void
GPlatesCanvasTools::ReorientGlobe::handle_left_drag(
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
}


void
GPlatesCanvasTools::ReorientGlobe::handle_left_release_after_drag(
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
	reorient_globe_by_drag_release(
			screen_width, screen_height,
			initial_screen_x, initial_screen_y,
			initial_pos_on_globe, was_on_globe,
			current_screen_x, current_screen_y,
			current_pos_on_globe, is_on_globe,
			centre_of_viewport);
}


void
GPlatesCanvasTools::ReorientGlobe::handle_shift_left_drag(
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
}


void
GPlatesCanvasTools::ReorientGlobe::handle_shift_left_release_after_drag(
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
	rotate_globe_by_drag_release(
			screen_width, screen_height,
			initial_screen_x, initial_screen_y,
			initial_pos_on_globe, was_on_globe,
			current_screen_x, current_screen_y,
			current_pos_on_globe, is_on_globe,
			centre_of_viewport);
}


void
GPlatesCanvasTools::ReorientGlobe::handle_alt_left_drag(
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
}


void
GPlatesCanvasTools::ReorientGlobe::handle_alt_left_release_after_drag(
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
	tilt_globe_by_drag_release(
			screen_width, screen_height,
			initial_screen_x, initial_screen_y,
			initial_pos_on_globe, was_on_globe,
			current_screen_x, current_screen_y,
			current_pos_on_globe, is_on_globe,
			centre_of_viewport);
}
