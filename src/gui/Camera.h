/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2021 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_CAMERA_H
#define GPLATES_GUI_CAMERA_H

#include <boost/optional.hpp>
#include <QObject>
#include <QPointF>

#include "ViewportProjectionType.h"

#include "maths/PointOnSphere.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "opengl/GLIntersectPrimitives.h"
#include "opengl/GLMatrix.h"


namespace GPlatesGui
{
	class ViewportZoom;

	/**
	 * Base class for globe and map cameras.
	 */
	class Camera :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * Default amount to pan, rotate or tilt the camera (in radians) in 'pan_*()', 'rotate_*()' and 'tilt_*()' methods.
		 *
		 * Note: Both *globe* and *map* cameras use radians. For the *map* camera you can think of the map extents
		 *       (that bound the map projection) as being a rectangle of roughly 360 degrees horizontally and 180 degrees vertically.
		 */
		static const double DEFAULT_PAN_ROTATE_TILT_RADIANS;


		virtual
		~Camera()
		{  }


		/**
		 * Switch between orthographic and perspective view projections.
		 */
		void
		set_viewport_projection(
				ViewportProjection::Type viewport_projection);

		/**
		 * Return the view projection (orthographic or perspective).
		 */
		ViewportProjection::Type
		get_viewport_projection() const
		{
			return d_viewport_projection;
		}


		/**
		 * Get the view transform to pass to OpenGL.
		 */
		GPlatesOpenGL::GLMatrix
		get_view_transform() const;

		/**
		 * Get the projection transform to pass to OpenGL.
		 *
		 * Note: The projection transform is 'orthographic' or 'perspective', and hence is only affected
		 *       by the viewport *aspect ratio*, so it's independent of whether we're using device pixels or
		 *       device *independent* pixels.
		 */
		GPlatesOpenGL::GLMatrix
		get_projection_transform(
				const double &viewport_aspect_ratio) const;


		/**
		 * The position on the globe (unit sphere) that the view is looking at.
		 *
		 * Note: For the globe this is the same as @a get_look_at_position.
		 *       For the map this is equivalent to the map-projection inverse of @a get_look_at_position
		 *       (ie, the actual look-at position on the z=0 map plane inverse projected back onto the globe).
		 */
		virtual
		GPlatesMaths::PointOnSphere
		get_look_at_position_on_globe() const = 0;

		/**
		 * Move the current look-at position to the specified look-at position on the globe.
		 *
		 * For the globe this rotates the view along the great circle arc (on the globe) between the current and
		 * new look-at positions. The view and up directions are rotated by same rotation as look-at position.
		 *
		 * For the map this pans the view along the line segment (on the map plane) between the
		 * current and new look-at positions. The view and up directions are not changed.
		 *
		 * Note that this does not change the current tilt angle (in globe or map view).
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		virtual
		void
		move_look_at_position_on_globe(
				const GPlatesMaths::PointOnSphere &look_at_position_on_globe,
				bool only_emit_if_changed = true) = 0;


		/**
		 * The position in 3D space that the view is looking at.
		 *
		 * Note: For the globe this is on the unit sphere.
		 *       For the map this is the map-projected position on the z=0 plane
		 *       (ie, returned position only has non-zero x and y).
		 */
		virtual
		GPlatesMaths::Vector3D
		get_look_at_position() const = 0;

		/**
		 * The view direction.
		 *
		 * This is from the eye position to the look-at position.
		 */
		virtual
		GPlatesMaths::UnitVector3D
		get_view_direction() const = 0;

		/**
		 * The 'up' vector for the view orientation.
		 */
		virtual
		GPlatesMaths::UnitVector3D
		get_up_direction() const = 0;

