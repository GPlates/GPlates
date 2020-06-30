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
	d_globe_camera(globe_camera)
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
	// Note that OpenGL (window) and Qt (screen) y-axes are the reverse of each other.
	const double initial_mouse_window_y = screen_height - initial_mouse_screen_y;
	const double initial_mouse_window_x = initial_mouse_screen_x;

	d_mouse_drag_info = MouseDragInfo(
			mouse_drag_mode,
			initial_mouse_pos_on_globe.position_vector(),
			initial_mouse_window_x,
			initial_mouse_window_y,
			d_globe_camera.get_look_at_position().position_vector(),
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
		int screen_height)
{
	// If the drag operation was disabled in 'start_drag()' then return early.
	// This can happen when tilt dragging.
	if (!d_mouse_drag_info)
	{
		return;
	}

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
		update_drag_rotate_and_tilt(mouse_pos_on_globe.position_vector());
		break;
	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


void
GPlatesViewOperations::GlobeViewOperation::end_drag()
{
	// Finished dragging mouse - no need for mouse drag info.
	d_mouse_drag_info = boost::none;
}


void
GPlatesViewOperations::GlobeViewOperation::start_drag_normal()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);
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

	d_globe_camera.set_view_orientation(view_orientation);
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

	d_globe_camera.set_view_orientation(view_orientation);
}


