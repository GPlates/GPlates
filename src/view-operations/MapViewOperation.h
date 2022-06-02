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
		 * Information generated in @a start_drag and used in subsequent calls to @a update_drag.
		 */
		struct MouseDragInfo
		{
			MouseDragInfo(
					MouseDragMode mode_,
					const double &start_mouse_window_x_,
					const double &start_mouse_window_y_,
					const boost::optional<QPointF> &start_map_position_,
					const QPointF &start_look_at_position_,
					const GPlatesMaths::UnitVector3D &start_view_direction_,
					const GPlatesMaths::UnitVector3D &start_up_direction_,
					const GPlatesMaths::real_t &start_rotation_angle_,
					const GPlatesMaths::real_t &start_tilt_angle_) :
				mode(mode_),
				start_mouse_window_x(start_mouse_window_x_),
				start_mouse_window_y(start_mouse_window_y_),
				start_map_position(start_map_position_),
				start_look_at_position(start_look_at_position_),
				start_view_direction(start_view_direction_),
				start_up_direction(start_up_direction_),
				pan_relative_to_start_in_view_frame(0, 0),
				start_rotation_angle(start_rotation_angle_),
				start_tilt_angle(start_tilt_angle_)
			{  }

			MouseDragMode mode;

			double start_mouse_window_x;
			double start_mouse_window_y;
			boost::optional<QPointF> start_map_position;

			QPointF start_look_at_position;
			GPlatesMaths::UnitVector3D start_view_direction;
			GPlatesMaths::UnitVector3D start_up_direction;

			// For DRAG_PAN...
			QPointF pan_relative_to_start_in_view_frame;

			// For DRAG_ROTATE_AND_TILT...
			GPlatesMaths::real_t start_rotation_angle;
			GPlatesMaths::real_t start_tilt_angle;
		};


		GPlatesGui::MapCamera &d_map_camera;

		/**
		 * Info generated in @a start_drag and used subsequently in @a update_drag.
		 */
		boost::optional<MouseDragInfo> d_mouse_drag_info;

		/**
		 * Is true if we're currently between the start of drag (@a start_drag) and
		 * end of drag ('end_of_drag' is true in call to @a update_drag).
		 */
		bool d_in_drag_operation;

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
		start_drag_pan();

		void
		update_drag_pan(
				const boost::optional<QPointF> &map_position);

		void
		start_drag_rotate_and_tilt();

		void
		update_drag_rotate_and_tilt(
				double mouse_window_x,
				double mouse_window_y,
				int window_width,
				int window_height);
	};
}

#endif // GPLATES_VIEW_OPERATIONS_MAPVIEWOPERATION_H
