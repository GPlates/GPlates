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
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_mouse_drag_mode && !d_pan_drag_info && !d_rotate_and_tilt_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// We've started a drag operation.
	d_mouse_drag_mode = mouse_drag_mode;
	d_in_last_update_drag = false;

	// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
	const double initial_mouse_window_y = screen_height - initial_screen_position.y();
	const double initial_mouse_window_x = initial_screen_position.x();

	switch (mouse_drag_mode)
	{
	case DRAG_PAN:
		start_drag_pan(initial_map_position, initial_mouse_window_x, initial_mouse_window_y);
		break;
	case DRAG_ROTATE_AND_TILT:
		start_drag_rotate_and_tilt(initial_mouse_window_x, initial_mouse_window_y);
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	//
	// Setup for rendering the camera look-at position (only during drag).
	//

	// Create a child rendered geometry layer to render centre of viewport (when panning/rotating/tilting).
	//
	// We store the returned object as a data member and it automatically destroys the created layer
	// for us when 'this' object is destroyed.
	d_rendered_layer_ptr = d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
			RenderedGeometryCollection::PAN_ROTATE_TILT_IN_ALL_CANVAS_TOOLS_LAYER);
	// Activate our render layer (and its parent/main layer) so it becomes visible.
	//
	// Note: The parent/main layer is not specific to a particular canvas tool workflow.
	//       This is because rotating and tilting are available in ALL canvas tools.
	d_rendered_geometry_collection.set_main_layer_active(RenderedGeometryCollection::PAN_ROTATE_TILT_IN_ALL_CANVAS_TOOLS_LAYER);
	d_rendered_layer_ptr->set_active(true);
}


void
GPlatesViewOperations::MapViewOperation::update_drag(
		const QPointF &screen_position,
		const boost::optional<QPointF> &map_position,
		int screen_width,
		int screen_height,
		bool end_of_drag)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_mode,
			GPLATES_ASSERTION_SOURCE);
	const MouseDragMode mouse_drag_mode = d_mouse_drag_mode.get();

	// If we're finishing the drag operation.
	if (end_of_drag)
	{
		// Set to false so that when clients call 'in_drag()' it will return false.
		//
		// It's important to do this at the start because this function can update
		// the map camera which in turn signals the map to be rendered which in turn
		// queries 'in_drag()' to see if it should optimise rendering *during* a mouse drag.
		// And that all happens before we even leave the current function.
		d_mouse_drag_mode = boost::none;

		d_in_last_update_drag = true;
	}

	// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
	const double mouse_window_y = screen_height - screen_position.y();
	const double mouse_window_x = screen_position.x();

	switch (mouse_drag_mode)
	{
	case DRAG_PAN:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_pan_drag_info && !d_rotate_and_tilt_drag_info,
				GPLATES_ASSERTION_SOURCE);
		update_drag_pan(map_position, mouse_window_x, mouse_window_y, screen_width, screen_height);
		break;
	case DRAG_ROTATE_AND_TILT:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_pan_drag_info && d_rotate_and_tilt_drag_info,
				GPLATES_ASSERTION_SOURCE);
		update_drag_rotate_and_tilt(mouse_window_x, mouse_window_y, screen_width, screen_height);
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	//
	// Render the camera look-at position.
	//
	d_rendered_layer_ptr->clear_rendered_geometries();
	if (end_of_drag)
	{
		// At end of drag, so clear/deactivate our pan/rotate/tilt rendered geometry layer.
		d_rendered_geometry_collection.set_main_layer_active(
				RenderedGeometryCollection::PAN_ROTATE_TILT_IN_ALL_CANVAS_TOOLS_LAYER,
				false);
		d_rendered_layer_ptr->set_active(false);
	}
	else
	{
		// Render a small circle at the viewport centre which is the camera look-at position.
		// This is so the user can see the camera look-at position when panning/rotating/tilting.
		const GPlatesMaths::PointOnSphere look_at_position_on_globe = d_map_camera.get_look_at_position_on_globe();

		const RenderedGeometry look_at_rendered_geom =
				RenderedGeometryFactory::create_rendered_circle_symbol(
						look_at_position_on_globe,
						GPlatesGui::Colour::get_silver(),
						2.0f/*size*/,
						false/*filled*/,
						1.0f/*line_width_hint*/);
		d_rendered_layer_ptr->add_rendered_geometry(look_at_rendered_geom);
	}

	// If we've finished the drag operation.
	if (end_of_drag)
	{
		// Finished dragging mouse - no need for mouse drag info.
		d_pan_drag_info = boost::none;
		d_rotate_and_tilt_drag_info = boost::none;

		d_in_last_update_drag = false;
	}
}


