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


		GlobeCamera(
				ViewportProjection::Type viewport_projection,
				ViewportZoom &viewport_zoom);

		/**
		 * The position on the globe that the view is looking at.
		 */
		GPlatesMaths::PointOnSphere
		get_look_at_position_on_globe() const override;

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
		move_look_at_position_on_globe(
				const GPlatesMaths::PointOnSphere &look_at_position_on_globe,
				bool only_emit_if_changed = true) override;


		/**
		 * Same as @a get_look_at_position_on_globe but returned as a Vector3D.
		 */
		GPlatesMaths::Vector3D
		get_look_at_position() const override;

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
		get_tilt_angle() const override
		{
			return d_tilt_angle;
		}

		/**
		 * Set the angle (in radians) that the view direction tilts.
		 *
		 * The tilt angle is clamped to the range [0, PI/2].
		 *
		 * Note that this does not change the current view orientation (returned by @a get_view_orientation).
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_tilt_angle(
				GPlatesMaths::real_t tilt_angle,
				bool only_emit_if_changed = true) override;


		/**
		 * Rotate the view so that the "up" direction points towards the North pole when @a reorientation_angle is zero.
		 *
		 * @a reorientation_angle, in radians, is [0, PI] for anti-clockwise view orientation with respect
		 * to North pole (note map appears to rotate clockwise relative to camera), and [0,-PI] for
		 * clockwise view orientation (note map appears to rotate anti-clockwise relative to camera).
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		reorient_up_direction(
				GPlatesMaths::real_t reorientation_angle = 0,
				bool only_emit_if_changed = true) override;


		/**
		 * Pan (rotate) the current look-at position "up" by the specified angle (in radians).
		 *
		 * If @a scale_by_viewport_zoom is true (defaults to true) then scales the angle by the current viewport zoom
		 * (divide angle by the viewport zoom factor so that there's less panning for zoomed-in views).
		 *
		 * The view and up directions are rotated by same rotation as look-at position.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		pan_up(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true) override;

		/**
		 * Pan (rotate) the current look-at position "right" by the specified angle (in radians).
		 *
		 * If @a scale_by_viewport_zoom is true (defaults to true) then scales the angle by the current viewport zoom
		 * (divide angle by the viewport zoom factor so that there's less panning for zoomed-in views).
		 *
		 * The view and up directions are rotated by same rotation as look-at position.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		pan_right(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true) override;


		/**
		 * Rotate the view "anticlockwise", around the current look-at position, by the specified angle (in radians).
		 *
		 * The view and up directions are rotated.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_anticlockwise(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true) override;


		/**
		 * Tilt the view "more" (view is more tilted) by the specified angle (in radians).
		 *
		 * The view and up directions are tilted.
		 *
		 * Note that this does not change the current rotation angle.
		 */
		void
		tilt_more(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true) override;


		/**
		 * Returns the position on the globe at the specified window coordinate.
		 *
		 * Window coordinates are typically in the range [0, window_width] and [0, window_height]
		 * where (0, 0) is bottom-left and (window_width, window_height) is top-right of window.
		 * Note that we use the OpenGL convention where 'window_x = 0' is the bottom of the window.
		 * But in Qt it means top, so a Qt mouse y coordinate (for example) needs be inverted
		 * before passing to this method.
		 *
		 * Note that either/both window coordinate could be outside the range[0, window_width] and
		 * [0, window_height], in which case the position on the globe is not visible either.
		 *
		 * Note: This is equivalent to calling @a get_camera_ray_at_window_coord to get a ray
		 *       followed by calling @a get_position_on_map_at_camera_ray with that ray.
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		get_position_on_globe_at_window_coord(
				double window_x,
				double window_y,
				int window_width,
				int window_height) const
		{
			// See if camera ray intersects the globe.
			return get_position_on_globe_at_camera_ray(
					// Project screen coordinates into a ray into 3D scene (containing globe)...
					get_camera_ray_at_window_coord(window_x, window_y, window_width, window_height));
		}


		/**
		 * Returns the position on the globe where the specified camera ray intersects the globe, or
		 * none if does not intersect.
		 *
		 * Note that the ray could be outside the view frustum (not associated with a visible screen pixel),
		 * in which case the position on the globe is not visible either.
		 *
		 * The @a camera_ray can be obtained from mouse coordinates using @a get_camera_ray_at_window_coord.
		 *
		 * Note: For *orthographic* viewing the negative or positive side of the ray can intersect the globe
		 *       (since the view rays are parallel and so if we ignore the near/far clip planes then
		 *       everything in the infinitely long rectangular prism is visible).
		 *       Whereas for *perspective* viewing only the positive side of the ray can intersect the globe
		 *       (since the view rays emanate/diverge from a single eye location and so, ignoring the
		 *       near/far clip planes, only the front infinitely long pyramid with apex at eye is visible).
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		get_position_on_globe_at_camera_ray(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray) const;


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
		 * Returns the aspect ratio that is optimally suited to the globe view.
		 *
		 * This is just 1.0 since the globe is circular.
		 *
		 * Note that this applies to both orthographic and perspective views.
		 */
		double
		get_optimal_aspect_ratio() const override
		{
			return 1.0;
		}

		/**
		 * In orthographic viewing mode, this is *half* the distance between top and bottom clip planes of
		 * the orthographic view frustum (rectangular prism) at default zoom (ie, a zoom factor of 1.0).
		 */
		double
		get_orthographic_half_height_extent_at_default_zoom() const override
		{
			// The globe radius is 1.0, so this is slightly larger than the globe radius (due to framing ratio).
			// This means the globe diameter (twice radius) will fit just inside the top and bottom clip planes.
			return FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT;
		}

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
		mutable boost::optional<ViewFrame> d_cached_view_frame;


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
			d_cached_view_frame = boost::none;
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