		/**
		 * The camera (eye) location.
		 *
		 * The eye location to the look-at position (@a get_look_at_position) is along the view direction.
		 *
		 * Note: For perspective viewing the current viewport zoom affects this eye location.
		 *
		 * Note: For orthographic viewing there is no real eye location since the view rays are
		 *       parallel and hence the eye location can be anywhere along the view direction
		 *       (including at infinity). This is because the view rays are parallel and hence only
		 *       the direction matters (not the position). However since the eye position does affect the
		 *       near/far clip plane distances we arbitrarily place the eye position *on* the near clip plane
		 *       (so it has a view/eye space z value of zero). And the near/far distances are such that
		 *       they encompass the bounds of the globe or map.
		 */
		GPlatesMaths::Vector3D
		get_eye_position() const;


		/**
		 * The angle (in radians) that the view direction tilts.
		 *
		 * The tilt angle is clamped to the range [0, PI/2].
		 *
		 * Zero angle implies looking straight down on the globe (or map plane). And PI/2 (90 degrees)
		 * means the view direction is looking tangentially at the look-at position (on globe surface or
		 * the map plane) and the up direction is pointing outward from the globe (or map plane along z-axis).
		 */
		virtual
		GPlatesMaths::real_t
		get_tilt_angle() const = 0;

		/**
		 * Set the angle (in radians) that the view direction tilts.
		 *
		 * The tilt angle is clamped to the range [0, PI/2].
		 *
		 * Note that this does not change the current view orientation (returned by @a get_view_orientation).
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		virtual
		void
		set_tilt_angle(
				GPlatesMaths::real_t tilt_angle,
				bool only_emit_if_changed = true) = 0;


		/**
		 * Rotate the view so that the "up" direction points towards the North pole when @a reorientation_angle is zero.
		 *
		 * @a reorientation_angle, in radians, is [0, PI] for anti-clockwise view orientation with respect
		 * to North pole (note map appears to rotate clockwise relative to camera), and [0,-PI] for
		 * clockwise view orientation (note map appears to rotate anti-clockwise relative to camera).
		 *
		 * Note that this does not change the current tilt angle.
		 */
		virtual
		void
		reorient_up_direction(
				GPlatesMaths::real_t reorientation_angle = 0,
				bool only_emit_if_changed = true) = 0;


		/**
		 * Pan the current look-at position "up" by the specified angle (in radians).
		 *
		 * If @a scale_by_viewport_zoom is true (defaults to true) then scales the angle by the current viewport zoom
		 * (divide angle by the viewport zoom factor so that there's less panning for zoomed-in views).
		 */
		virtual
		void
		pan_up(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true) = 0;

		/**
		 * Same as @a pan_up but pans "down".
		 */
		void
		pan_down(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true)
		{
			pan_up(-angle, scale_by_viewport_zoom, only_emit_if_changed);
		}

		/**
		 * Pan the current look-at position "right" by the specified angle (in radians).
		 *
		 * If @a scale_by_viewport_zoom is true (defaults to true) then scales the angle by the current viewport zoom
		 * (divide angle by the viewport zoom factor so that there's less panning for zoomed-in views).
		 */
		virtual
		void
		pan_right(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true) = 0;

		/**
		 * Same as @a pan_right but pans "left".
		 */
		void
		pan_left(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true)
		{
			pan_right(-angle, scale_by_viewport_zoom, only_emit_if_changed);
		}


		/**
		 * Rotate the view "anticlockwise", around the current look-at position, by the specified angle (in radians).
		 *
		 * The view and up directions are rotated.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		virtual
		void
		rotate_anticlockwise(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true) = 0;

		/**
		 * Same as @a rotate_anticlockwise but rotates "clockwise".
		 */
		void
		rotate_clockwise(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true)
		{
			rotate_anticlockwise(-angle, only_emit_if_changed);
		}


		/**
		 * Tilt the view "more" (view is more tilted) by the specified angle (in radians).
		 *
		 * The view and up directions are tilted.
		 *
		 * Note that this does not change the current rotation angle.
		 */
		virtual
		void
		tilt_more(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true) = 0;

