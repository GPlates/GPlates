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
			d_globe_camera.get_view_orientation());

	switch (d_mouse_drag_info->mode)
	{
	case DRAG_NORMAL:
		start_drag_normal();
		break;
	case DRAG_ROTATE:
		start_drag_rotate();
		break;
	case DRAG_TILT:
		start_drag_tilt(screen_width, screen_height);
		break;
	case DRAG_ROTATE_AND_TILT:
		start_drag_rotate_and_tilt(screen_width, screen_height);
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
		case DRAG_ROTATE:
			update_drag_rotate(mouse_pos_on_globe.position_vector());
			break;
		case DRAG_TILT:
			update_drag_tilt(mouse_window_x, mouse_window_y, screen_width, screen_height);
				break;
		case DRAG_ROTATE_AND_TILT:
			update_drag_rotate_and_tilt(
					mouse_pos_on_globe.position_vector(),
					mouse_window_x, mouse_window_y,
					screen_width, screen_height);
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
GPlatesViewOperations::GlobeViewOperation::start_drag_rotate()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// The rotation angle, around look-at position, at the start of the drag.
	d_mouse_drag_info->start_rotation_angle = calc_drag_rotate_angle(
			d_mouse_drag_info->start_mouse_pos_on_globe,
			d_mouse_drag_info->start_look_at_position,
			d_mouse_drag_info->start_view_direction,
			d_mouse_drag_info->start_up_direction);
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_rotate(
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

	// The current rotation angle around look-at position.
	const GPlatesMaths::real_t rotation_angle = calc_drag_rotate_angle(
			mouse_pos_on_globe_relative_to_start_view,
			d_mouse_drag_info->start_look_at_position,
			d_mouse_drag_info->start_view_direction,
			d_mouse_drag_info->start_up_direction);

	// The model-space rotation from initial angle at start of drag to current angle.
	const GPlatesMaths::Rotation globe_rotation_relative_to_start = GPlatesMaths::Rotation::create(
			d_mouse_drag_info->start_look_at_position,
			rotation_angle - d_mouse_drag_info->start_rotation_angle);

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
GPlatesViewOperations::GlobeViewOperation::start_drag_tilt(
		int window_width,
		int window_height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// The rotation axis that the view direction (and up direction) will tilt around.
	// However note that the axis will pass through the look-at position on globe surface (not globe centre).
	const GPlatesMaths::UnitVector3D tilt_axis = cross(
			d_mouse_drag_info->start_view_direction,
			d_mouse_drag_info->start_up_direction).get_normalisation();

	// Calculate tilt angle, which is angle of view direction relative to the globe normal (look-at direction).
	d_mouse_drag_info->start_tilt_angle = calc_rotation_angle(
			d_mouse_drag_info->start_view_direction,  // Satisfies precondition: perpendicular to rotation axis.
			tilt_axis/*rotation_axis*/,
			-d_mouse_drag_info->start_look_at_position/*zero_rotation_direction*/);

#if 0
	if (!d_globe_camera.get_position_on_globe_at_camera_ray(
			d_globe_camera.get_camera_ray_at_window_coord(
					d_mouse_drag_info->start_mouse_window_x,
					d_mouse_drag_info->start_mouse_window_y,
					window_width,
					window_height)))
	{
		// The ray misses the globe so we cannot use cylinder intersections.
		// Instead we'll simply convert changes in mouse y-coordinate to changes in tilt angle.
		d_mouse_drag_info->tilt_method = TiltMethod::DONT_USE_CYLINDER_INTERSECTIONS;

		d_mouse_drag_info->start_intersects_globe_cylinder = false;

		return;
	}
#endif

	// Ray from camera eye to mouse position moved horizontally to centre line of viewport.
	//
	// Using the centre line of viewport removes any effect of the 'x' coordinate of the mouse coordinates
	// and instead relies only on the 'y' coordinate.
	// An alternative to moving the mouse position to the centre line of viewport is to subsequently
	// intersect the ray with a cylinder (containing globe), instead of a sphere (globe), and then
	// project the intersection onto the centre line (ie, onto tilt plane containing view and up vectors).
	//
	// Note that we use the mouse window coordinate (and not position on globe) because the window
	// coordinate might be *off* the globe (whereas position on globe will be nearest position *on* globe)
	// and we will be intersecting the ray with a cylinder that extends *off* the globe.
	const GPlatesOpenGL::GLIntersect::Ray centre_line_camera_ray =
			d_globe_camera.get_camera_ray_at_window_coord(
					window_width / 2.0,  // centre line of viewport
					d_mouse_drag_info->start_mouse_window_y,
					window_width,
					window_height);

	// Find a position on the surface of the tilt cylinder (so we can determine its radius).
	//
	// See if centre-line camera ray intersects the globe.
	// Since the camera ray is on the centre line of viewport, the intersection will be on the centre line
	// great circle of the globe (for this reason we could have intersected with the globe to get the same result).
	GPlatesMaths::Vector3D position_on_tilt_cylinder;
	if (boost::optional<GPlatesMaths::PointOnSphere> globe_intersection =
		d_globe_camera.get_position_on_globe_at_camera_ray(centre_line_camera_ray))
	{
		// The tilt cylinder surface will contain the ray-globe intersection.
		position_on_tilt_cylinder = GPlatesMaths::Vector3D(globe_intersection->position_vector());

		d_mouse_drag_info->start_intersects_globe_cylinder = true;
	}
	else // camera ray does not intersect globe...
	{
		// The ray misses the globe so we cannot use cylinder intersections.
		// Instead we'll simply convert changes in mouse y-coordinate to changes in tilt angle.
		d_mouse_drag_info->tilt_method = TiltMethod::DONT_USE_CYLINDER_INTERSECTIONS;

		d_mouse_drag_info->start_intersects_globe_cylinder = false;

		return;
	}

	// Radius of tilt cylinder is distance from look-at position to centre-line ray globe intersection.
	//
	// We add a small epsilon to ensure a subsequent centre-line camera ray will intersect the cylinder if the
	// ray happens to be tangential to the cylinder (due to numerical precision it might not have otherwise).
	d_mouse_drag_info->tilt_cylinder_radius = 1e-4 +
			(position_on_tilt_cylinder - GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)).magnitude();

	// Create a tilt cylinder whose axis passes through the look-at position.
	//
	// When the user tilts the view they are essentially grabbing this cylinder and rotating it.
	const GPlatesOpenGL::GLIntersect::Cylinder tilt_cylinder(
			GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*cylinder_base_point*/,
			tilt_axis,
			d_mouse_drag_info->tilt_cylinder_radius);

	// Intersect centre-line camera ray, as an infinite line, with tilt cylinder (to find both intersections).
	//
	// Since the camera ray is on the centre line of viewport, we could have instead intersected with
	// a sphere of the same radius (and centered at look-at position) to get the same result.
	boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>> ray_distances_to_tilt_cylinder =
			intersect_line_cylinder(centre_line_camera_ray, tilt_cylinder);
	if (!ray_distances_to_tilt_cylinder)
	{
		// We've ensured that the centre-line camera ray intersects the tilt cylinder by defining
		// a point on the cylinder surface that also intersects the ray's line. We've also made the
		// cylinder radius slightly larger to deal with any numerical precision issues).
		// And so we should not get here.
		//
		// Disable the current drag operation, which renders 'update_drag()' a no-op.
		d_mouse_drag_info = boost::none;
		return;
	}

	// Determine whether drag updates should use front or back ray-cylinder intersection, or use neither.
	//
	// Is mouse in upper part of viewport?
	if (d_mouse_drag_info->start_mouse_window_y > window_height / 2.0)
	{
		// When dragging the globe in the upper viewport, the upper half of the globe appears to
		// tilt away from the camera. This means the upper half of the globe always intersects with
		// the back half of the tilt cylinder (with respect to the view direction). So we know that
		// we can always use the back intersection when tilting the upper half of the globe.
		d_mouse_drag_info->tilt_method = TiltMethod::USE_CYLINDER_BACK_INTERSECTION;
	}
	else // mouse in lower viewport...
	{
		//
		// When dragging the globe in the lower viewport, the lower half of the globe appears to
		// tilt towards the camera. However, unlike the upper viewport, the lower half of the
		// globe can intersect with either the front or back half of the tilt cylinder
		// (with respect to the view direction) depending on the current tilt angle.
		//
		// When it intersects with the front we can use ray-cylinder intersections to tilt the
		// globe such that the mouse y-coordinate follows a position on the globe at the centre
		// vertical line of the viewport. In other words the user can essentially drag a feature
		// on the globe (along the centre line) and have the globe/view tilt such that the mouse
		// (y-coordinate) remains attached to that feature.
		//
		// When it intersects with the back we cannot use ray-cylinder intersections because a
		// mouse drag upwards (in the viewport) results in the globe (at the initial mouse coordinate)
		// tilting downwards, and so the mouse does not follow the initial position on the globe
		// (at the initial mouse coordinate). In this case we'll simply convert changes in the
		// mouse y-coordinate to changes in tilt angle.
		//

		const GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder =
				centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->first);
		const GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder =
				centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->second);

		// Determine which intersection matches the position on surface of tilt cylinder
		// (which is either the ray-globe intersection or closest point on ray's line to globe).
		if ((front_ray_intersect_tilt_cylinder - position_on_tilt_cylinder).magSqrd() <
			(back_ray_intersect_tilt_cylinder - position_on_tilt_cylinder).magSqrd())
		{
			// The globe currently intersects the front of tilt cylinder, so we can use
			// cylinder intersections (specifically front intersections).
			d_mouse_drag_info->tilt_method = TiltMethod::USE_CYLINDER_FRONT_INTERSECTION;
		}
		else // back intersection matches...
		{
			// The globe currently intersects the back of tilt cylinder, so we cannot use
			// cylinder intersections.
			d_mouse_drag_info->tilt_method = TiltMethod::DONT_USE_CYLINDER_INTERSECTIONS;

			return;
		}
	}

	// We're using tilt cylinder intersections (otherwise we would have returned from this method already)
	// so calculate the initial cylinder intersection angle with respect to the view direction.
	const GPlatesMaths::Vector3D ray_intersect_tilt_cylinder = centre_line_camera_ray.get_point_on_ray(
			(d_mouse_drag_info->tilt_method == TiltMethod::USE_CYLINDER_FRONT_INTERSECTION)
			? ray_distances_to_tilt_cylinder->first/*front*/
			: ray_distances_to_tilt_cylinder->second/*back*/);

	const GPlatesMaths::Vector3D ray_intersect_tilt_cylinder_rel_look_at =
			ray_intersect_tilt_cylinder - GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position);

	// Calculate rotation angle, relative to view direction, of vector from ray-cylinder intersection to look-at position.
	boost::optional<GPlatesMaths::real_t> cylinder_intersect_angle_relative_to_view_opt = calc_rotation_angle(
			ray_intersect_tilt_cylinder_rel_look_at,  // Satisfies precondition: perpendicular to rotation axis.
			tilt_axis/*rotation_axis*/,
			// Set the direction associated with a zero rotation angle to avoid wraparound when differencing
			// two angles. For front cylinder intersections this is achieved by placing the zero direction
			// on the front of the cylinder with respect to the view direction (such as the negative view direction).
			// For back intersections we place it on the back (such as positive view direction)...
			(d_mouse_drag_info->tilt_method == TiltMethod::USE_CYLINDER_FRONT_INTERSECTION)
				? -d_mouse_drag_info->start_view_direction
				: d_mouse_drag_info->start_view_direction);
	if (!cylinder_intersect_angle_relative_to_view_opt)
	{
		// The tilt cylinder intersection is at the look-at position. This can only happen if the cylinder
		// radius is zero (within epsilon) since the cylinder axis passes through the look-at position.
		// This shouldn't happen because the smallest cylinder radius is limited to 1e-4 above, which is
		// big enough to give a non-zero magnitude (within the much smaller epsilons used for that).
		// In any case, if the tilt radius is that small then the globe will tilt wildly for even tiny
		// mouse drag movements, so we might as well disable the current drag operation.
		//
		// Disable the current drag operation, which renders 'update_drag()' a no-op.
		d_mouse_drag_info = boost::none;
		return;
	}
	d_mouse_drag_info->start_cylinder_intersect_angle_relative_to_view =
			cylinder_intersect_angle_relative_to_view_opt.get();
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_tilt(
		double mouse_window_x,
		double mouse_window_y,
		int window_width,
		int window_height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// If we're not using tilt cylinder intersections then just use the mouse y-coordinate for tilting.
	// When the y-coordinate reaches half window height then tilting has reached its limit (either 0 or 90 degrees).
	if (d_mouse_drag_info->tilt_method == TiltMethod::DONT_USE_CYLINDER_INTERSECTIONS)
	{
		update_drag_tilt_without_cylinder_intersections(mouse_window_x, mouse_window_y, window_width, window_height);

		return;
	}

	// Ray from camera eye to mouse position moved horizontally to centre line of viewport.
	//
	// Using the centre line of viewport removes any effect of the 'x' coordinate of the mouse coordinates
	// and instead relies only on the 'y' coordinate.
	// An alternative to moving the mouse position to the centre line of viewport is to subsequently
	// intersect the ray with the tilt cylinder and then project the intersection onto the centre line
	// (ie, onto tilt plane containing view and up vectors).
	//
	// Note that we use the mouse window coordinate (and not position on globe) because the window
	// coordinate might be *off* the globe (whereas position on globe will be nearest position *on* globe)
	// and we will be intersecting the ray with a cylinder that extends *off* the globe.
	const GPlatesOpenGL::GLIntersect::Ray centre_line_camera_ray =
			d_globe_camera.get_camera_ray_at_window_coord(
					window_width / 2.0,  // centre line of viewport
					mouse_window_y,
					window_width,
					window_height);

	// The rotation axis that the view direction (and up direction) will tilt around.
	// However note that the axis will pass through the look-at position on globe surface (not globe centre).
	const GPlatesMaths::UnitVector3D tilt_axis =
			cross(d_globe_camera.get_view_direction(), d_globe_camera.get_up_direction()).get_normalisation();

	// Create a tilt cylinder whose axis passes through the look-at position.
	//
	// When the user tilts the view they are essentially grabbing this cylinder and rotating it.
	const GPlatesOpenGL::GLIntersect::Cylinder tilt_cylinder(
			GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*cylinder_base_point*/,
			tilt_axis,
			d_mouse_drag_info->tilt_cylinder_radius);

	GPlatesMaths::Vector3D pos_on_tilt_cylinder;
	GPlatesMaths::real_t delta_tilt_angle;  // Starts at zero and we add/subtract to it.

	// Intersect centre-line camera ray, as an infinite line, with tilt cylinder (to find both intersections).
	//
	// Since the camera ray is on the centre line of viewport, we could have instead intersected with
	// a sphere of the same radius (and centered at look-at position) to get the same result.
	boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>> ray_distances_to_tilt_cylinder =
			intersect_line_cylinder(centre_line_camera_ray, tilt_cylinder);
	if (ray_distances_to_tilt_cylinder)
	{
		const GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder =
				centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->first);
		const GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder =
				centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->second);

		// Use the front or back intersection as determined at the start of tilt dragging.
		pos_on_tilt_cylinder =
				(d_mouse_drag_info->tilt_method == TiltMethod::USE_CYLINDER_FRONT_INTERSECTION)
				? front_ray_intersect_tilt_cylinder
				: back_ray_intersect_tilt_cylinder;
	}
	else  // centre-line camera ray did *not* intersect tilt cylinder...
	{
		// Find point on horizon of tilt circle (sphere) with respect to the camera.
		// As the mouse drags it can transition from intersecting the tilt circle to not intersecting it.
		// When it no longer intersects we need to set the tilt as if the last intersection was at
		// the very end of the circle (at horizon ray that touches circle tangentially).
		//
		// Note that we're using a sphere instead of a cylinder here since we currently only have a
		// camera function to find horizon position on a sphere. It doesn't matter though because
		// sphere and cylinder are equivalent here since we're intersecting *centre line* of viewport.
		const GPlatesOpenGL::GLIntersect::Sphere tilt_sphere(
				GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*centre*/,
				d_mouse_drag_info->tilt_cylinder_radius);
		// Note that we cannot violate the precondition of the following camera function because if
		// we get here then the camera ray's *line* did not intersect the sphere which also means
		// the camera ray origin must be outside the sphere.
		const GPlatesMaths::Vector3D cylinder_horizon_point =
				d_globe_camera.get_nearest_sphere_horizon_position_at_camera_ray(
						centre_line_camera_ray,
						tilt_sphere);

		// The horizon point touches the tilt cylinder/circle tangentially.
		pos_on_tilt_cylinder = cylinder_horizon_point;

		//
		// The above position on tilt cylinder will later give us the tilt we'd get if the camera ray
		// intersected the tilt cylinder tangentially on its surface (at horizon point).
		// However there can also be a gap between the camera ray and the tilt cylinder as the user
		// drags the mouse past the tilt cylinder. In this case we need to apply a further adjustment
		// to the tilt angle. This is what the code block below does.
		//
		// Calculate the window y-coordinate of the horizon point. The difference between that and
		// the current mouse y-coordinate is the further adjustment to the tilt angle.
		// There is a bit of a noticeable transition in tilt speed as the user drags the mouse from
		// on the tilt cylinder to off it.
		//
		boost::optional<std::pair<double/*window_x*/, double/*window_y*/>> window_coord_at_horizon_point =
				d_globe_camera.get_window_coord_at_position(
						cylinder_horizon_point, window_width, window_height);
		if (window_coord_at_horizon_point)  // horizon point is not on the view plane containing camera eye
		{
			const GPlatesMaths::real_t window_y_at_horizon_point = window_coord_at_horizon_point->second;

			const GPlatesMaths::real_t half_window_height = window_height / 2.0;

			// This value increases from 0.0 at the horizon y-coordinate to +/- 1.0 at half window height.
			// This way the mouse distance from the horizon y-coordinate to the centre y of the
			// viewport represents a delta tilt of 90 degrees.
			GPlatesMaths::real_t mouse_drag_distance_away_from_tilt_cylinder;
			if (d_mouse_drag_info->start_mouse_window_y < half_window_height.dval())
			{
				// Initial mouse was in *lower* viewport so dragging mouse *down* (ie, away from tilt
				// cylinder) decreases tilt and hence should correspond to a negative value here
				// (when 'mouse_window_y < window_y_at_horizon_point')...
				mouse_drag_distance_away_from_tilt_cylinder =
						(mouse_window_y - window_y_at_horizon_point) /
						// Note that this is non-zero since horizon point cannot project onto centre of viewport...
						abs(half_window_height - window_y_at_horizon_point);
			}
			else
			{
				// Initial mouse was in *upper* viewport so dragging mouse *up*  (ie, away from tilt
				// cylinder) decreases tilt and hence should correspond to a negative value here
				// (when 'mouse_window_y > window_y_at_horizon_point')...
				mouse_drag_distance_away_from_tilt_cylinder =
						(window_y_at_horizon_point - mouse_window_y) /
						// Note that this is non-zero since horizon point cannot project onto centre of viewport...
						abs(half_window_height - window_y_at_horizon_point);
			}

			delta_tilt_angle += mouse_drag_distance_away_from_tilt_cylinder * GPlatesMaths::HALF_PI;
		}
	}

	const GPlatesMaths::Vector3D pos_on_tilt_cylinder_rel_look_at =
			pos_on_tilt_cylinder - GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position);

	// Calculate rotation angle of position on tilt cylinder relative to the *current* view direction.
	boost::optional<GPlatesMaths::real_t> cylinder_intersect_angle_relative_to_view_opt = calc_rotation_angle(
			pos_on_tilt_cylinder_rel_look_at,  // Satisfies precondition: perpendicular to rotation axis.
			tilt_axis/*rotation_axis*/,
			// Set the direction associated with a zero rotation angle to avoid wraparound when differencing
			// two angles. For front cylinder intersections this is achieved by placing the zero direction
			// on the front of the cylinder with respect to the view direction (such as the negative view direction).
			// For back intersections we place it on the back (such as positive view direction).
			// Note that this should be consistent with the angle calculated in 'start_drag_tilt()'...
			(d_mouse_drag_info->tilt_method == TiltMethod::USE_CYLINDER_FRONT_INTERSECTION)
				? -d_globe_camera.get_view_direction()
				: d_globe_camera.get_view_direction());
	if (!cylinder_intersect_angle_relative_to_view_opt)
	{
		// The position on tilt cylinder is at the look-at position. This can only happen if the cylinder
		// radius is zero (within epsilon) since the cylinder axis passes through the look-at position.
		// This shouldn't happen because the smallest cylinder radius was limited to 1e-4, which is
		// big enough to give a non-zero magnitude (within the much smaller epsilons used for that).
		// In any case, if the tilt radius is that small then the globe will tilt wildly for even tiny
		// mouse drag movements, so we return without updating the camera's tilt angle.
		return;
	}
	const GPlatesMaths::real_t cylinder_intersect_angle_relative_to_view =
			cylinder_intersect_angle_relative_to_view_opt.get();

	const GPlatesMaths::real_t delta_cylinder_intersect_angle_relative_to_view =
			cylinder_intersect_angle_relative_to_view -
				d_mouse_drag_info->start_cylinder_intersect_angle_relative_to_view;

	// Need to tilt view in opposite direction to achieve same result as tilting the globe.
	delta_tilt_angle -= delta_cylinder_intersect_angle_relative_to_view;

	const GPlatesMaths::real_t tilt_angle = d_mouse_drag_info->start_tilt_angle + delta_tilt_angle;

	d_globe_camera.set_tilt_angle(
			tilt_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_tilt_without_cylinder_intersections(
		double mouse_window_x,
		double mouse_window_y,
		int window_width,
		int window_height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::real_t half_window_height = window_height / 2.0;

	GPlatesMaths::real_t delta_tilt_angle;
	if (d_mouse_drag_info->start_intersects_globe_cylinder)
	{
		// If the mouse y-coordinate is not at the boundary between upper and lower viewport halves.
		// This is an epsilon test  (avoids divide-by-zero).
		if (half_window_height == d_mouse_drag_info->start_mouse_window_y)
		{
			return;
		}

		// This value increases from 0.0 at the initial mouse y-coordinate to 1.0 at half window height.
		// This way when the user drags the mouse from the initial y-coordinate to the centre y of the
		// viewport they get a delta tilt of 90 degrees.
		// This value is positive when the current y-coordinate is closer to half window height than
		// the initial y-coordinate, and negative when its farther away.
		const GPlatesMaths::real_t mouse_drag_distance =
				(mouse_window_y - d_mouse_drag_info->start_mouse_window_y) /
				(half_window_height - d_mouse_drag_info->start_mouse_window_y);

		delta_tilt_angle = mouse_drag_distance * GPlatesMaths::HALF_PI;
	}
	else // start of drag did not intersect globe cylinder...
	{
		// If the mouse y-coordinate is not at the centre of the viewport.
		// This is an epsilon test  (avoids divide-by-zero).
		if (half_window_height == 0)
		{
			return;
		}

		// This value increases from 0.0 at the initial mouse y-coordinate to 1.0 at a y distance
		// of half the window height away from the initial y-coordinate. This way when the user drags
		// the mouse half the distance up or down the viewport they get a delta tilt of 90 degrees.
		// If the initial mouse y-coordinate was in the lower viewport then this value is positive when
		// the current y-coordinate is above it.
		GPlatesMaths::real_t mouse_drag_distance =
				(mouse_window_y - d_mouse_drag_info->start_mouse_window_y) /
				half_window_height;
		// If the initial mouse y-coordinate was in the upper viewport then this value is positive when
		// the current y-coordinate is below it (so need to invert sign).
		if (d_mouse_drag_info->start_mouse_window_y > half_window_height.dval())
		{
			mouse_drag_distance = -mouse_drag_distance;
		}

		delta_tilt_angle = mouse_drag_distance * GPlatesMaths::HALF_PI;
	}

	// Increase or decrease the initial tilt angle (and clamp to [0, PI/2]).
	const GPlatesMaths::real_t tilt_angle = d_mouse_drag_info->start_tilt_angle + delta_tilt_angle;

	d_globe_camera.set_tilt_angle(
			tilt_angle,
			// Always emit on last update so client can turn off any rendering optimisations now that drag has finished...
			!d_in_last_update_drag/*only_emit_if_changed*/);
}


void
GPlatesViewOperations::GlobeViewOperation::start_drag_rotate_and_tilt(
		int window_width,
		int window_height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	start_drag_rotate();
	start_drag_tilt(window_width, window_height);
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_rotate_and_tilt(
		const GPlatesMaths::UnitVector3D &mouse_pos_on_globe,
		double mouse_window_x,
		double mouse_window_y,
		int window_width,
		int window_height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	update_drag_rotate(mouse_pos_on_globe);
	update_drag_tilt(
			mouse_window_x, mouse_window_y,
			window_width, window_height);
}
