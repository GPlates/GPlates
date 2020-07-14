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

#ifndef GPLATES_VIEW_OPERATIONS_GLOBEVIEWOPERATION_H
#define GPLATES_VIEW_OPERATIONS_GLOBEVIEWOPERATION_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"


namespace GPlatesGui
{
	class GlobeCamera;
}

namespace GPlatesViewOperations
{
	/**
	 * Enables users to drag the globe view to a new orientation/tilt.
	 */
	class GlobeViewOperation :
			private boost::noncopyable
	{
	public:

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
		GlobeViewOperation(
				GPlatesGui::GlobeCamera &globe_camera);

		/**
		 * Start a mouse drag, using the specified mode, at the specified position on the globe.
		 *
		 * Subsequent calls to @a update_drag will use the specified drag mode.
		 *
		 * Note that @a initial_mouse_pos_on_globe is the nearest position *on* the globe when
		 * the mouse coordinate is *off* the globe.
		 */
		void
		start_drag(
				MouseDragMode mouse_drag_mode,
				const GPlatesMaths::PointOnSphere &initial_mouse_pos_on_globe,
				double initial_mouse_screen_x,
				double initial_mouse_screen_y,
				int screen_width,
				int screen_height);

		/**
		 * Update the camera view using the specified mouse drag position on the globe.
		 *
		 * This uses the drag mode specified in the last call to @a start_drag.
		 *
		 * If @a end_of_drag is true then this is the last update of the drag, and hence @a start_drag
		 * must be called before the next @a update_drag.
		 *
		 * Depending on the drag mode, this can update the view direction, up direction, look-at position and
		 * perspective eye position.
		 *
		 * Note that @a mouse_pos_on_globe is the nearest position *on* the globe when
		 * the mouse coordinate is *off* the globe.
		 */
		void
		update_drag(
				const GPlatesMaths::PointOnSphere &mouse_pos_on_globe,
				double mouse_screen_x,
				double mouse_screen_y,
				int screen_width,
				int screen_height,
				bool end_of_drag);

		/**
		 * Returns true if currently in a drag, where @a start_drag has been called but the last
		 * @a update_drag ('end_of_drag' is true) has not yet been called.
		 *
		 * Note that this means false is returned *during* the last update
		 * (ie, during the call to @a update_drag where 'end_of_drag' is true).
		 */
		bool
		in_drag() const
		{
			return d_in_drag_operation;
		}

	private:

		/**
		 * The method used to tilt the view.
		 */
		enum class TiltMethod
		{
			USE_CYLINDER_FRONT_INTERSECTION,
			USE_CYLINDER_BACK_INTERSECTION,
			DONT_USE_CYLINDER_INTERSECTIONS
		};

		/**
		 * Information generated in @a start_drag and used in subsequent calls to @a update_drag.
		 */
		struct MouseDragInfo
		{
			MouseDragInfo(
					MouseDragMode mode_,
					const GPlatesMaths::UnitVector3D &start_mouse_pos_on_globe_,
					const double &start_mouse_window_x_,
					const double &start_mouse_window_y_,
					const GPlatesMaths::UnitVector3D &start_look_at_pos_on_globe_,
					const GPlatesMaths::UnitVector3D &start_view_direction_,
					const GPlatesMaths::UnitVector3D &start_up_direction_,
					const GPlatesMaths::Rotation &start_view_orientation_) :
				mode(mode_),
				start_mouse_pos_on_globe(start_mouse_pos_on_globe_),
				start_mouse_window_x(start_mouse_window_x_),
				start_mouse_window_y(start_mouse_window_y_),
				start_look_at_position(start_look_at_pos_on_globe_),
				start_view_direction(start_view_direction_),
				start_up_direction(start_up_direction_),
				start_view_orientation(start_view_orientation_),
				view_rotation_relative_to_start(GPlatesMaths::Rotation::create_identity_rotation()),
				tilt_method(TiltMethod::DONT_USE_CYLINDER_INTERSECTIONS/*arbitrary initialization value*/),
				start_intersects_globe_cylinder(false/*arbitrary initialization value*/)
			{  }

			MouseDragMode mode;

			GPlatesMaths::UnitVector3D start_mouse_pos_on_globe;
			double start_mouse_window_x;
			double start_mouse_window_y;

			GPlatesMaths::UnitVector3D start_look_at_position;
			GPlatesMaths::UnitVector3D start_view_direction;
			GPlatesMaths::UnitVector3D start_up_direction;
			GPlatesMaths::Rotation start_view_orientation;

			GPlatesMaths::Rotation view_rotation_relative_to_start;

			// For DRAG_ROTATE...
			GPlatesMaths::real_t start_rotation_angle;

			// For DRAG_TILT...
			TiltMethod tilt_method;
			bool start_intersects_globe_cylinder;
			GPlatesMaths::real_t tilt_cylinder_radius;
			GPlatesMaths::real_t start_tilt_angle;
			GPlatesMaths::real_t start_cylinder_intersect_angle_relative_to_view;
		};


		GPlatesGui::GlobeCamera &d_globe_camera;

		/**
		 * Info generated in @a start_drag and used subsequently in @a update_drag.
		 *
		 * Note that it can be none *during* a drag operation if the drag operation was
		 * disabled in 'start_drag()' for some reason.
		 */
		boost::optional<MouseDragInfo> d_mouse_drag_info;

		/**
		 * Is true if we're currently between the start of drag (@a start_drag) and
		 * end of drag ('end_of_drag' is true in call to @a update_drag).
		 *
		 * Note that this does not always coincide with @a d_mouse_drag_info which can
		 * be none if the drag operation was disabled in 'start_drag()' for some reason.
		 */
		bool d_in_drag_operation;

		/**
		 * Is true if we're currently in the last call to @a update_drag.
		 */
		bool d_in_last_update_drag;


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
		start_drag_tilt(
				int window_width,
				int window_height);

		void
		update_drag_tilt(
				double mouse_window_x,
				double mouse_window_y,
				int window_width,
				int window_height);

		void
		update_drag_tilt_without_cylinder_intersections(
				double mouse_window_x,
				double mouse_window_y,
				int window_width,
				int window_height);


		void
		start_drag_rotate_and_tilt(
				int window_width,
				int window_height);

		void
		update_drag_rotate_and_tilt(
				const GPlatesMaths::UnitVector3D &mouse_pos_on_globe,
				double mouse_window_x,
				double mouse_window_y,
				int window_width,
				int window_height);
	};
}

#endif // GPLATES_VIEW_OPERATIONS_GLOBEVIEWOPERATION_H