		/**
		 * Same as @a tilt_more but tilts "less" (view is less tilted).
		 */
		void
		tilt_less(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true)
		{
			tilt_more(-angle, only_emit_if_changed);
		}


		/**
		 * Returns ray from camera eye into the projected scene at the specified window coordinate.
		 *
		 * Window coordinates are typically in the range [0, window_width] and [0, window_height]
		 * where (0, 0) is bottom-left and (window_width, window_height) is top-right of window.
		 * Note that we use the OpenGL convention where 'window_x = 0' is the bottom of the window.
		 * But in Qt it means top, so a Qt mouse y coordinate (for example) needs be inverted
		 * before passing to this method.
		 *
		 * Note that either/both window coordinate could be outside the range[0, window_width] and
		 * [0, window_height], in which case the ray is not associated with a window pixel inside the
		 * viewport (visible projected scene).
		 *
		 * The ray origin is at the camera eye (@a get_eye_position).
		 * Note that, for orthographic viewing, the ray origin isn't actually at @a get_eye_position but
		 * it's in the same view plane as it (ie, plane passing through @a get_eye_position with plane normal
		 * @a get_view_direction). And this means the entire scene (globe or map) will always be in *front*
		 * of the ray (see the description of @a get_eye_position for the detailed reason).
		 */
		GPlatesOpenGL::GLIntersect::Ray
		get_camera_ray_at_window_coord(
				double window_x,
				double window_y,
				int window_width,
				int window_height) const;


		/**
		 * Returns ray from camera eye to the specified arbitrary position.
		 *
		 * Note that the position could be outside the view frustum, in which case the ray
		 * is not associated with a screen pixel inside the viewport (visible projected scene).
		 *
		 * The ray origin is at the camera eye (@a get_eye_position).
		 * Note that, for orthographic viewing, the ray origin isn't actually at @a get_eye_position but
		 * it's in the same view plane as it (ie, plane passing through @a get_eye_position with plane normal
		 * @a get_view_direction). And this means the entire scene (globe or map) will always be in *front*
		 * of the ray (see the description of @a get_eye_position for the detailed reason).
		 *
		 * NOTE: A precondition, for perspective projection, is the specified position must not coincide
		 *       with the camera eye (which would prevent the ray direction from being found).
		 */
		GPlatesOpenGL::GLIntersect::Ray
		get_camera_ray_at_position(
				const GPlatesMaths::Vector3D &position) const;


		/**
		 * Returns the window coordinates that the specified arbitrary position projects onto.
		 *
		 * Window coordinates are typically in the range [0, window_width] and [0, window_height]
		 * where (0, 0) is bottom-left and (window_width, window_height) is top-right of window.
		 * Note that we use the OpenGL convention where 'window_x = 0' is the bottom of the window.
		 * But in Qt it means top, so if a Qt mouse y-coordinate is desired then the returned
		 * OpenGL y-coordinate needs be inverted.
		 *
		 * Note that either/both window coordinate could be outside the range[0, window_width] and
		 * [0, window_height], in which case the specified position is not visible (does not
		 * project inside the viewport).
		 *
		 * Returns none if the projection is perspective and the specified position is on the plane
		 * containing the camera eye with plane normal equal to view direction.
		 */
		boost::optional<QPointF>
		get_window_coord_at_position(
				const GPlatesMaths::Vector3D &position,
				int window_width,
				int window_height) const;

	Q_SIGNALS:
	
		/**
		 * This signal is emitted when the camera changes.
		 */
		void
		camera_changed();

	private Q_SLOTS:

		void
		handle_zoom_changed();

	protected:

		Camera(
				ViewportProjection::Type viewport_projection,
				ViewportZoom &viewport_zoom);

		const ViewportZoom &
		get_viewport_zoom() const
		{
			return d_viewport_zoom;
		}


