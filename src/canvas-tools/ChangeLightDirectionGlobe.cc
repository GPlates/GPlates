/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "ChangeLightDirectionGlobe.h"

#include "gui/Globe.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryCollection.h"


GPlatesCanvasTools::ChangeLightDirectionGlobe::ChangeLightDirectionGlobe(
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesPresentation::ViewState &view_state_) :
	GlobeCanvasTool(globe_, globe_canvas_),
	d_viewport_window(&viewport_window_),
	d_change_light_direction_operation(
			view_state_.get_scene_lighting_parameters(),
			globe_.orientation(),
			view_state_.get_viewport_zoom(),
			rendered_geometry_collection,
			main_rendered_layer_type),
	d_is_in_drag(false)
{
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_activation()
{
	// Activate our ChangeLightDirectionOperation.
	d_change_light_direction_operation.activate();

	if (globe_canvas().isVisible())
	{
		d_viewport_window->status_message(QObject::tr(
				"Drag arrow to change the light direction."));
	}
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_deactivation()
{
	// Deactivate our ChangeLightDirectionOperation.
	d_change_light_direction_operation.deactivate();
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_left_drag(
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
		d_change_light_direction_operation.start_drag(
				oriented_initial_pos_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(initial_pos_on_globe));

		d_is_in_drag = true;
	}

	d_change_light_direction_operation.update_drag(oriented_current_pos_on_globe);
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_left_release_after_drag(
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

	// In case clicked and released at same time.
	handle_left_drag(
			initial_pos_on_globe,
			oriented_initial_pos_on_globe,
			was_on_globe,
			current_pos_on_globe,
			oriented_current_pos_on_globe,
			is_on_globe,
			oriented_centre_of_viewport);

	d_change_light_direction_operation.end_drag(oriented_current_pos_on_globe);
	d_is_in_drag = false;
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_ctrl_left_drag(
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

	// Delegate default reorient handling to base class.
	GPlatesGui::GlobeCanvasTool::handle_ctrl_left_drag(
			initial_pos_on_globe,
			oriented_initial_pos_on_globe,
			was_on_globe,
			current_pos_on_globe,
			oriented_current_pos_on_globe,
			is_on_globe,
			oriented_centre_of_viewport);

	// Make sure the light direction is in the correct location when it is attached to the
	// view frame (because the view re-orientation above will change things).
	d_change_light_direction_operation.mouse_move(
			oriented_current_pos_on_globe,
			globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe));
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_ctrl_left_release_after_drag(
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

	// Delegate default reorient handling to base class.
	GPlatesGui::GlobeCanvasTool::handle_ctrl_left_release_after_drag(
			initial_pos_on_globe,
			oriented_initial_pos_on_globe,
			was_on_globe,
			current_pos_on_globe,
			oriented_current_pos_on_globe,
			is_on_globe,
			oriented_centre_of_viewport);

	// Make sure the light direction is in the correct location when it is attached to the
	// view frame (because the view re-orientation above will change things).
	d_change_light_direction_operation.mouse_move(
			oriented_current_pos_on_globe,
			globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe));
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_shift_ctrl_left_drag(
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

	// Delegate default reorient handling to base class.
	GPlatesGui::GlobeCanvasTool::handle_shift_ctrl_left_drag(
			initial_pos_on_globe,
			oriented_initial_pos_on_globe,
			was_on_globe,
			current_pos_on_globe,
			oriented_current_pos_on_globe,
			is_on_globe,
			oriented_centre_of_viewport);

	// Make sure the light direction is in the correct location when it is attached to the
	// view frame (because the view re-orientation above will change things).
	d_change_light_direction_operation.mouse_move(
			oriented_current_pos_on_globe,
			globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe));
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_shift_ctrl_left_release_after_drag(
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

	// Delegate default reorient handling to base class.
	GPlatesGui::GlobeCanvasTool::handle_shift_ctrl_left_release_after_drag(
			initial_pos_on_globe,
			oriented_initial_pos_on_globe,
			was_on_globe,
			current_pos_on_globe,
			oriented_current_pos_on_globe,
			is_on_globe,
			oriented_centre_of_viewport);

	// Make sure the light direction is in the correct location when it is attached to the
	// view frame (because the view re-orientation above will change things).
	d_change_light_direction_operation.mouse_move(
			oriented_current_pos_on_globe,
			globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe));
}


void
GPlatesCanvasTools::ChangeLightDirectionGlobe::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (!globe_canvas().isVisible())
	{
		return;
	}

	d_change_light_direction_operation.mouse_move(
			oriented_current_pos_on_globe,
			globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe));
}
