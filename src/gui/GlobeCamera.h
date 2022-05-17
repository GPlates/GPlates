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

#ifndef GPLATES_GUI_GLOBECAMERA_H
#define GPLATES_GUI_GLOBECAMERA_H

#include <utility>  // std::pair
#include <boost/optional.hpp>

#include "Camera.h"
#include "GlobeProjectionType.h"

#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "opengl/GLIntersectPrimitives.h"


namespace GPlatesGui
{
	class ViewportZoom;

	/**
	 * Handles viewing of the globe (eye location, view direction, projection).
	 */
	class GlobeCamera :
			public Camera
	{
	public:
		/**
		 * At the initial zoom, and untilted view, the smaller dimension of the viewport will be
		 * @a FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT times the diameter of the globe
		 * (in the orthographic projection). This creates a little space between the globe circumference
		 * and the viewport. When the viewport is resized, the globe will be scaled accordingly.
		 *
		 * The value of this constant is purely cosmetic.
		 */
		static const double FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT;


		explicit
		GlobeCamera(
				ViewportZoom &viewport_zoom);

		/**
		 * The position on the globe that the view is looking at.
		 */
		const GPlatesMaths::PointOnSphere &
		get_look_at_position_on_globe() const;

		/**
		 * Same as @a get_look_at_position_on_globe but returned as a Vector3D.
		 */
		GPlatesMaths::Vector3D
		get_look_at_position() const override
		{
			return GPlatesMaths::Vector3D(get_look_at_position_on_globe().position_vector());
		}

		/**
		 * The view direction.
		 *
		 * For perspective viewing this is from the eye position to the look-at position.
		 */
		GPlatesMaths::UnitVector3D
		get_view_direction() const override;

		/**
		 * The 'up' vector for the view orientation.
		 */
		GPlatesMaths::UnitVector3D
		get_up_direction() const override;


		/**
		 * The camera orientation (excluding tilt).
		 *
		 * This is the rotation of the moving camera around the fixed globe.
		 *
		 * Note that we don't actually rotate the globe, instead rotating the view frame
		 * (look-at position and view/up directions) to achieve the same effect.
		 */
		const GPlatesMaths::Rotation &
		get_view_orientation() const
		{
			return d_view_orientation;
		}

		/**
		 * The orientation of the fixed globe relative to the moving camera (excluding tilt).
		 *
		 * Even though the globe is fixed (in the global universe coordinate system) and the camera moves,
		 * from the point of view of the camera it looks like the globe is rotating in the opposite direction
		 * of camera movement.
		 *
		 * Note that we don't actually rotate the globe, instead rotating the view frame
		 * (look-at position and view/up directions) to achieve the same effect.
		 */
		GPlatesMaths::Rotation
		get_globe_orientation_relative_to_view() const
		{
			return d_view_orientation.get_reverse();
		}

		/**
		 * Set the camera orientation (excluding tilt).
		 *
		 * Note that this does not change the current tilt angle.
		 *
		 * This sets the inverse orientation set by @a set_globe_orientation_relative_to_view.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_view_orientation(
				const GPlatesMaths::Rotation &view_orientation,
				bool only_emit_if_changed = true);

		/**
		 * Set the orientation of the fixed globe relative to the moving camera (excluding tilt).
		 *
		 * Note that this does not change the current tilt angle.
		 *
		 * This sets the inverse orientation set by @a set_view_orientation.
		 */
		void
		set_globe_orientation_relative_to_view(
				const GPlatesMaths::Rotation &globe_orientation_relative_to_view)
		{
			set_view_orientation(globe_orientation_relative_to_view.get_reverse());
		}

		/**
		 * The angle (in radians) that the view direction tilts.
		 *
		 * Zero angle implies looking straight down on the globe. And PI/2 (90 degrees) means the view
		 * direction is looking tangentially at the globe surface (at look-at position) and the up
		 * direction is pointing radially outward from the globe.
		 */
		GPlatesMaths::real_t
		get_tilt_angle() const
		{
			return d_tilt_angle;
		}

		/**
		 * Set the angle (in radians) that the view direction tilts.
		 *
		 * Note that this does not change the current view orientation (returned by @a get_view_orientation).
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_tilt_angle(
				const GPlatesMaths::real_t &tilt_angle,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the current look-at position to the specified look-at position along the
		 * great circle arc between them.
		 *
		 * The view and up directions are rotated by same rotation as look-at position.
		 *
		 * Note that this does not change the current tilt angle.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		move_look_at_position(
				const GPlatesMaths::PointOnSphere &new_look_at_position,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the view around the view direction so that the "up" direction points
		 * towards the North pole when @a reorientation_angle is zero.
		 *
		 * @a reorientation_angle, in radians, is [0, PI] for anti-clockwise view orientation with respect
		 * to North pole (note globe appears to rotate clockwise relative to camera), and [0,-PI] for
		 * clockwise view orientation (note globe appears to rotate anti-clockwise relative to camera).
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		reorient_up_direction(
				const GPlatesMaths::real_t &reorientation_angle = 0,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the current look-at position "up" by the specified angle (in radians).
		 *
		 * The view and up directions are rotated by same rotation as look-at position.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_up(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true)
		{
			rotate_down(-angle, only_emit_if_changed);
		}

		/**
		 * Same as @a rotate_up but rotates "down".
		 */
		void
		rotate_down(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the current look-at position "left" by the specified angle (in radians).
		 *
		 * The view and up directions are rotated by same rotation as look-at position.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_left(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true)
		{
			rotate_right(-angle, only_emit_if_changed);
		}

		/**
		 * Same as @a rotate_left but rotates "right".
		 */
		void
		rotate_right(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the view "clockwise", around the current look-at position, by the specified angle (in radians).
		 *
		 * The view and up directions are rotated.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_clockwise(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true)
		{
			rotate_anticlockwise(-angle, only_emit_if_changed);
		}

		/**
		 * Same as @a rotate_clockwise but rotates "anti-clockwise".
		 */
		void
		rotate_anticlockwise(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);


		/**
		 * Returns the position on the globe where the specified camera ray intersects the globe, or
		 * none if does not intersect.
		 *
		 * Note that the ray could be outside the view frustum (not associated with a visible screen pixel),
		 * in which case the position on the globe is not visible either.
		 *
		 * The @a camera_ray can be obtained from mouse coordinates using @a get_camera_ray_at_window_coord.
		 */
		static
		boost::optional<GPlatesMaths::PointOnSphere>
		get_position_on_globe_at_camera_ray(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray);


		/**
		 * Returns the nearest point on globe horizon (visible circumference) to the specified camera ray.
		 *
		 * The @a camera_ray can be obtained from mouse coordinates using @a get_camera_ray_at_window_coord.
		 *
		 * NOTE: It is assumed that @a camera_ray is a valid camera ray (in that the ray origin is outside the globe)
		 * and that the ray's line does not intersect the globe.
		 */
		GPlatesMaths::PointOnSphere
		get_nearest_globe_horizon_position_at_camera_ray(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray) const;

		/**
		 * Returns the nearest point on sphere horizon (visible circumference) to the specified camera ray.
		 *
		 * The @a camera_ray can be obtained from mouse coordinates using @a get_camera_ray_at_window_coord.
		 *
		 * NOTE: It is assumed that @a camera_ray is such that its ray origin is outside the sphere and
		 * that the ray's line does not intersect the sphere.
		 *
		 * This method is similar to @a get_nearest_globe_horizon_position_at_camera_ray except it
		 * accepts an arbitrary sphere (any centre, any radius).
		 */
		GPlatesMaths::Vector3D
		get_nearest_sphere_horizon_position_at_camera_ray(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray,
				const GPlatesOpenGL::GLIntersect::Sphere &sphere) const;

		/**
		 * Returns the plane that separates the visible front half of globe from invisible rear half.
		 *
		 * Note that the plane normal defines the positive half space as the *front* of the globe
		 * (ie, the plane normal points toward the viewer).
		 *
		 * For orthographic viewing (where all camera rays are parallel, the plane simply divides
		 * the globe into two equal sized halves. And the plane normal is the opposite of view direction.
		 *
		 * For perspective viewing, the closer the camera eye is to the globe the more the plane moves
		 * toward the camera eye (since the visible front part of globe becomes smaller relative to the
		 * rear of the globe). The plane normal is the direction from globe centre to camera eye (ie, it is
		 * only aligned with the view direction when there is no camera tilt, when camera looks at globe centre).
		 */
		GPlatesOpenGL::GLIntersect::Plane
		get_front_globe_horizon_plane() const;

	protected:

		/**
		 * In perspective viewing mode, this is the distance from the eye position to the look-at
		 * position for the default zoom (ie, a zoom factor of 1.0).
		 */
		double
		get_perspective_viewing_distance_from_eye_to_look_at_for_at_default_zoom() const override;

		/**
		 * Return the radius of the sphere that bounds the globe.
		 *
		 * This includes a reasonable amount of extra space around the globe to include objects
		 * off the globe such as velocity arrows.
		 */
		double
		get_bounding_radius() const override
		{
			// 1.0 for the globe radius and 0.5 to include objects off the sphere such as rendered arrows.
			return 1.0 + 0.5;
		}

	private:

		struct ViewFrame
		{
			ViewFrame(
					const GPlatesMaths::PointOnSphere &look_at_position_,
					const GPlatesMaths::UnitVector3D &view_direction_,
					const GPlatesMaths::UnitVector3D &up_direction_) :
				look_at_position(look_at_position_),
				view_direction(view_direction_),
				up_direction(up_direction_)
			{  }

			GPlatesMaths::PointOnSphere look_at_position;
			GPlatesMaths::UnitVector3D view_direction;
			GPlatesMaths::UnitVector3D up_direction;
		};


		/**
		 * The view-space orientation.
		 *
		 * This rotates the view about an axis that passes through the origin.
		 *
		 * Note that we don't actually rotate the globe, instead rotating the view frame
		 * (look-at position and view/up directions) to achieve the same effect.
		 */
		GPlatesMaths::Rotation d_view_orientation;

		/**
		 * The angle that the view direction tilts.
		 *
		 * Zero angle implies looking straight down on the globe and 90 degrees means the view
		 * direction is looking tangentially at the globe surface (at look-at position).
		 */
		GPlatesMaths::real_t d_tilt_angle;

		/**
		 * The view frame (look-at position and view/up directions) is calculated/cached from the
		 * view orientation and view tilt.
		 */
		mutable boost::optional<ViewFrame> d_view_frame;


		/**
		 * Update @a d_view_frame using the current view orientation and view tilt.
		 */
		void
		cache_view_frame() const;

		/**
		 * The view frame needs updating.
		 */
		void
		invalidate_view_frame() const
		{
			d_view_frame = boost::none;
		}


		/**
		 * The initial position on the sphere that the camera looks at.
		 */
		static const GPlatesMaths::PointOnSphere INITIAL_LOOK_AT_POSITION;

		/**
		 * Initial view direction.
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_VIEW_DIRECTION;

		/**
		 * Initial up direction (orthogonal to view direction).
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_UP_DIRECTION;
	};
}

#endif // GPLATES_GUI_GLOBECAMERA_H
