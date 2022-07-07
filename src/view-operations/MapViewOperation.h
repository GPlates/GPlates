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

#ifndef GPLATES_VIEW_OPERATIONS_MAPVIEWOPERATION_H
#define GPLATES_VIEW_OPERATIONS_MAPVIEWOPERATION_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QPointF>

#include "RenderedGeometryCollection.h"

#include "maths/PointOnSphere.h"
#include "maths/types.h"


namespace GPlatesGui
{
	class MapCamera;
}

namespace GPlatesViewOperations
{
	/**
	 * Enables users to pan/rotate/tilt the map by dragging it.
	 */
	class MapViewOperation :
			private boost::noncopyable
	{
	public:

		/**
		 * Mouse drag modes.
		 */
		enum MouseDragMode
		{
			// Pan along the 2D map plane as mouse is dragged across the map...
			DRAG_PAN,
			// Rotate and tilt using same mouse drag motion...
			// Using horizontal drag, rotate about the 2D map plane normal at the look-at position (centre of viewport).
			// Using vertical drag, tilt around the axis (perpendicular to view and up directions) passing tangentially
			// through the look-at position on map.
			DRAG_ROTATE_AND_TILT
		};


		MapViewOperation(
				GPlatesGui::MapCamera &map_camera,
				RenderedGeometryCollection &rendered_geometry_collection);

		/**
		 * Start a mouse drag, using the specified mode, at the specified initial position.
		 *
		 * Subsequent calls to @a update_drag will use the specified drag mode.
		 */
		void
		start_drag(
				MouseDragMode mouse_drag_mode,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				int screen_width,
				int screen_height);

		/**
		 * Update the camera view using the specified mouse drag position.
		 *
		 * This uses the drag mode specified in the last call to @a start_drag.
		 *
		 * If @a end_of_drag is true then this is the last update of the drag, and hence @a start_drag
		 * must be called before the next @a update_drag.
		 */
		void
		update_drag(
				const QPointF &screen_position,
				const boost::optional<QPointF> &map_position,
				int screen_width,
				int screen_height,
				bool end_of_drag);

		/**
		 * Returns true if currently in a drag.
		 *
		 * A drag is where @a start_drag has been called but the last
		 * @a update_drag ('end_of_drag' is true) has not yet been called.
		 *
		 * Note that this means false is returned *during* the last update
		 * (ie, during the call to @a update_drag where 'end_of_drag' is true).
		 */
		bool
		in_drag() const
		{
			return static_cast<bool>(d_mouse_drag_mode);
		}

		/**
		 * Returns the drag mode if currently in a drag (otherwise returns none).
		 *
		 * See @a in_drag.
		 */
		boost::optional<MouseDragMode>
		drag_mode() const
		{
			return d_mouse_drag_mode;
		}

	private:

		/**
		 * Information generated in @a start_drag_pan and used in subsequent calls to @a update_drag_pan.
		 */
		struct PanDragInfo
		{
			explicit
			PanDragInfo(
					// Start mouse window coordinates, but only if mouse is *on* the map plane...
					const boost::optional<QPointF> &start_mouse_window_coords_) :
				drag_from_mouse_window_coords(start_mouse_window_coords_)
			{  }

			/**
			 * The mouse window coordinates to drag *from* in the next drag update, but only if mouse was *on* the map plane.
			 *
			 * For the first drag update this will be from the start of the drag, and for subsequent drag updates
			 * this will be the *to* (or destination) mouse coordinates of the previous drag update.
			 *
			 * Note: A value of none indicates that the mouse was NOT on the map plane.
			 */
			boost::optional<QPointF> drag_from_mouse_window_coords;
		};

		/**
		 * Information generated in @a start_drag_rotate_and_tilt and used in subsequent calls to @a update_drag_rotate_and_tilt.
		 */
		struct RotateAndTiltDragInfo
		{
			RotateAndTiltDragInfo(
					const double &start_mouse_window_x_,
					const double &start_mouse_window_y_,
					const GPlatesMaths::real_t &start_rotation_angle_,
					const GPlatesMaths::real_t &start_tilt_angle_) :
				start_mouse_window_x(start_mouse_window_x_),
				start_mouse_window_y(start_mouse_window_y_),
				start_rotation_angle(start_rotation_angle_),
				start_tilt_angle(start_tilt_angle_)
			{  }

			double start_mouse_window_x;
			double start_mouse_window_y;

			GPlatesMaths::real_t start_rotation_angle;
			GPlatesMaths::real_t start_tilt_angle;
		};


		GPlatesGui::MapCamera &d_map_camera;

		/**
		 * Is true if we're currently between the start of drag (@a start_drag) and
		 * end of drag ('end_of_drag' is true in call to @a update_drag).
		 */
		boost::optional<MouseDragMode> d_mouse_drag_mode;

		/**
		 * Info used when panning the view.
		 */
		boost::optional<PanDragInfo> d_pan_drag_info;

		/**
		 * Info used when rotating and tilting the view.
		 */
		boost::optional<RotateAndTiltDragInfo> d_rotate_and_tilt_drag_info;

		/**
		 * Is true if we're currently in the last call to @a update_drag.
		 */
		bool d_in_last_update_drag;

		/**
		 * Used to render the centre of viewport when panning/rotating/tilting.
		 */
		RenderedGeometryCollection &d_rendered_geometry_collection;
		RenderedGeometryCollection::child_layer_owner_ptr_type d_rendered_layer_ptr;


		void
		start_drag_pan(
				const boost::optional<QPointF> &start_map_position,
				const double &start_mouse_window_x,
				const double &start_mouse_window_y);

		void
		update_drag_pan(
				const boost::optional<QPointF> &map_position,
				double mouse_window_x,
				double mouse_window_y,
				int window_width,
				int window_height);

		void
		start_drag_rotate_and_tilt(
				const double &start_mouse_window_x,
				const double &start_mouse_window_y);

		void
		update_drag_rotate_and_tilt(
				double mouse_window_x,
				double mouse_window_y,
				int window_width,
				int window_height);
	};
}

#endif // GPLATES_VIEW_OPERATIONS_MAPVIEWOPERATION_H
