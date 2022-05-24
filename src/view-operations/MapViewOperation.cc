/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2022 The University of Sydney, Australia
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

#include "MapViewOperation.h"

#include "RenderedGeometryFactory.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/MapCamera.h"

#include "maths/MathsUtils.h"


GPlatesViewOperations::MapViewOperation::MapViewOperation(
		GPlatesGui::MapCamera &map_camera,
		RenderedGeometryCollection &rendered_geometry_collection) :
	d_map_camera(map_camera),
	d_in_drag_operation(false),
	d_in_last_update_drag(false),
	d_rendered_geometry_collection(rendered_geometry_collection)
{
}


void
GPlatesViewOperations::MapViewOperation::start_drag(
		MouseDragMode mouse_drag_mode,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		int screen_width,
		int screen_height)
{
	// We've started a drag operation.
	d_in_drag_operation = true;
	d_in_last_update_drag = false;

	// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
	const double initial_mouse_window_y = screen_height - initial_screen_position.y();
	const double initial_mouse_window_x = initial_screen_position.x();

	d_mouse_drag_info = MouseDragInfo(
			mouse_drag_mode,
			initial_mouse_window_x,
			initial_mouse_window_y,
			initial_map_position,
			d_map_camera.get_look_at_position_on_map(),
			d_map_camera.get_view_direction(),
			d_map_camera.get_up_direction(),
			d_map_camera.get_pan(),
			d_map_camera.get_rotation_angle(),
			d_map_camera.get_tilt_angle());

	switch (d_mouse_drag_info->mode)
	{
	case DRAG_PAN:
		start_drag_pan();
		break;
	case DRAG_ROTATE_AND_TILT:
		start_drag_rotate_and_tilt();
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


void
GPlatesViewOperations::MapViewOperation::update_drag(
		const QPointF &screen_position,
		const boost::optional<QPointF> &map_position,
		int screen_width,
		int screen_height,
		bool end_of_drag)
{
	// If we're finishing the drag operation.
	if (end_of_drag)
	{
		// Set to false so that when clients call 'in_drag()' it will return false.
		//
		// It's important to do this at the start because this function can update
		// the map camera which in turn signals the map to be rendered which in turn
		// queries 'in_drag()' to see if it should optimise rendering *during* a mouse drag.
		// And that all happens before we even leave the current function.
		d_in_drag_operation = false;

		d_in_last_update_drag = true;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
	const double mouse_window_y = screen_height - screen_position.y();
	const double mouse_window_x = screen_position.x();

	switch (d_mouse_drag_info->mode)
	{
	case DRAG_PAN:
		update_drag_pan(map_position);
		break;
	case DRAG_ROTATE_AND_TILT:
		update_drag_rotate_and_tilt(mouse_window_x, mouse_window_y, screen_width, screen_height);
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// If we've finished the drag operation.
	if (end_of_drag)
	{
		// Finished dragging mouse - no need for mouse drag info.
		d_mouse_drag_info = boost::none;

		d_in_last_update_drag = false;
	}
}


void
GPlatesViewOperations::MapViewOperation::start_drag_pan()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	//
	// Nothing to be done.
	//
}


void
GPlatesViewOperations::MapViewOperation::update_drag_pan(
		const boost::optional<QPointF> &map_position)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// If the mouse position at start of drag, or current mouse position, is not on the map plane
	// then we cannot pan the view.
	if (!d_mouse_drag_info->start_map_position ||
		!map_position)
	{
		return;
	}

	// The current mouse position-on-map is in global (universe) coordinates.
	// It actually doesn't change (within numerical precision) when the view pans.
	// However, in the frame-of-reference of the view at the start of drag, it has changed.
	// To detect how much change we need to pan it by the reverse of the change in view frame
	// (it's reverse because a change in view space is equivalent to the reverse change in model space
	// and the map, and points on it, are in model space).
	const QPointF map_position_relative_to_start_view =
			-d_mouse_drag_info->pan_relative_to_start_in_view_frame + map_position.get();

	// The pan since start of drag in the map frame of reference.
	const QPointF pan_relative_to_start_in_map_frame =
			map_position_relative_to_start_view - d_mouse_drag_info->start_map_position.get();

	// Convert the pan to the view frame of reference.
	// The view moves in the opposite direction.
	const QPointF pan_relative_to_start_in_view_frame = -pan_relative_to_start_in_map_frame;

	// The total pan is the pan at the start of drag plus the pan relative to that start.
	const QPointF pan = d_mouse_drag_info->start_pan + pan_relative_to_start_in_view_frame;

	// Keep track of the updated view pan relative to the start.
	d_mouse_drag_info->pan_relative_to_start_in_view_frame = pan_relative_to_start_in_view_frame;

	d_map_camera.set_pan(
			pan,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);
}


void
GPlatesViewOperations::MapViewOperation::start_drag_rotate_and_tilt()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// Create a child rendered geometry layer to render centre of viewport (when rotating/tilting).
	//
	// We store the returned object as a data member and it automatically destroys the created layer
	// for us when 'this' object is destroyed.
	d_rotate_and_tilt_rendered_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				RenderedGeometryCollection::PAN_ROTATE_TILT_IN_ALL_CANVAS_TOOLS_LAYER);

	// Activate our render layer (and its parent/main layer) so it becomes visible.
	//
	// Note: The parent/main layer is not specific to a particular canvas tool workflow.
	//       This is because rotating and tilting are available in ALL canvas tools.
	d_rendered_geometry_collection.set_main_layer_active(RenderedGeometryCollection::PAN_ROTATE_TILT_IN_ALL_CANVAS_TOOLS_LAYER);
	d_rotate_and_tilt_rendered_layer_ptr->set_active(true);

	// Render a small circle at the viewport centre which is the camera look-at position.
	// This is so the user can see the camera look-at position used for rotating tilting around.
	const RenderedGeometry rotate_and_tilt_arrow_rendered_geom =
			RenderedGeometryFactory::create_rendered_circle_symbol(
					d_map_camera.get_look_at_position_on_globe(),
					GPlatesGui::Colour::get_silver(),
					2.0f/*size*/,
					false/*filled*/,
					1.0f/*line_width_hint*/);
	d_rotate_and_tilt_rendered_layer_ptr->add_rendered_geometry(rotate_and_tilt_arrow_rendered_geom);
}


