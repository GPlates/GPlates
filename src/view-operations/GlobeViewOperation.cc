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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/GlobeCamera.h"

#include "opengl/GLIntersect.h"
#include "opengl/GLIntersectPrimitives.h"

#include "maths/MathsUtils.h"


namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Find angle that rotates @a zero_rotation_direction to @a vec about @a rotation_axis.
		 *
		 * Note that @a zero_rotation_direction and @a vec should both be perpendicular to @a rotation_axis.
		 */
		GPlatesMaths::real_t
		calc_rotation_angle(
				const GPlatesMaths::UnitVector3D &vec,
				const GPlatesMaths::UnitVector3D &rotation_axis,
				const GPlatesMaths::UnitVector3D &zero_rotation_direction)
		{
			// Absolute angle.
			GPlatesMaths::real_t angle = acos(dot(vec, zero_rotation_direction));

			// Angles go clockwise around rotation axis, so negate when going anti-clockwise.
			if (dot(vec, cross(rotation_axis, zero_rotation_direction)).dval() < 0)
			{
				angle = -angle;
			}

			return angle;
		}

		/**
		 * Find angle that rotates @a zero_rotation_direction to @a vec about @a rotation_axis.
		 *
		 * Note that @a zero_rotation_direction and @a vec should both be perpendicular to @a rotation_axis.
		 *
		 * Returns none if @a vec has zero magnitude (and hence rotation angle cannot be determined).
		 */
		boost::optional<GPlatesMaths::real_t>
		calc_rotation_angle(
				const GPlatesMaths::Vector3D &vec,
				const GPlatesMaths::UnitVector3D &rotation_axis,
				const GPlatesMaths::UnitVector3D &zero_rotation_direction)
		{
			if (vec.is_zero_magnitude())
			{
				return boost::none;
			}

			return calc_rotation_angle(
					vec.get_normalisation(),
					rotation_axis,
					zero_rotation_direction);
		}


		/**
		 * Calculate the rotation angle around look-at position.
		 *
		 * The zero-angle reference direction is to the right of the view direction (ie, cross(view, up)).
		 * And angles are clockwise around the look-at position/direction.
		 */
		GPlatesMaths::real_t
		calc_drag_rotate_angle(
				const GPlatesMaths::UnitVector3D &mouse_pos_on_globe,
				const GPlatesMaths::UnitVector3D &look_at_position,
				const GPlatesMaths::UnitVector3D &view_direction,
				const GPlatesMaths::UnitVector3D &up_direction)
		{
			// Plane of rotation passes through origin and has look-at direction as plane normal.
			const GPlatesMaths::UnitVector3D &rotation_axis = look_at_position;

			// Project mouse position onto plane of rotation (plane passes through origin and
			// has look-at direction as plane normal).
			const GPlatesMaths::Vector3D mouse_pos_on_globe_projected_onto_rotation_plane =
					GPlatesMaths::Vector3D(mouse_pos_on_globe) -
						dot(mouse_pos_on_globe, rotation_axis) * rotation_axis;

			// Zero-angle reference direction (perpendicular to look-at position/direction).
			const GPlatesMaths::UnitVector3D zero_rotation_direction = cross(
					view_direction,
					up_direction).get_normalisation();

			boost::optional<GPlatesMaths::real_t> rotation_angle = calc_rotation_angle(
					mouse_pos_on_globe_projected_onto_rotation_plane,
					rotation_axis,
					zero_rotation_direction);
			if (!rotation_angle)
			{
				// Arbitrarily select angle zero.
				// When the mouse is very near the rotation axis then the rotation will spin wildly.
				// So when the mouse is directly *on* the rotation axis the user won't notice this arbitrariness.
				rotation_angle = GPlatesMaths::real_t(0);
			}

			return rotation_angle.get();
		}
	}
}


