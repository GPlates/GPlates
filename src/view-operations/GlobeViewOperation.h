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
				int screen_height);

		void
		end_drag();

	private:

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
				in_upper_viewport(false/*arbitrary initialization value*/)
			{  }

			MouseDragMode mode;

			GPlatesMaths::UnitVector3D start_mouse_pos_on_globe;
			double start_mouse_window_x;
			double start_mouse_window_y;

			GPlatesMaths::UnitVector3D start_look_at_position;
			GPlatesMaths::UnitVector3D start_view_direction;
			GPlatesMaths::UnitVector3D start_up_direction;
			GPlatesMaths::Rotation start_view_orientation;

			// For DRAG_ROTATE...
			GPlatesMaths::real_t start_rotation_angle;

			// For DRAG_TILT...
			GPlatesMaths::real_t tilt_cylinder_radius;
			bool in_upper_viewport;
			GPlatesMaths::real_t start_cyl_intersect_relative_to_view_tilt_angle;
			GPlatesMaths::real_t start_view_relative_to_globe_normal_tilt_angle;

			GPlatesMaths::Rotation view_rotation_relative_to_start;
		};


		GPlatesGui::GlobeCamera &d_globe_camera;

		/**
		 * Info generated in @a start_drag and used subsequently in @a update_drag.
		 */
		boost::optional<MouseDragInfo> d_mouse_drag_info;


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
		start_drag_rotate_and_tilt();

		void
		update_drag_rotate_and_tilt(
				const GPlatesMaths::UnitVector3D &mouse_pos_on_globe);
	};
}

#endif // GPLATES_VIEW_OPERATIONS_GLOBEVIEWOPERATION_H
