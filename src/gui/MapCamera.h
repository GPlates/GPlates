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

#ifndef GPLATES_GUI_MAPCAMERA_H
#define GPLATES_GUI_MAPCAMERA_H

#include <utility>  // std::pair
#include <boost/optional.hpp>
#include <QObject>
#include <QPointF>

#include "GlobeProjectionType.h"

#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "opengl/GLIntersectPrimitives.h"


namespace GPlatesGui
{
	class MapProjection;
	class ViewportZoom;

	/**
	 * Handles viewing of the map (eye location, view direction, projection).
	 */
	class MapCamera :
			public QObject
	{
		Q_OBJECT

	public:


		explicit
		MapCamera(
				ViewportZoom &viewport_zoom);

		/**
		 * Switch between orthographic and perspective projections.
		 */
		void
		set_view_projection_type(
				GlobeProjection::Type view_projection_type);

		GlobeProjection::Type
		get_view_projection_type() const
		{
			return d_view_projection_type;
		}

		/**
		 * The position on the map that the view is looking at.
		 */
		const QPointF &
		get_look_at_position() const;

		/**
		 * The view direction.
		 *
		 * For perspective viewing this is from the eye position to the look-at position.
		 */
		const GPlatesMaths::UnitVector3D &
		get_view_direction() const;

		/**
		 * The 'up' vector for the view orientation.
		 */
		const GPlatesMaths::UnitVector3D &
		get_up_direction() const;


		/**
		 * The angle (in radians) that the view direction tilts.
		 *
		 * Zero angle implies looking straight down on the map plane (z=0). And PI/2 (90 degrees) means the view
		 * direction is looking tangentially at the map plane (at look-at position) and the up
		 * direction is pointing outward from the map plane (along z-axis).
		 */
		GPlatesMaths::real_t
		get_tilt_angle() const
		{
			return d_tilt_angle;
		}

		/**
		 * Set the angle (in radians) that the view direction tilts.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_tilt_angle(
				const GPlatesMaths::real_t &tilt_angle,
				bool only_emit_if_changed = true);

		/**
		 * Move the current look-at position to the specified look-at position along the line segment between them.
		 *
		 * Note that this does not change the current tilt angle.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		move_look_at_position(
				const QPointF &new_look_at_position,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the view so that the "up" direction points towards the North pole when @a reorientation_angle is zero.
		 *
		 * @a reorientation_angle, in radians, is [0, PI] for anti-clockwise view orientation with respect
		 * to North pole (note map appears to rotate clockwise relative to camera), and [0,-PI] for
		 * clockwise view orientation (note map appears to rotate anti-clockwise relative to camera).
		 */
		void
		reorient_up_direction(
				const GPlatesMaths::real_t &reorientation_angle = 0,
				bool only_emit_if_changed = true);

		/**
		 * Pan the current look-at position "up" by the specified angle (in radians).
		 */
		void
		pan_up(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);

		/**
		 * Same as @a pan_up but pans "down".
		 */
		void
		pan_down(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);

		/**
		 * Pan the current look-at position "left" by the specified angle (in radians).
		 */
		void
		pan_left(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);

		/**
		 * Same as @a pan_left but pans "right".
		 */
		void
		pan_right(
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
		 * Returns the left/right/bottom/top parameters of the 'glOrtho()' function, given the
		 * specified viewport aspect ratio.
		 *
		 * The current viewport zoom affects these parameters.
		 */
		void
		get_orthographic_left_right_bottom_top(
				const double &aspect_ratio,
				double &ortho_left,
				double &ortho_right,
				double &ortho_bottom,
				double &ortho_top) const;

		/**
		 * The camera (eye) location for perspective viewing.
		 *
		 * The current viewport zoom affects this eye location.
		 *
		 * NOTE: For orthographic viewing there is no real eye location since the view rays
		 *       are parallel and hence the eye location is at infinity.
		 */
		GPlatesMaths::Vector3D
		get_perspective_eye_position() const;

		/**
		 * Returns the field-of-view (in y-axis) parameters of the 'gluPerspective()' function,
		 * given the specified viewport aspect ratio.
		 */
		void
		get_perspective_fovy(
				const double &aspect_ratio,
				double &fovy_degrees) const;


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
		 * For a perspective projection, the ray origin is at the camera eye (@a get_perspective_eye_position).
		 *
		 * For an orthographic projection, all view rays are parallel and so there's no real eye position.
		 * Instead the ray origin is placed an arbitrary distance (currently 1.0) from the look-at position
		 * (more accurately the distance from the plane, orthogonal to view direction, containing look-at position)
		 * back along the view direction.
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
		 * For a perspective projection, the ray origin is at the camera eye (@a get_perspective_eye_position).
		 *
		 * For an orthographic projection, all view rays are parallel and so there's no real eye position.
		 * Instead the ray origin is placed an arbitrary distance (currently 1.0) from the specified position
		 * back along the view direction.
		 *
		 * NOTE: A precondition, for perspective projection, is the camera eye must not coincide with the specified position.
		 */
		GPlatesOpenGL::GLIntersect::Ray
		get_camera_ray_at_position(
				const GPlatesMaths::Vector3D &position) const;


		/**
		 * Returns the position on the map plane (z=0) where the specified camera ray intersects, or
		 * none if does not intersect.
		 *
		 * Note that the ray could be outside the view frustum (not associated with a visible screen pixel),
		 * in which case the position on the map plane is not visible either.
		 *
		 * The @a camera_ray can be obtained from mouse coordinates using @a get_camera_ray_at_window_coord.
		 */
		static
		boost::optional<QPointF>
		get_position_on_map_at_camera_ray(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray);

	Q_SIGNALS:
	
		/**
		 * This signal is emitted when the camera changes.
		 */
		void
		camera_changed();

	private Q_SLOTS:

		void
		handle_zoom_changed();

	private:

		ViewportZoom &d_viewport_zoom;

		GlobeProjection::Type d_view_projection_type;

		/**
		 * The angle that the view direction tilts.
		 *
		 * Zero angle implies looking straight down on the map and 90 degrees means the view
		 * direction is looking tangentially at the map plane (at look-at position).
		 */
		GPlatesMaths::real_t d_tilt_angle;


		/**
		 * Ratio of the map extent in the longitude direction divided by the latitude direction.
		 *
		 * This varies depending on the map projection so we'll just use the Rectangular projection as a basis.
		 */
		static constexpr double MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE = 2.0;
		//! Extent of map projection in longitude direction (just using the Rectangular projection as a basis).
		static constexpr double MAP_LONGITUDE_EXTENT_IN_MAP_SPACE = 360.0;
		//! Extent of map projection in latitude direction (just using the Rectangular projection as a basis).
		static constexpr double MAP_LATITUDE_EXTENT_IN_MAP_SPACE =
				MAP_LONGITUDE_EXTENT_IN_MAP_SPACE / MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE;

		/**
		 * At the initial zoom, and untilted view, this creates a little space between the map boundary and the viewport.
		 *
		 * When the viewport is resized, the map will be scaled accordingly.
		 * The value of this constant is purely cosmetic.
		 */
		static const double FRAMING_RATIO_OF_MAP_IN_VIEWPORT;

		/**
		 * Angle of field-of-view for perspective projection.
		 */
		static const double PERSPECTIVE_FIELD_OF_VIEW_DEGREES;

		//! Tangent of half of field-of-view angle.
		static const double TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW;

		/**
		 * How far to nudge or pan the camera when incrementally moving the camera.
		 *
		 * This is in map units (in the map plane).
		 */
		static const double NUDGE_CAMERA_IN_MAP_SPACE;

		/**
		 * The initial position on the map that the camera looks at.
		 */
		static const QPointF INITIAL_LOOK_AT_POSITION;

		/**
		 * Initial view direction.
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_VIEW_DIRECTION;

		/**
		 * Initial up direction (orthogonal to view direction).
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_UP_DIRECTION;

		/**
		 * In perspective viewing mode, this is the distance from the eye position to the look-at
		 * position for the default zoom (ie, a zoom factor of 1.0).
		 */
		static
		double
		get_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom();

		static
		double
		calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom();


		void
		pan(
				const QPointF &delta,
				bool only_emit_if_changed);

		double
		get_perspective_tan_half_fovy(
				const double &aspect_ratio) const;
	};
}

#endif // GPLATES_GUI_MAPCAMERA_H