void
GPlatesViewOperations::MapViewOperation::start_drag_pan(
		const boost::optional<QPointF> &start_map_position,
		const double &start_mouse_window_x,
		const double &start_mouse_window_y)
{
	boost::optional<QPointF> start_mouse_window_coords;
	if (start_map_position)
	{
		start_mouse_window_coords = QPointF(start_mouse_window_x, start_mouse_window_y);
	}

	d_pan_drag_info = PanDragInfo(start_mouse_window_coords);
}


void
GPlatesViewOperations::MapViewOperation::update_drag_pan(
		const boost::optional<QPointF> &map_position,
		double mouse_window_x,
		double mouse_window_y,
		int window_width,
		int window_height)
{
	//
	// Incrementally update the panning by calculating the pan due to dragging since the last drag update
	// (rather than since the start of the drag).
	//
	// The drag-from and drag-to mouse coordinates and map positions refer to this incremental update.
	//
	// This prevents the mouse appearing to no longer be responsive in panning the map until
	// the user moves the mouse position back to where it was when the map stopped panning.
	// By limiting each update to the interval since the last update, when the proposed (updated)
	// look-at position is outside the map projection boundary the user can just reverse the mouse
	// movement direction and panning will immediately continue again.
	//

	// The current mouse position is the drag-to (or destination) of the current drag update.
	const double drag_to_mouse_window_x = mouse_window_x;
	const double drag_to_mouse_window_y = mouse_window_y;
	const boost::optional<QPointF> &drag_to_map_position = map_position;

	// If the drag-to mouse position is not anywhere on the map *plane* then we cannot pan the view.
	if (!drag_to_map_position)
	{
		// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
		// So signal that the drag-from coordinates for the next drag update are *off* the map plane.
		// The next drag update will then know that it cannot pan the view.
		d_pan_drag_info->drag_from_mouse_window_coords = boost::none;  // Off map plane.
		return;
	}
	// If the drag-from mouse position is not anywhere on the map *plane* then we cannot pan the view.
	if (!d_pan_drag_info->drag_from_mouse_window_coords)
	{
		// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
		// So signal that the drag-from coordinates for the next drag update are *on* the map plane.
		// The next drag update will then know that it can pan the view (provided its drag-to is also on the map plane).
		d_pan_drag_info->drag_from_mouse_window_coords = QPointF(drag_to_mouse_window_x, drag_to_mouse_window_y);  // On map plane.
		return;
	}

	// Get the drag-from position on the map plane.
	// This uses the drag-from mouse coordinates, but calculated using the *current* map camera
	// (not camera from previous drag update).
	const boost::optional<QPointF> drag_from_map_position =
			d_map_camera.get_position_on_map_plane_at_window_coord(
					d_pan_drag_info->drag_from_mouse_window_coords->x(),
					d_pan_drag_info->drag_from_mouse_window_coords->y(),
					window_width,
					window_height);
	if (!drag_from_map_position)
	{
		// We shouldn't really get here because the drag-from mouse coordinates are the drag-to coordinates
		// from the previous drag update and that update had a drag-to map position *on* the map plane.
		// The only thing that changed was the map camera but it should only have panned (not rotated/tilted)
		// and so the ray at those mouse coordinates should still intersect the map plane.
		// But we'll handle this just in case.
		//
		// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
		// So signal that the drag-from coordinates for the next drag update are *off* the map plane.
		// And since *off* the map plane then return early (since cannot pan the view).
		d_pan_drag_info->drag_from_mouse_window_coords = boost::none;
		return;
	}

	// The new camera look-at position is the current look-at position plus the pan due to
	// mouse movement during the current drag update (from drag-from to drag-to).
	const QPointF pan_in_current_drag_update = drag_to_map_position.get() - drag_from_map_position.get();

	// Negate the pan because a change in view space is equivalent to the reverse change in model space and the map,
	// and points on it, are in model space. Essentially when we drag the mouse the view moves in the opposite direction.
	const QPointF camera_look_at_position = d_map_camera.get_look_at_position_on_map() - pan_in_current_drag_update;

	// Attempt to move the camera's look-at position on map.
	d_map_camera.move_look_at_position_on_map(
			camera_look_at_position,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
	// So signal that the drag-from coordinates for the next drag update are *on* the map plane.
	d_pan_drag_info->drag_from_mouse_window_coords = QPointF(drag_to_mouse_window_x, drag_to_mouse_window_y);
}


void
GPlatesViewOperations::MapViewOperation::start_drag_rotate_and_tilt(
		const double &start_mouse_window_x,
		const double &start_mouse_window_y)
{
	d_rotate_and_tilt_drag_info = RotateAndTiltDragInfo(start_mouse_window_x, start_mouse_window_y);
}


void
GPlatesViewOperations::MapViewOperation::update_drag_rotate_and_tilt(
		double mouse_window_x,
		double mouse_window_y,
		int window_width,
		int window_height)
{
	//
	// Incrementally update the rotating/tilting by calculating it due to dragging since the last drag update
	// (rather than since the start of the drag).
	//
	// The drag-from and drag-to mouse coordinates refer to this incremental update.
	//
	// This prevents the mouse appearing to no longer be responsive in tilting the map until
	// the user moves the mouse position back to where it was when the map stopped tilting.
	// By limiting each update to the interval since the last update, when the proposed (updated)
	// tilt is outside the [0, 90] degree range the user can just reverse the mouse 'y' movement direction
	// and tilting will immediately continue again.
	// This is not strictly needed for rotation in the 'x' direction but we do it anyway.
	//

	// The current mouse position is the drag-to (or destination) of the current drag update.
	const double rotate_to_mouse_window_coord = mouse_window_x;
	const double tilt_to_mouse_window_coord = mouse_window_y;

	//
	// Horizontal dragging rotates the view.
	//

	// Each multiple of PI means dragging the full window *width* will *rotate* by PI radians (180 degrees).
	const double delta_rotation_angle = 3 * GPlatesMaths::PI *
			(d_rotate_and_tilt_drag_info->rotate_from_mouse_window_coord - rotate_to_mouse_window_coord) / window_width;

	// New rotation angle.
	const GPlatesMaths::real_t rotation_angle = d_map_camera.get_rotation_angle() + delta_rotation_angle;

	d_map_camera.set_rotation_angle(
			rotation_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	//
	// Vertical dragging tilts the view.
	//

	// Each multiple of PI means dragging the full window *height* will *tilt* by PI radians (180 degrees).
	const double delta_tilt_angle = 1.5 * GPlatesMaths::PI *
			(tilt_to_mouse_window_coord - d_rotate_and_tilt_drag_info->tilt_from_mouse_window_coord) / window_height;

	const GPlatesMaths::real_t tilt_angle = d_map_camera.get_tilt_angle() + delta_tilt_angle;

	d_map_camera.set_tilt_angle(
			tilt_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	//
	// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
	//

	d_rotate_and_tilt_drag_info->rotate_from_mouse_window_coord = rotate_to_mouse_window_coord;
	d_rotate_and_tilt_drag_info->tilt_from_mouse_window_coord = tilt_to_mouse_window_coord;
}
