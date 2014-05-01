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

#include "MovePoleGlobe.h"

#include "gui/Globe.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::MovePoleGlobe::MovePoleGlobe(
		const GPlatesViewOperations::MovePoleOperation::non_null_ptr_type &move_pole_operation,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_) :
	GlobeCanvasTool(globe_, globe_canvas_),
	d_viewport_window(&viewport_window_),
	d_move_pole_operation(move_pole_operation),
	d_is_in_drag(false)
{
}


void
GPlatesCanvasTools::MovePoleGlobe::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		// Activate our MovePoleOperation.
		d_move_pole_operation->activate();

		d_viewport_window->status_message(QObject::tr(
				"Drag arrow to move the pole location."));
	}
}


void
GPlatesCanvasTools::MovePoleGlobe::handle_deactivation()
{
	if (globe_canvas().isVisible())
	{
		// Deactivate our MovePoleOperation.
		d_move_pole_operation->deactivate();
	}
}


void
GPlatesCanvasTools::MovePoleGlobe::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (!globe_canvas().isVisible())
	{
		return;
	}

	if (!d_is_in_drag)
	{
		d_move_pole_operation->start_drag_on_globe(
				oriented_initial_pos_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(initial_pos_on_globe));

		d_is_in_drag = true;
	}

	d_move_pole_operation->update_drag(oriented_current_pos_on_globe);
}


void
GPlatesCanvasTools::MovePoleGlobe::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	// In case clicked and released at same time.
	handle_left_drag(
			initial_pos_on_globe,
			oriented_initial_pos_on_globe,
			was_on_globe,
			current_pos_on_globe,
			oriented_current_pos_on_globe,
			is_on_globe,
			oriented_centre_of_viewport);

	d_move_pole_operation->end_drag(oriented_current_pos_on_globe);
	d_is_in_drag = false;
}


void
GPlatesCanvasTools::MovePoleGlobe::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (!globe_canvas().isVisible())
	{
		return;
	}

	d_move_pole_operation->mouse_move_on_globe(
			oriented_current_pos_on_globe,
			globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe));
}