void
GPlatesViewOperations::MapViewOperation::update_drag_rotate_and_tilt(
		double mouse_window_x,
		double mouse_window_y,
		int window_width,
		int window_height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	//
	// Horizontal dragging rotates the view.
	//

	// Each multiple of PI means dragging the full window *width* will *rotate* by PI radians (180 degrees).
	const double delta_rotation_angle = 3 * GPlatesMaths::PI * (d_mouse_drag_info->start_mouse_window_x - mouse_window_x) / window_width;

	const GPlatesMaths::real_t rotation_angle = d_mouse_drag_info->start_rotation_angle + delta_rotation_angle;

	d_map_camera.set_rotation_angle(
			rotation_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	//
	// Vertical dragging tilts the view.
	//

	// Each multiple of PI means dragging the full window *height* will *tilt* by PI radians (180 degrees).
	const double delta_tilt_angle = 1.5 * GPlatesMaths::PI * (mouse_window_y - d_mouse_drag_info->start_mouse_window_y) / window_height;

	const GPlatesMaths::real_t tilt_angle = d_mouse_drag_info->start_tilt_angle + delta_tilt_angle;

	d_map_camera.set_tilt_angle(
			tilt_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	// If at end of drag then clear/deactivate our rotate/tilt rendered geometry layer.
	if (d_in_last_update_drag)
	{
		d_rendered_geometry_collection.set_main_layer_active(
				RenderedGeometryCollection::PAN_ROTATE_TILT_IN_ALL_CANVAS_TOOLS_LAYER,
				false);
		d_rotate_and_tilt_rendered_layer_ptr->set_active(false);
		d_rotate_and_tilt_rendered_layer_ptr->clear_rendered_geometries();
	}
}