		/**
		 * The camera (eye) location for orthographic viewing.
		 *
		 * For orthographic viewing there is no single eye location (like perspective viewing) since
		 * the view rays are parallel and hence never converge on a single point.
		 * This is also why the eye position depends on the look-at position.
		 * Also since the eye position does affect the near/far clip plane distances we arbitrarily
		 * place the eye position *on* the near clip plane (so it has a view/eye space z value of zero).
		 * And the near/far distances are such that they encompass the bounds of the globe or map.
		 * One reason for this is the eye position can then be used as a ray origin such that the
		 * entire scene (globe or map) will always be in *front* of that ray.
		 *
		 * The eye location to the specified look-at position is along the positive or negative view direction.
		 * Note that this means the specified look-at position can be behind the eye position
		 * (but this does not change the view direction).
		 */
		GPlatesMaths::Vector3D
		get_orthographic_eye_position(
				const GPlatesMaths::Vector3D &look_at_position) const;

		/**
		 * The camera (eye) location for perspective viewing.
		 *
		 * Unlike orthographic viewing, with perspective viewing the view rays all converge at a single point.
		 *
		 * Note: The current viewport zoom affects this eye location.
		 */
		GPlatesMaths::Vector3D
		get_perspective_eye_position() const;


		/**
		 * Returns the left/right/bottom/top parameters of the 'glOrtho()' function, given the
		 * specified viewport aspect ratio.
		 *
		 * Note: The current viewport zoom affects these parameters.
		 */
		void
		get_orthographic_left_right_bottom_top(
				const double &aspect_ratio,
				double &ortho_left,
				double &ortho_right,
				double &ortho_bottom,
				double &ortho_top) const;

		/**
		 * Returns the field-of-view (in y-axis) parameters of the 'gluPerspective()' function,
		 * given the specified viewport aspect ratio.
		 */
		double
		get_perspective_fovy(
				const double &aspect_ratio) const;

		/**
		 * Same as @a get_perspective_fovy except returns tan(fovy/2) instead of fovy.
		 */
		double
		get_perspective_tan_half_fovy(
				const double &aspect_ratio) const;

		/**
		 * Returns the aspect ratio that is optimally suited to the globe or map view.
		 *
		 * For the globe this is just 1.0 since it is circular.
		 * For the map this is typically 2.0 since in the default orientation in Rectangular projection
		 * the width is twice the height.
		 *
		 * Note that this applies to both orthographic and perspective views.
		 */
		virtual
		double
		get_optimal_aspect_ratio() const = 0;

		/**
		 * In orthographic viewing mode, this is *half* the distance between top and bottom clip planes of
		 * the orthographic view frustum (rectangular prism) at default zoom (ie, a zoom factor of 1.0).
		 */
		virtual
		double
		get_orthographic_half_height_extent_at_default_zoom() const = 0;

		/**
		 * In perspective viewing mode, this is the distance from the eye position to the look-at
		 * position for the default zoom (ie, a zoom factor of 1.0).
		 */
		virtual
		double
		get_perspective_viewing_distance_from_eye_to_look_at_for_at_default_zoom() const = 0;

		/**
		 * Return the radius of the sphere that bounds the globe or map.
		 *
		 * This includes a reasonable amount of extra space around the globe or map to include objects
		 * off the globe or map such as velocity arrows.
		 *
		 * Note: For a map view the bounds depend on the current map projection.
		 */
		virtual
		double
		get_bounding_radius() const = 0;


		/**
		 * Angle of field-of-view for perspective projection.
		 */
		static const double PERSPECTIVE_FIELD_OF_VIEW_DEGREES;

		//! Tangent of half of field-of-view angle.
		static const double TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW;

	private:

		/**
		 * The viewport projection (orthographic or perspective).
		 */
		ViewportProjection::Type d_viewport_projection;

		ViewportZoom &d_viewport_zoom;
	};
}

#endif // GPLATES_GUI_CAMERA_H
