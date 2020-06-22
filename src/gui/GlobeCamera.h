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


		/**
		 * Mouse drag modes.
		 */
		enum MouseDragMode
		{
			// Rotate along great circle arcs (axes) as mouse is dragged across the globe...
			DRAG_NORMAL,
			// Rotate about the axis (from globe centre) through the look-at position on globe (centre of viewport)...
			DRAG_ROTATE,
			// Tilt the view around the axis (perpendicular to view and up directions) passing tangentially through the look-at position on globe...
			DRAG_TILT,
			// Rotate and tilt using same mouse drag motion...
			DRAG_ROTATE_AND_TILT
		};


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
		 * The view direction.
		 *
		 * For perspective viewing this is from the eye position to the look-at position.
		 */
		const GPlatesMaths::UnitVector3D &
		get_view_direction() const
		{
			return d_view_direction;
		}

		/**
		 * The position on the globe that the view is looking at.
		 */
		const GPlatesMaths::UnitVector3D &
		get_look_at_position() const
		{
			return d_look_at_position;
		}

		/**
		 * The 'up' vector for the view orientation.
		 */
		const GPlatesMaths::UnitVector3D &
		get_up_direction() const
		{
			return d_up_direction;
		}

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
		 * Start a mouse drag, using the specified mode, at the specified position on the globe.
		 *
		 * Subsequent calls to @a update_drag will use the specified drag mode.
		 */
		void
		start_drag(
				MouseDragMode mouse_drag_mode,
				const GPlatesMaths::PointOnSphere &mouse_pos_on_globe);

		/**
		 * Update the camera view using the specified mouse drag position on the globe.
		 *
		 * This uses the drag mode specified in the last call to @a start_drag.
		 *
		 * Depending on the drag mode, this can update the view direction, up direction, look-at position and
		 * perspective eye position.
		 */
		void
		update_drag(
				const GPlatesMaths::PointOnSphere &mouse_pos_on_globe);

		void
		end_drag();

	Q_SIGNALS:
	
		/**
		 * This signal should only be emitted if the camera is actually different to what it was.
		 */
		void
		camera_changed();

	private Q_SLOTS:

		void
		handle_zoom_changed();

	private:

		/**
		 * Information generated in @a start_drag and used in subsequent calls to @a update_drag.
		 */
		struct MouseDragInfo
		{
			MouseDragInfo(
					MouseDragMode mode_,
					const GPlatesMaths::UnitVector3D &start_mouse_pos_on_globe_,
					const GPlatesMaths::UnitVector3D &start_look_at_pos_on_globe_,
					const GPlatesMaths::UnitVector3D &start_view_direction_,
					const GPlatesMaths::UnitVector3D &start_up_direction_) :
				mode(mode_),
				start_mouse_pos_on_globe(start_mouse_pos_on_globe_),
				start_look_at_position(start_look_at_pos_on_globe_),
				start_view_direction(start_view_direction_),
				start_up_direction(start_up_direction_),
				view_rotation_relative_to_start(GPlatesMaths::Rotation::create_identity_rotation())
			{  }

			MouseDragMode mode;

			GPlatesMaths::UnitVector3D start_mouse_pos_on_globe;
			GPlatesMaths::UnitVector3D start_look_at_position;
			GPlatesMaths::UnitVector3D start_view_direction;
			GPlatesMaths::UnitVector3D start_up_direction;

			GPlatesMaths::Rotation view_rotation_relative_to_start;

			boost::optional<GPlatesMaths::real_t> intersect_cylinder_radius;
			boost::optional<GPlatesMaths::UnitVector3D> start_cylinder_axis;
			boost::optional<GPlatesMaths::real_t> start_tilt_angle;
			boost::optional<GPlatesMaths::real_t> start_spin_angle;
		};


		void
		start_drag_normal();

		void
		update_drag_normal(
				const GPlatesMaths::UnitVector3D &mouse_pos_on_globe);


		void
		start_drag_rotate();

		void
		update_drag_rotate(
				const GPlatesMaths::UnitVector3D &mouse_pos_on_globe);


		void
		start_drag_tilt();

		void
		update_drag_tilt(
				const GPlatesMaths::UnitVector3D &mouse_pos_on_globe);


		void
		start_drag_rotate_and_tilt();

		void
		update_drag_rotate_and_tilt(
				const GPlatesMaths::UnitVector3D &mouse_pos_on_globe);


		ViewportZoom &d_viewport_zoom;

		GlobeProjection::Type d_projection_type;

		GPlatesMaths::UnitVector3D d_look_at_position;
		GPlatesMaths::UnitVector3D d_view_direction;
		GPlatesMaths::UnitVector3D d_up_direction;

		/**
		 * In perspective viewing mode, this is the distance from the eye position to the look-at
		 * position for the default zoom (ie, a zoom factor of 1.0).
		 */
		double d_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom;

		/**
		 * Info generated in @a start_drag and used subsequently in @a update_drag.
		 */
		boost::optional<MouseDragInfo> d_mouse_drag_info;


		/**
		 * Angle of field-of-view for perspective projection.
		 */
		static const double PERSPECTIVE_FIELD_OF_VIEW_DEGREES;

		/**
		 * The initial position on the sphere that the camera looks at.
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_LOOK_AT_POSITION;

		/**
		 * Initial view direction.
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_VIEW_DIRECTION;

		/**
		 * Initial up direction (orthogonal to view direction).
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_UP_DIRECTION;

		static
		double
		calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom();
	};
}

#endif // GPLATES_GUI_GLOBECAMERA_H