void
GPlatesViewOperations::GlobeViewOperation::start_drag_tilt(
		int window_width,
		int window_height)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

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

	// See if centre-line camera ray intersects the globe.
	//
	// Since the camera ray is on the centre line of viewport, the intersection will be on the centre line
	// great circle of the globe (for this reason we could have intersected with the globe to get the same result).
	boost::optional<GPlatesMaths::PointOnSphere> globe_intersection_opt =
			d_globe_camera.get_position_on_globe_at_camera_ray(centre_line_camera_ray);
	if (!globe_intersection_opt)
	{
		// Camera ray does not intersect unit sphere.
		// So get nearest point on the horizon (visible circumference) of the globe.
		// This will be the nearest point on the great circle at the centre line of viewport.
		globe_intersection_opt = d_globe_camera.get_nearest_globe_horizon_position_at_camera_ray(centre_line_camera_ray);
	}
	const GPlatesMaths::Vector3D globe_intersection(globe_intersection_opt->position_vector());

	// Radius of tilt cylinder is distance from look-at position to centre-line ray globe intersection.
	//
	// We add a small epsilon to ensure the centre-line camera ray will intersect it if the intersection
	// happens to be tangential to the cylinder (due to numerical precision it might not have otherwise).
	const GPlatesMaths::real_t tilt_cylinder_radius = 1e-4 +
			(globe_intersection - GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)).magnitude();

	// The rotation axis that the view direction (and up direction) will tilt around.
	// However note that the axis will pass through the look-at position on globe surface (not globe centre).
	const GPlatesMaths::UnitVector3D tilt_axis = cross(
			d_mouse_drag_info->start_view_direction,
			d_mouse_drag_info->start_up_direction).get_normalisation();

	// Create a tilt cylinder whose axis passes through the look-at position.
	//
	// When the user tilts the view they are essentially grabbing this cylinder and rotating it.
	const GPlatesOpenGL::GLIntersect::Cylinder tilt_cylinder(
			GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*cylinder_base_point*/,
			tilt_axis,
			tilt_cylinder_radius);

	// Intersect centre-line camera ray, as an infinite line, with tilt cylinder (to find both intersections).
	//
	// Since the camera ray is on the centre line of viewport, we could have instead intersected with
	// a sphere of the same radius (and centered at look-at position) to get the same result.
	boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>> ray_distances_to_tilt_cylinder =
			intersect_line_cylinder(centre_line_camera_ray, tilt_cylinder);
	if (!ray_distances_to_tilt_cylinder)
	{
		// If the centre-line camera ray intersected the globe then it should also intersect the
		// tilt cylinder because cylinder radius was based on it (and we've made the radius slightly
		// larger to deal with any numerical precision issues). And so we should not get here.
		//
		// However if the centre-line camera ray did *not* intersect the globe then it's possible
		// the centre-line ray does not intersect the tilt cylinder. This can happen if the mouse
		// position is far enough off the globe. In this case we don't tilt the view.
		//
		// Disable the current drag operation.
		d_mouse_drag_info = boost::none;
		return;
	}

	const GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder =
			centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->first);
	const GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder =
			centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->second);

	// Determine which intersection matches the ray-globe-cylinder intersection projected onto tilt plane.
	const bool is_front_ray_intersect_tilt_cylinder =
			(front_ray_intersect_tilt_cylinder - globe_intersection).magSqrd() <
			(back_ray_intersect_tilt_cylinder - globe_intersection).magSqrd();
	const GPlatesMaths::Vector3D ray_intersect_tilt_cylinder =
			is_front_ray_intersect_tilt_cylinder
			? front_ray_intersect_tilt_cylinder
			: back_ray_intersect_tilt_cylinder;

	const GPlatesMaths::Vector3D ray_intersect_tilt_cylinder_rel_look_at =
			ray_intersect_tilt_cylinder - GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position);

	// Calculate rotation angle of cylinder intersection relative to the view direction.
	boost::optional<GPlatesMaths::real_t> cyl_intersect_relative_to_view_tilt_angle_opt = calc_rotation_angle(
			ray_intersect_tilt_cylinder_rel_look_at,
			tilt_axis/*rotation_axis*/,
			d_mouse_drag_info->start_view_direction/*zero_rotation_direction*/);
	const GPlatesMaths::real_t cyl_intersect_relative_to_view_tilt_angle =
			cyl_intersect_relative_to_view_tilt_angle_opt
			? cyl_intersect_relative_to_view_tilt_angle_opt.get()
			// Arbitrarily select angle zero...
			// When the mouse is very near the tilt axis then the globe will tilt wildly.
			// So when the mouse is directly *on* the tilt axis the user won't notice this arbitrariness.
			: GPlatesMaths::real_t(0);

	// Calculate rotation angle of view direction relative to the globe normal (look-at direction).
	const GPlatesMaths::real_t view_relative_to_globe_normal_tilt_angle = calc_rotation_angle(
			d_mouse_drag_info->start_view_direction,  // Perpendicular to tilt axis.
			tilt_axis/*rotation_axis*/,
			-d_mouse_drag_info->start_look_at_position/*zero_rotation_direction*/);

	d_mouse_drag_info->tilt_cylinder_radius = tilt_cylinder_radius;
	d_mouse_drag_info->start_cyl_intersect_relative_to_view_tilt_angle = cyl_intersect_relative_to_view_tilt_angle;
	d_mouse_drag_info->start_view_relative_to_globe_normal_tilt_angle = view_relative_to_globe_normal_tilt_angle;

	//qDebug() << "TILT CYLINDER RADIUS:" << tilt_cylinder_radius;
	//qDebug() << "start cyl_intersect_relative_to_view_tilt_angle:" << convert_rad_to_deg(cyl_intersect_relative_to_view_tilt_angle);
	//qDebug() << "start view_relative_to_globe_normal_tilt_angle:" << convert_rad_to_deg(view_relative_to_globe_normal_tilt_angle);

	// See if mouse is in upper part of viewport.
	// This will determine which way to tilt the globe when the mouse moves.
	d_mouse_drag_info->in_upper_viewport = (d_mouse_drag_info->start_mouse_window_y > window_height / 2.0);
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

	GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder;
	GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder;
	// Intersect centre-line camera ray, as an infinite line, with tilt cylinder (to find both intersections).
	//
	// Since the camera ray is on the centre line of viewport, we could have instead intersected with
	// a sphere of the same radius (and centered at look-at position) to get the same result.
	boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>> ray_distances_to_tilt_cylinder =
			intersect_line_cylinder(centre_line_camera_ray, tilt_cylinder);
	if (ray_distances_to_tilt_cylinder)
	{
		front_ray_intersect_tilt_cylinder =
				centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->first);
		back_ray_intersect_tilt_cylinder =
				centre_line_camera_ray.get_point_on_ray(ray_distances_to_tilt_cylinder->second);
	}
	else
	{
		// Find point on horizon of tilt circle (sphere) with respect to the camera.
		// As the mouse drags it can transition from intersecting the tilt circle to not intersecting it.
		// When it no longer intersects we need to set the tilt as if the last intersection was at
		// the very end of the circle (at horizon ray that touches circle tangentially).
		const GPlatesOpenGL::GLIntersect::Sphere tilt_sphere(
				GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*centre*/,
				d_mouse_drag_info->tilt_cylinder_radius);
		const GPlatesMaths::Vector3D cylinder_horizon_point =
				d_globe_camera.get_nearest_sphere_horizon_position_at_camera_ray(
						centre_line_camera_ray,
						tilt_sphere);

		// Both intersections are the same position since a camera ray to horizon point is
		// tangential to the tilt circle.
		front_ray_intersect_tilt_cylinder = cylinder_horizon_point;
		back_ray_intersect_tilt_cylinder = cylinder_horizon_point;
	}

	// Determine which intersection to use.
	GPlatesMaths::Vector3D ray_intersect_tilt_cylinder;
	if (d_mouse_drag_info->in_upper_viewport)
	{
		// Always use the back intersection.
		// TODO: Explain why.
		ray_intersect_tilt_cylinder = back_ray_intersect_tilt_cylinder;
		//qDebug() << "in upper viewport";
	}
	else
	{
		ray_intersect_tilt_cylinder = front_ray_intersect_tilt_cylinder;
	}

	const GPlatesMaths::Vector3D ray_intersect_tilt_cylinder_rel_look_at =
			ray_intersect_tilt_cylinder - GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position);

	// Calculate rotation angle of cylinder intersection relative to the *current* view direction.
	boost::optional<GPlatesMaths::real_t> cyl_intersect_relative_to_view_tilt_angle_opt = calc_rotation_angle(
			ray_intersect_tilt_cylinder_rel_look_at,
			tilt_axis/*rotation_axis*/,
			d_globe_camera.get_view_direction()/*zero_rotation_direction*/);
	const GPlatesMaths::real_t cyl_intersect_relative_to_view_tilt_angle =
			cyl_intersect_relative_to_view_tilt_angle_opt
			? cyl_intersect_relative_to_view_tilt_angle_opt.get()
			// Arbitrarily select angle zero...
			// When the mouse is very near the tilt axis then the globe will tilt wildly.
			// So when the mouse is directly *on* the tilt axis the user won't notice this arbitrariness.
			: GPlatesMaths::real_t(0);

	GPlatesMaths::real_t delta_cyl_intersect_relative_to_view_tilt_angle =
			cyl_intersect_relative_to_view_tilt_angle -
				d_mouse_drag_info->start_cyl_intersect_relative_to_view_tilt_angle;
	// Restrict difference to the range [-PI, PI].
	if (delta_cyl_intersect_relative_to_view_tilt_angle.dval() > GPlatesMaths::PI)
	{
		delta_cyl_intersect_relative_to_view_tilt_angle -= 2.0 * GPlatesMaths::PI;
	}
	else if (delta_cyl_intersect_relative_to_view_tilt_angle.dval() < -GPlatesMaths::PI)
	{
		delta_cyl_intersect_relative_to_view_tilt_angle += 2.0 * GPlatesMaths::PI;
	}

	// Need to tilt view in opposite direction to achieve same result as tilting the globe.
	delta_cyl_intersect_relative_to_view_tilt_angle = -delta_cyl_intersect_relative_to_view_tilt_angle;

	GPlatesMaths::real_t view_relative_to_globe_normal_tilt_angle =
			d_mouse_drag_info->start_view_relative_to_globe_normal_tilt_angle +
				delta_cyl_intersect_relative_to_view_tilt_angle;
	if (view_relative_to_globe_normal_tilt_angle.dval() > GPlatesMaths::HALF_PI)
	{
		view_relative_to_globe_normal_tilt_angle = GPlatesMaths::HALF_PI;
	}
	else if (view_relative_to_globe_normal_tilt_angle.dval() < 0)
	{
		view_relative_to_globe_normal_tilt_angle = 0;
	}
	//qDebug() << "delta_cyl_intersect_relative_to_view_tilt_angle:" << convert_rad_to_deg(delta_cyl_intersect_relative_to_view_tilt_angle);
	//qDebug() << "view_relative_to_globe_normal_tilt_angle:" << convert_rad_to_deg(view_relative_to_globe_normal_tilt_angle);

	// Calculate rotation angle of *current* view direction relative to the globe normal (look-at direction).
	//const GPlatesMaths::real_t view_relative_to_globe_normal_tilt_angle = calc_rotation_angle(
	//		d_view_direction,  // Perpendicular to tilt axis.
	//		tilt_axis/*rotation_axis*/,
	//		-d_look_at_position/*zero_rotation_direction*/);
	//qDebug() << "cyl_intersect_relative_to_view_tilt_angle:" << convert_rad_to_deg(cyl_intersect_relative_to_view_tilt_angle);
	//qDebug() << "start_cyl_intersect_relative_to_view_tilt_angle:" << convert_rad_to_deg(d_mouse_drag_info->start_cyl_intersect_relative_to_view_tilt_angle);
	//qDebug() << "delta_cyl_intersect_relative_to_view_tilt_angle:" << convert_rad_to_deg(delta_cyl_intersect_relative_to_view_tilt_angle);
	//qDebug() << "view_relative_to_globe_normal_tilt_angle:" << convert_rad_to_deg(view_relative_to_globe_normal_tilt_angle);
	//qDebug() << "view + delta_cyl tilt_angle:"
	//	<< convert_rad_to_deg(view_relative_to_globe_normal_tilt_angle + delta_cyl_intersect_relative_to_view_tilt_angle);
	//
	//if (delta_cyl_intersect_relative_to_view_tilt_angle.dval() >
	//	GPlatesMaths::HALF_PI - view_relative_to_globe_normal_tilt_angle.dval())
	//{
	//	delta_cyl_intersect_relative_to_view_tilt_angle =
	//			GPlatesMaths::HALF_PI - view_relative_to_globe_normal_tilt_angle;
	//}
	//else if (delta_cyl_intersect_relative_to_view_tilt_angle.dval() <
	//		-view_relative_to_globe_normal_tilt_angle.dval())
	//{
	//	delta_cyl_intersect_relative_to_view_tilt_angle = -view_relative_to_globe_normal_tilt_angle;
	//}

	d_mouse_drag_info->start_cyl_intersect_relative_to_view_tilt_angle =
			cyl_intersect_relative_to_view_tilt_angle +
			delta_cyl_intersect_relative_to_view_tilt_angle;

	d_globe_camera.set_tilt_angle(view_relative_to_globe_normal_tilt_angle);
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
		const GPlatesMaths::UnitVector3D &mouse_pos_on_globe)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

}
