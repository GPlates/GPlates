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
#include "gui/MapProjection.h"

#include "maths/MathsUtils.h"


namespace
{
	/**
	 * Return the length of the specified QPointF.
	 */
	double
	get_length(
			const QPointF &point)
	{
		return std::sqrt(QPointF::dotProduct(point, point));
	}
}


GPlatesViewOperations::MapViewOperation::MapViewOperation(
		GPlatesGui::MapCamera &map_camera,
		const GPlatesGui::MapProjection &map_projection,
		RenderedGeometryCollection &rendered_geometry_collection) :
	d_map_camera(map_camera),
	d_map_projection(map_projection),
	d_in_last_update_drag(false),
	d_rendered_geometry_collection(rendered_geometry_collection)
{
}


void
GPlatesViewOperations::MapViewOperation::start_drag(
		MouseDragMode mouse_drag_mode,
		const QPointF &initial_screen_position,
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
		start_drag_pan(initial_mouse_window_x, initial_mouse_window_y);
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
		update_drag_pan(mouse_window_x, mouse_window_y, screen_width, screen_height);
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
		const double &start_mouse_window_x,
		const double &start_mouse_window_y)
{
	d_pan_drag_info = PanDragInfo(start_mouse_window_x, start_mouse_window_y);
}


void
GPlatesViewOperations::MapViewOperation::update_drag_pan(
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

	// The *previous* mouse position is the drag-*from* (or source) of the current drag update.
	const QPointF drag_from_mouse_window_coords = d_pan_drag_info->drag_from_mouse_window_coords;
	// The *current* mouse position is the drag-*to* (or destination) of the current drag update.
	const QPointF drag_to_mouse_window_coords(mouse_window_x, mouse_window_y);

	// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
	//
	// We set this here (upfront) and only use the local variable 'drag_from_mouse_window_coords' from now on.
	// That way we can bail out and return early knowing this has been taken care of for the next drag update.
	d_pan_drag_info->drag_from_mouse_window_coords = drag_to_mouse_window_coords;

	// Get the drag-from position on the map plane.
	// This uses the drag-from mouse coordinates, but calculated using the *current* map camera
	// (not camera from previous drag update).
	const GPlatesOpenGL::GLIntersect::Ray drag_from_camera_ray = d_map_camera.get_camera_ray_at_window_coord(
			drag_from_mouse_window_coords.x(),
			drag_from_mouse_window_coords.y(),
			window_width,
			window_height);
	boost::optional<QPointF> drag_from_map_position = d_map_camera.get_position_on_map_plane_at_camera_ray(
			drag_from_camera_ray);

	//
	// If the drag-from map position is not inside the map boundary (or not even *on* the map plane)
	// then get the position on the map boundary that is the intersection of the map boundary with the
	// line segment from the camera look-at position (always inside map boundary) and the drag-from
	// map position (outside the map boundary).
	//
	// This will be our new drag-from map position. And it'll correspond to mouse window coordinates that
	// are offset from the actual drag-from mouse window coordinates. So we'll need to apply the same offset
	// to the drag-*to* mouse window coordinates. This essentially pretends the mouse drag-from position
	// started on the map boundary (rather than outside the map boundary).
	//
	// Other approaches were experimented with but the current approach seems to work the best because:
	// - It generates no discontinuous panning motions.
	//   * Between positions on and off the map plane, and
	//   * between positions inside and outside map boundary (on map plane).
	// - It supports panning when the mouse if *off* the map plane.
	// - When the mouse is outside the map boundary the panning (direction and speed) seems appropriate for the respective outside positions, and
	//   * mouse doesn't move from outside to inside map boundary in a single drag (unless camera look-at gets clamped to boundary).
	// Other approached included:
	// - Not clamping map positions outside map boundary (but still on map plane) to map boundary.
	//   Problem was positions (outside map boundary) close to the camera panned too slowly (and far away positions panned too quickly).
	//   Also there was no panning for map positions *off* the map plane.
	// - Reseting map positions outside map boundary to the camera look-at position.
	//   Problem was panning motions appeared discontinuous and mouse could move from outside to inside
	//   map boundary within a single panning motion.
	//
	QPointF mouse_window_offset(0, 0);
	if (!(drag_from_map_position && d_map_projection.is_inside_map_boundary(drag_from_map_position.get())))
	{
		// The drag-from mouse coordinates are NOT inside the map boundary.

		if (drag_from_map_position) // drag-from mouse coordinates are *on* the map plane...
		{
			// Get the intersection of line segment (from camera look-at position to camera ray intersection on map plane)
			// with map projection boundary.
			drag_from_map_position = d_map_projection.get_map_boundary_position(
					d_map_camera.get_look_at_position_on_map(),
					drag_from_map_position.get());
		}
		else // drag-from mouse coordinates are *off* the map plane...
		{
			// Project the 3D camera ray *direction* onto 2D map plane (z=0).
			const QPointF drag_from_2d_ray_direction(
					drag_from_camera_ray.get_direction().x().dval(),
					drag_from_camera_ray.get_direction().y().dval());
			// Camera look-at position.
			const QPointF drag_from_2d_ray_origin = d_map_camera.get_look_at_position_on_map();

			// Intersect 2D ray, from camera look-at position in direction of 3D camera ray (projected onto 2D map plane),
			// with map projection boundary.
			drag_from_map_position = d_map_camera.get_position_on_map_boundary_intersected_by_2d_camera_ray(
					drag_from_2d_ray_direction,
					drag_from_2d_ray_origin);
			if (!drag_from_map_position)
			{
				// The 3D camera ray direction points straight down (ie, camera ray x and y are zero).
				//
				// We shouldn't really get here for a valid camera ray since we already know it did not intersect the
				// 2D map plane and so if it points straight down then it would have intersected the map plane (z=0).
				// However it's possible that at 90 degree tilt the camera eye (in perspective viewing) dips just below
				// the map plane (z=0) due to numerical tolerance and hence just misses the map plane.
				// But even then the camera view direction would be horizontal and with a field-of-view of 90 degrees or less
				// there wouldn't be any screen pixel in the view frustum that could look straight down.
				// So it really should never happen.
				//
				// Not much can be done, so just return without panning the map camera.
				return;
			}
		}

		// Get the mouse window coordinates corresponding to the map boundary position.
		const boost::optional<QPointF> drag_from_mouse_window_coord =
				d_map_camera.get_window_coord_at_position(
						GPlatesMaths::Vector3D(drag_from_map_position->x(), drag_from_map_position->y(), 0),
						window_width, window_height);
		if (!drag_from_mouse_window_coord)
		{
			// The map boundary position happened to be in the plane containing the camera eye (the plane
			// with normal in the view direction), and the projection is perspective (not orthographic).
			//
			// Not much can be done, so just return without panning the map camera.
			return;
		}

		// Set the non-zero mouse window offset to also apply to the drag-*to* mouse window coordinates.
		mouse_window_offset = drag_from_mouse_window_coord.get() - drag_from_mouse_window_coords;
	}

	// Get the drag-to position on the map plane.
	// This uses the drag-to mouse coordinates plus any mouse offset applied to the drag-*from* mouse coordinates.
	const GPlatesOpenGL::GLIntersect::Ray drag_to_camera_ray = d_map_camera.get_camera_ray_at_window_coord(
			drag_to_mouse_window_coords.x() + mouse_window_offset.x(),
			drag_to_mouse_window_coords.y() + mouse_window_offset.y(),
			window_width,
			window_height);
	boost::optional<QPointF> drag_to_map_position = d_map_camera.get_position_on_map_plane_at_camera_ray(
			drag_to_camera_ray);
	//
	// Unlike the drag-*from* map position we do not force the drag-*to* map position to be inside the
	// map boundary. It's fine if it's just *on* the map plane. This is because we simply want to know
	// how much to pan the map (so we don't want to clamp to the map boundary for that).
	//
	// However if it's *off* the map plane then find a map position *on* the plane using the 3D camera ray
	// projected on the 2D map plane (z=0) that, while not infinitely far away, is still far enough away
	// from the map boundary that it'll likely pan the camera so much as to cause the camera look-at
	// position to become pinned to the map boundary (since camera look-at cannot go outside map boundary).
	// That's what we want because that is also what would happen if it was *on* the map plane but very far away.
	//
	if (!drag_to_map_position)
	{
		// Project the 3D camera ray origin onto the 2D map plane (z=0).
		const QPointF drag_to_camera_ray_origin_on_map_plane(
				drag_to_camera_ray.get_origin().x().dval(),
				drag_to_camera_ray.get_origin().y().dval());

		// Project the 3D camera ray direction onto 2D map plane (z=0).
		const QPointF drag_to_camera_ray_direction_on_map_plane(
				drag_to_camera_ray.get_direction().x().dval(),
				drag_to_camera_ray.get_direction().y().dval());

		// Length of 2D projected camera ray direction.
		const GPlatesMaths::real_t length_of_drag_to_camera_ray_direction_on_map_plane =
				get_length(drag_to_camera_ray_direction_on_map_plane);
		if (length_of_drag_to_camera_ray_direction_on_map_plane == 0) // epsilon comparison
		{
			// The 3D camera ray direction points straight down (ie, camera ray x and y are zero).
			//
			// We shouldn't really get here for a valid camera ray since we already know it did not intersect the
			// 2D map plane and so if it points straight down then it would have intersected the map plane (z=0).
			// However it's possible that at 90 degree tilt the camera eye (in perspective viewing) dips just below
			// the map plane (z=0) due to numerical tolerance and hence just misses the map plane.
			// But even then the camera view direction would be horizontal and with a field-of-view of 90 degrees or less
			// there wouldn't be any screen pixel in the view frustum that could look straight down.
			// Well actually, there is the mouse offset which could push it outside the field-of-view (so there's that).
			// But it really should never happen in practice.
			//
			// If this happens then no panning is really possible, so just return without panning the map camera.
			return;
		}

		// Make the drag-to map position an arbitrarily large distance (large multiple of map bounding radius)
		// from the 2D projected camera ray *origin* in the direction of the 2D projected camera ray *direction*.
		drag_to_map_position = drag_to_camera_ray_origin_on_map_plane +
				(1000 * d_map_projection.get_map_bounding_radius() / length_of_drag_to_camera_ray_direction_on_map_plane.dval()) *
				drag_to_camera_ray_direction_on_map_plane;
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
	//
	// Note that dragging from left to right produces a *positive* delta angle.
	const double delta_rotation_angle = 3 * GPlatesMaths::PI *
			(rotate_to_mouse_window_coord - d_rotate_and_tilt_drag_info->rotate_from_mouse_window_coord) / window_width;

	// Rotate the camera.
	//
	// Note that dragging from left to right produces a *positive* delta angle. And when the camera rotates clockwise
	// it appears that the map is rotating anticlockwise (relative to the camera view).
	//
	// Hence dragging from left to right makes the map appear to rotate anticlockwise.
	d_map_camera.rotate_clockwise(
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
	d_map_camera.tilt_more(
			delta_tilt_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	//
	// The current drag-to mouse coordinates will be the drag-from coordinates for the next drag update.
	//

	d_rotate_and_tilt_drag_info->rotate_from_mouse_window_coord = rotate_to_mouse_window_coord;
	d_rotate_and_tilt_drag_info->tilt_from_mouse_window_coord = tilt_to_mouse_window_coord;
}
