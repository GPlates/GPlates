/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include "GlobeViewOperation.h"

#include "RenderedGeometryFactory.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/GlobeCamera.h"

#include "maths/MathsUtils.h"


GPlatesViewOperations::GlobeViewOperation::GlobeViewOperation(
		GPlatesGui::GlobeCamera &globe_camera,
		RenderedGeometryCollection &rendered_geometry_collection) :
	d_globe_camera(globe_camera),
	d_in_last_update_drag(false),
	d_rendered_geometry_collection(rendered_geometry_collection)
{
}


void
GPlatesViewOperations::GlobeViewOperation::start_drag(
		MouseDragMode mouse_drag_mode,
		const GPlatesMaths::PointOnSphere &initial_mouse_pos_on_globe,
		double initial_mouse_screen_x,
		double initial_mouse_screen_y,
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
	const double initial_mouse_window_y = screen_height - initial_mouse_screen_y;
	const double initial_mouse_window_x = initial_mouse_screen_x;

	switch (mouse_drag_mode)
	{
	case DRAG_PAN:
		start_drag_pan(initial_mouse_pos_on_globe);
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
GPlatesViewOperations::GlobeViewOperation::update_drag(
		const GPlatesMaths::PointOnSphere &mouse_pos_on_globe,
		double mouse_screen_x,
		double mouse_screen_y,
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
		// the globe camera which in turn signals the globe to be rendered which in turn
		// queries 'in_drag()' to see if it should optimise rendering *during* a mouse drag.
		// And that all happens before we even leave the current function.
		d_mouse_drag_mode = boost::none;

		d_in_last_update_drag = true;
	}

	// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
	const double mouse_window_y = screen_height - mouse_screen_y;
	const double mouse_window_x = mouse_screen_x;

	switch (mouse_drag_mode)
	{
	case DRAG_PAN:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_pan_drag_info && !d_rotate_and_tilt_drag_info,
				GPLATES_ASSERTION_SOURCE);
		update_drag_pan(mouse_pos_on_globe);
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
		const GPlatesMaths::PointOnSphere look_at_position_on_globe = d_globe_camera.get_look_at_position_on_globe();

		const RenderedGeometry look_at_rendered_geom =
				RenderedGeometryFactory::create_rendered_circle_symbol(
						look_at_position_on_globe,
						GPlatesGui::Colour::get_silver(),
						3.0f/*size*/,
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
GPlatesViewOperations::GlobeViewOperation::start_drag_pan(
		const GPlatesMaths::PointOnSphere &start_mouse_pos_on_globe)
{
	d_pan_drag_info = PanDragInfo(
			start_mouse_pos_on_globe,
			d_globe_camera.get_view_orientation());
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_pan(
		const GPlatesMaths::PointOnSphere &mouse_pos_on_globe)
{
	// The current mouse position-on-globe is in global (universe) coordinates.
	// It actually doesn't change (within numerical precision) when the view rotates.
	// However, in the frame-of-reference of the view at the start of drag, it has changed.
	// To detect how much change we need to rotate it by the reverse of the change in view frame
	// (it's reverse because a change in view space is equivalent to the reverse change in model space
	// and the globe, and points on it, are in model space).
	const GPlatesMaths::UnitVector3D mouse_pos_on_globe_relative_to_start_view =
			d_pan_drag_info->view_rotation_relative_to_start.get_reverse() * mouse_pos_on_globe.position_vector();

	// The model-space rotation from initial position at start of drag to current position.
	const GPlatesMaths::Rotation globe_rotation_relative_to_start = GPlatesMaths::Rotation::create(
			d_pan_drag_info->start_mouse_pos_on_globe,
			mouse_pos_on_globe_relative_to_start_view);

	// Rotation in view space is reverse of rotation in model space.
	const GPlatesMaths::Rotation view_rotation_relative_to_start = globe_rotation_relative_to_start.get_reverse();

	// Rotate the view frame.
	const GPlatesMaths::Rotation view_orientation = view_rotation_relative_to_start * d_pan_drag_info->start_view_orientation;

	// Keep track of the updated view rotation relative to the start.
	d_pan_drag_info->view_rotation_relative_to_start = view_rotation_relative_to_start;

	d_globe_camera.set_view_orientation(
			view_orientation,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);
}


void
GPlatesViewOperations::GlobeViewOperation::start_drag_rotate_and_tilt(
		const double &start_mouse_window_x,
		const double &start_mouse_window_y)
{
	d_rotate_and_tilt_drag_info = RotateAndTiltDragInfo(
			start_mouse_window_x,
			start_mouse_window_y);
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_rotate_and_tilt(
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
	//
	// Note that dragging from left to right produces a *positive* delta angle.
	const double delta_rotation_angle = 3 * GPlatesMaths::PI *
			(rotate_to_mouse_window_coord - d_rotate_and_tilt_drag_info->rotate_from_mouse_window_coord) / window_width;

	// Rotate the camera.
	//
	// Note that dragging from left to right produces a *positive* delta angle. And when the camera rotates clockwise
	// it appears that the globe is rotating anticlockwise (relative to the camera view).
	//
	// Hence dragging from left to right makes the globe appear to rotate anticlockwise.
	d_globe_camera.rotate_clockwise(
			delta_rotation_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	//
	// Vertical dragging tilts the view.
	//

	// Each multiple of PI means dragging the full window *height* will *tilt* by PI radians (180 degrees).
	//
	// Note that dragging from bottom to top produces a *positive* delta angle.
	const double delta_tilt_angle = 1.5 * GPlatesMaths::PI *
			(tilt_to_mouse_window_coord - d_rotate_and_tilt_drag_info->tilt_from_mouse_window_coord) / window_height;

	// Tilt the camera.
	//
	// Note that dragging from bottom to top produces a *positive* delta angle, which causes the camera to tilt more.
	d_globe_camera.tilt_more(
			delta_tilt_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	//
	// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
	//

	d_rotate_and_tilt_drag_info->rotate_from_mouse_window_coord = rotate_to_mouse_window_coord;
	d_rotate_and_tilt_drag_info->tilt_from_mouse_window_coord = tilt_to_mouse_window_coord;
}