GPlatesViewOperations::GlobeViewOperation::GlobeViewOperation(
		GPlatesGui::GlobeCamera &globe_camera) :
	d_globe_camera(globe_camera),
	d_in_drag_operation(false),
	d_in_last_update_drag(false)
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
	// We've started a drag operation.
	d_in_drag_operation = true;
	d_in_last_update_drag = false;

	// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
	const double initial_mouse_window_y = screen_height - initial_mouse_screen_y;
	const double initial_mouse_window_x = initial_mouse_screen_x;

	d_mouse_drag_info = MouseDragInfo(
			mouse_drag_mode,
			initial_mouse_pos_on_globe.position_vector(),
			initial_mouse_window_x,
			initial_mouse_window_y,
			d_globe_camera.get_look_at_position_on_globe().position_vector(),
			d_globe_camera.get_view_direction(),
			d_globe_camera.get_up_direction(),
			d_globe_camera.get_view_orientation(),
			d_globe_camera.get_tilt_angle());

	switch (d_mouse_drag_info->mode)
	{
	case DRAG_NORMAL:
		start_drag_normal();
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
GPlatesViewOperations::GlobeViewOperation::update_drag(
		const GPlatesMaths::PointOnSphere &mouse_pos_on_globe,
		double mouse_screen_x,
		double mouse_screen_y,
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
		// the globe camera which in turn signals the globe to be rendered which in turn
		// queries 'in_drag()' to see if it should optimise rendering *during* a mouse drag.
		// And that all happens before we even leave the current function.
		d_in_drag_operation = false;

		d_in_last_update_drag = true;
	}

	if (d_mouse_drag_info)  // drag operation might have been disabled in 'start_drag()' for some reason.
	{
		// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
		const double mouse_window_y = screen_height - mouse_screen_y;
		const double mouse_window_x = mouse_screen_x;

		switch (d_mouse_drag_info->mode)
		{
		case DRAG_NORMAL:
			update_drag_normal(mouse_pos_on_globe.position_vector());
			break;
		case DRAG_ROTATE_AND_TILT:
			update_drag_rotate_and_tilt(mouse_window_x, mouse_window_y, screen_width, screen_height);
			break;
		default:
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}
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
GPlatesViewOperations::GlobeViewOperation::start_drag_normal()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	//
	// Nothing to be done.
	//
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_normal(
		const GPlatesMaths::UnitVector3D &mouse_pos_on_globe)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// The current mouse position-on-globe is in global (universe) coordinates.
	// It actually doesn't change (within numerical precision) when the view rotates.
	// However, in the frame-of-reference of the view at the start of drag, it has changed.
	// To detect how much change we need to rotate it by the reverse of the change in view frame
	// (it's reverse because a change in view space is equivalent to the reverse change in model space
	// and the globe, and points on it, are in model space).
	const GPlatesMaths::UnitVector3D mouse_pos_on_globe_relative_to_start_view =
			d_mouse_drag_info->view_rotation_relative_to_start.get_reverse() * mouse_pos_on_globe;

	// The model-space rotation from initial position at start of drag to current position.
	const GPlatesMaths::Rotation globe_rotation_relative_to_start = GPlatesMaths::Rotation::create(
			d_mouse_drag_info->start_mouse_pos_on_globe,
			mouse_pos_on_globe_relative_to_start_view);

	// Rotation in view space is reverse of rotation in model space.
	const GPlatesMaths::Rotation view_rotation_relative_to_start = globe_rotation_relative_to_start.get_reverse();

	// Rotate the view frame.
	const GPlatesMaths::Rotation view_orientation = view_rotation_relative_to_start * d_mouse_drag_info->start_view_orientation;

	// Keep track of the updated view rotation relative to the start.
	d_mouse_drag_info->view_rotation_relative_to_start = view_rotation_relative_to_start;

	d_globe_camera.set_view_orientation(
			view_orientation,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);
}


void
GPlatesViewOperations::GlobeViewOperation::start_drag_rotate_and_tilt()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_rotate_and_tilt(
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
	const double view_rotation_angle_relative_to_start = 3 * GPlatesMaths::PI * (
			d_mouse_drag_info->start_mouse_window_x - mouse_window_x) / window_width;

	// The rotation from start of drag.
	const GPlatesMaths::Rotation view_rotation_relative_to_start = GPlatesMaths::Rotation::create(
			d_mouse_drag_info->start_look_at_position,
			view_rotation_angle_relative_to_start);

	// Rotate the view frame.
	const GPlatesMaths::Rotation view_orientation = view_rotation_relative_to_start * d_mouse_drag_info->start_view_orientation;

	d_globe_camera.set_view_orientation(
			view_orientation,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);

	//
	// Vertical dragging tilts the view.
	//

	// Each multiple of PI means dragging the full window *height* will *tilt* by PI radians (180 degrees).
	const double delta_tilt_angle = 1.5 * GPlatesMaths::PI * (mouse_window_y - d_mouse_drag_info->start_mouse_window_y) / window_height;

	const GPlatesMaths::real_t tilt_angle = d_mouse_drag_info->start_tilt_angle + delta_tilt_angle;

	d_globe_camera.set_tilt_angle(
			tilt_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);
}
