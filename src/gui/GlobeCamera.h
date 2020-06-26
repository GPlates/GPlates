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

#include <boost/optional.hpp>
#include <QObject>

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
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * At the initial zoom, the smaller dimension of the viewport will be @a FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT
		 * times the diameter of the globe. This creates a little space between the globe circumference and the viewport.
		 * When the viewport is resized, the globe will be scaled accordingly.
		 *
		 * The value of this constant is purely cosmetic.
		 */
		static const double FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT;


		explicit
		GlobeCamera(
				ViewportZoom &viewport_zoom);

		/**
		 * Switch between orthographic and perspective projections.
		 */
		void
		set_projection_type(
				GlobeProjection::Type projection_type);

		GlobeProjection::Type
		get_projection_type() const
		{
			return d_projection_type;
		}

		/**
		 * The position on the globe that the view is looking at.
		 */
		const GPlatesMaths::PointOnSphere &
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
		 * This sets the inverse orientation set by @a set_globe_orientation_relative_to_view.
		 */
		void
		set_view_orientation(
				const GPlatesMaths::Rotation &view_orientation);

		/**
		 * Set the orientation of the fixed globe relative to the moving camera (excluding tilt).
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
		 * Zero angle implies looking straight down on the globe and PI/2 (90 degrees) means the view
		 * direction is looking tangentially at the globe surface (at look-at position).
		 */
		GPlatesMaths::real_t
		get_tilt_angle() const
		{
			return d_tilt_angle;
		}

		/**
		 * Set the angle (in radians) that the view direction tilts.
		 */
		void
		set_tilt_angle(
				const GPlatesMaths::real_t &tilt_angle);

		/**
		 * Rotate the current look-at position to the specified look-at position along the
		 * great circle arc between them.
		 *
		 * The view and up directions are rotated by same rotation.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		move_look_at_position(
				const GPlatesMaths::PointOnSphere &new_look_at_position);

		/**
		 * Rotate the current look-at position "up" by the specified angle (in radians).
		 *
		 * The view and up directions are rotated by same rotation.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_up(
				const GPlatesMaths::real_t &angle)
		{
			rotate_down(-angle);
		}

		/**
		 * Same as @a rotate_up but rotates "down".
		 */
		void
		rotate_down(
				const GPlatesMaths::real_t &angle);

		/**
		 * Rotate the current look-at position "left" by the specified angle (in radians).
		 *
		 * The view and up directions are rotated by same rotation.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_left(
				const GPlatesMaths::real_t &angle)
		{
			rotate_right(-angle);
		}

		/**
		 * Same as @a rotate_left but rotates "right".
		 */
		void
		rotate_right(
				const GPlatesMaths::real_t &angle);

		/**
		 * Rotate the view "clockwise", around the current look-at position, by the specified angle (in radians).
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_clockwise(
				const GPlatesMaths::real_t &angle)
		{
			rotate_anticlockwise(-angle);
		}

		/**
		 * Same as @a rotate_clockwise but rotates "anti-clockwise".
		 */
		void
		rotate_anticlockwise(
				const GPlatesMaths::real_t &angle);


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
		 * Returns the field-of-view (in y-axis) parameters of the 'gluPerspective()' function,
		 * given the specified viewport aspect ratio.
		 */
		void
		get_perspective_fovy(
				const double &aspect_ratio,
				double &fovy_degrees) const;


		/**
		 * Returns ray from camera to the specified position on the globe.
		 *
		 * For a perspective projection, the ray origin is at the camera eye (@a get_perspective_eye_position).
		 *
		 * For an orthographic projection, all view rays are parallel and so there's no real eye position.
		 * Instead the ray origin is placed an arbitrary distance (currently 1.0) from the specified position
		 * back along the view direction.
		 */
		GPlatesOpenGL::GLIntersect::Ray
		get_camera_ray_at_position_on_globe(
				const GPlatesMaths::UnitVector3D &pos_on_globe);

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


		ViewportZoom &d_viewport_zoom;

		GlobeProjection::Type d_projection_type;

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
		 * Angle of field-of-view for perspective projection.
		 */
		static const double PERSPECTIVE_FIELD_OF_VIEW_DEGREES;

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
	};
}

#endif // GPLATES_GUI_GLOBECAMERA_H
