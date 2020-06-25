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
		const GPlatesMaths::PointOnSphere &mouse_pos_on_globe)
{
	d_mouse_drag_info = MouseDragInfo(
			mouse_drag_mode,
			mouse_pos_on_globe.position_vector(),
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
		start_drag_tilt();
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
		const GPlatesMaths::PointOnSphere &mouse_pos_on_globe)
{
	if (!d_mouse_drag_info)
	{
		return;
	}

	switch (d_mouse_drag_info->mode)
	{
	case DRAG_NORMAL:
		update_drag_normal(mouse_pos_on_globe.position_vector());
		break;
	case DRAG_ROTATE:
		update_drag_rotate(mouse_pos_on_globe.position_vector());
		break;
	case DRAG_TILT:
		update_drag_tilt(mouse_pos_on_globe.position_vector());
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
GPlatesViewOperations::GlobeViewOperation::start_drag_tilt()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// The rotation axis that the view direction (and up direction) will tilt around.
	// However note that the axis will pass through the look-at position on globe (not globe centre).
	const GPlatesMaths::UnitVector3D tilt_axis = cross(
			d_mouse_drag_info->start_view_direction,
			d_mouse_drag_info->start_up_direction).get_normalisation();

	// Create a cylinder that exactly contains the globe (unit sphere) and is aligned with the tilt axis.
	const GPlatesOpenGL::GLIntersect::Cylinder globe_cylinder(
			GPlatesMaths::Vector3D()/*globe origin*/,
			tilt_axis,
			1.0/*globe radius*/);

	// Ray from camera eye to mouse position on globe.
	const GPlatesOpenGL::GLIntersect::Ray ray =
			d_globe_camera.get_camera_ray_at_pos_on_globe(d_mouse_drag_info->start_mouse_pos_on_globe);

	// Intersect ray with globe cylinder (to find first intersection).
	//
	// Should only need first intersection (of ray with globe cylinder) because...
	// The ray origin should be outside the globe since the camera eye is outside.
	// And ray origin should then be outside cylinder since cylinder is along tilt axis which is
	// perpendicular to the view direction.
	boost::optional<GPlatesMaths::real_t> ray_distance_to_globe_cylinder =
			intersect_ray_cylinder(ray, globe_cylinder);
	if (!ray_distance_to_globe_cylinder)
	{
		// Ray should intersect cylinder because it should intersect globe.
		// If we get here then we are dealing with numerical precision issues.
		//
		// TODO: Use closest point on ray to cylinder.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}

	const GPlatesMaths::Vector3D ray_intersect_globe_cylinder =
			ray.get_point_on_ray(ray_distance_to_globe_cylinder.get());

	// Project ray-cylinder intersection onto the tilt plane.
	// The tilt plane passes through the globe origin and has a plane normal equal to the tilt axis.
	const GPlatesMaths::Vector3D ray_intersect_globe_cylinder_projected_onto_tilt_plane = ray_intersect_globe_cylinder -
			dot(ray_intersect_globe_cylinder, tilt_axis) * tilt_axis;

	// Radius of tilt cylinder is distance from look-at position to ray-globe-cylinder intersection projected onto tilt plane.
	// Note that the look-at position is already on the tilt plane (so doesn't need to be projected).
	const GPlatesMaths::real_t tilt_cylinder_radius =
			(ray_intersect_globe_cylinder_projected_onto_tilt_plane -
					GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)).magnitude();

	const GPlatesOpenGL::GLIntersect::Cylinder tilt_cylinder(
			GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*cylinder_base_point*/,
			tilt_axis,
			tilt_cylinder_radius);

	// Intersect ray, as an infinite line, with tilt cylinder (to find both intersections).
	boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>> ray_distances_to_tilt_cylinder =
			intersect_line_cylinder(ray, tilt_cylinder);
	if (ray_distances_to_tilt_cylinder)
	{
		const GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder =
				ray.get_point_on_ray(ray_distances_to_tilt_cylinder->first);
		const GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder =
				ray.get_point_on_ray(ray_distances_to_tilt_cylinder->second);

		// Project each ray-cylinder intersection onto the tilt plane.
		const GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder_projected_onto_tilt_plane =
				front_ray_intersect_tilt_cylinder -
					dot(front_ray_intersect_tilt_cylinder, tilt_axis) * tilt_axis;
		const GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder_projected_onto_tilt_plane =
				back_ray_intersect_tilt_cylinder -
					dot(back_ray_intersect_tilt_cylinder, tilt_axis) * tilt_axis;

		// Determine which intersection matches the ray-globe-cylinder intersection projected onto tilt plane.
		const bool is_front_ray_intersect_tilt_cylinder =
				(front_ray_intersect_tilt_cylinder_projected_onto_tilt_plane - ray_intersect_globe_cylinder_projected_onto_tilt_plane).magSqrd() <
				(back_ray_intersect_tilt_cylinder_projected_onto_tilt_plane - ray_intersect_globe_cylinder_projected_onto_tilt_plane).magSqrd();
		const GPlatesMaths::Vector3D ray_intersect_tilt_cylinder_projected_onto_tilt_plane =
				is_front_ray_intersect_tilt_cylinder
				? front_ray_intersect_tilt_cylinder_projected_onto_tilt_plane
				: back_ray_intersect_tilt_cylinder_projected_onto_tilt_plane;

		const GPlatesMaths::UnitVector3D &zero_rotation_direction = d_mouse_drag_info->start_look_at_position;

		GPlatesMaths::Vector3D ray_intersect_tilt_cylinder_projected_onto_tilt_plane_rel_look_at =
				ray_intersect_tilt_cylinder_projected_onto_tilt_plane -
					GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position);

		// Calculate rotation angle of cylinder intersection relative to the view direction.
		boost::optional<GPlatesMaths::real_t> cyl_intersect_relative_to_view_tilt_angle_opt = calc_rotation_angle(
				ray_intersect_tilt_cylinder_projected_onto_tilt_plane_rel_look_at,
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
		qDebug() << "TILT CYLINDER RADIUS:" << tilt_cylinder_radius;
		qDebug() << "start cyl_intersect_relative_to_view_tilt_angle:" << convert_rad_to_deg(cyl_intersect_relative_to_view_tilt_angle);
		qDebug() << "start view_relative_to_globe_normal_tilt_angle:" << convert_rad_to_deg(view_relative_to_globe_normal_tilt_angle);
	}
	else
	{
		// Ray should intersect cylinder because cylinder radius was based on it.
		// If we get here then we are dealing with numerical precision issues.
		//
		// TODO: Use closest point on ray to cylinder.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}

	// See if mouse is in upper part of viewport.
	// This will determine which way to tilt the globe when the mouse moves.
	const GPlatesOpenGL::GLIntersect::Plane up_plane(
			d_mouse_drag_info->start_up_direction/*normal*/,
			GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*point_on_plane*/);
	d_mouse_drag_info->in_upper_viewport = (
			up_plane.classify_point(d_mouse_drag_info->start_mouse_pos_on_globe) ==
				GPlatesOpenGL::GLIntersect::Plane::POSITIVE);
}


void
GPlatesViewOperations::GlobeViewOperation::update_drag_tilt(
		const GPlatesMaths::UnitVector3D &mouse_pos_on_globe)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_drag_info,
			GPLATES_ASSERTION_SOURCE);

	// Ray from camera eye to mouse position on globe.
	const GPlatesOpenGL::GLIntersect::Ray ray =
			d_globe_camera.get_camera_ray_at_pos_on_globe(mouse_pos_on_globe);

	// The rotation axis that the view direction (and up direction) will tilt around.
	// However note that the axis will pass through the look-at position on globe (not globe centre).
	const GPlatesMaths::UnitVector3D tilt_axis =
			cross(d_globe_camera.get_view_direction(), d_globe_camera.get_up_direction()).get_normalisation();

	const GPlatesOpenGL::GLIntersect::Cylinder tilt_cylinder(
			GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position)/*cylinder_base_point*/,
			tilt_axis,
			d_mouse_drag_info->tilt_cylinder_radius);

	// Intersect ray, as an infinite line, with tilt cylinder (to find both intersections).
	boost::optional<std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t>> ray_distances_to_tilt_cylinder =
			intersect_line_cylinder(ray, tilt_cylinder);
	if (ray_distances_to_tilt_cylinder)
	{
		const GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder =
				ray.get_point_on_ray(ray_distances_to_tilt_cylinder->first);
		const GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder =
				ray.get_point_on_ray(ray_distances_to_tilt_cylinder->second);

		// Project each ray-cylinder intersection onto the tilt plane.
		const GPlatesMaths::Vector3D front_ray_intersect_tilt_cylinder_projected_onto_tilt_plane =
				front_ray_intersect_tilt_cylinder -
					dot(front_ray_intersect_tilt_cylinder, tilt_axis) * tilt_axis;
		const GPlatesMaths::Vector3D back_ray_intersect_tilt_cylinder_projected_onto_tilt_plane =
				back_ray_intersect_tilt_cylinder -
					dot(back_ray_intersect_tilt_cylinder, tilt_axis) * tilt_axis;

		// Determine which intersection to use.
		GPlatesMaths::Vector3D ray_intersect_tilt_cylinder_projected_onto_tilt_plane;
		if (d_mouse_drag_info->in_upper_viewport)
		{
			// Always use the back intersection.
			// TODO: Explain why.
			ray_intersect_tilt_cylinder_projected_onto_tilt_plane = back_ray_intersect_tilt_cylinder_projected_onto_tilt_plane;
			//qDebug() << "in upper viewport";
		}
		else
		{
			qDebug() << "NOT in upper viewport";
			//
			// TODO: Handle this case.
		}

		GPlatesMaths::Vector3D ray_intersect_tilt_cylinder_projected_onto_tilt_plane_rel_look_at =
				ray_intersect_tilt_cylinder_projected_onto_tilt_plane -
					GPlatesMaths::Vector3D(d_mouse_drag_info->start_look_at_position);

		// Calculate rotation angle of cylinder intersection relative to the *current* view direction.
		boost::optional<GPlatesMaths::real_t> cyl_intersect_relative_to_view_tilt_angle_opt = calc_rotation_angle(
				ray_intersect_tilt_cylinder_projected_onto_tilt_plane_rel_look_at,
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
		qDebug() << "delta_cyl_intersect_relative_to_view_tilt_angle:" << convert_rad_to_deg(delta_cyl_intersect_relative_to_view_tilt_angle);
		qDebug() << "view_relative_to_globe_normal_tilt_angle:" << convert_rad_to_deg(view_relative_to_globe_normal_tilt_angle);

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

		d_mouse_drag_info->start_cyl_intersect_relative_to_view_tilt_angle = cyl_intersect_relative_to_view_tilt_angle +
				delta_cyl_intersect_relative_to_view_tilt_angle;

		d_globe_camera.set_tilt_angle(view_relative_to_globe_normal_tilt_angle);
	}
	else
	{
		qDebug() << "Missed tilt cylinder";
		//
		// TODO: Handle this case - perhaps use closest point on ray to cylinder.
	}
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
