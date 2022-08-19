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
#include <QPointF>

#include "RenderedGeometryCollection.h"

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
	 * Enables users to pan/rotate/tilt the globe by dragging it.
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
			// Pan (rotate along great circle arcs) as mouse is dragged across the globe...
			DRAG_PAN,
			// Rotate and tilt using same mouse drag motion...
			// Using horizontal drag, rotate about the axis (from globe centre) through the
			// look-at position on globe (centre of viewport).
			// Using vertical drag, tilt around the axis (perpendicular to view and up directions)
			// passing tangentially through the look-at position on globe.
			DRAG_ROTATE_AND_TILT
		};


		GlobeViewOperation(
				GPlatesGui::GlobeCamera &globe_camera,
				RenderedGeometryCollection &rendered_geometry_collection);

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
				const QPointF &initial_mouse_screen_position,
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
				const QPointF &mouse_screen_position,
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
		 * Information generated in @a start_drag and used in subsequent calls to @a update_drag.
		 */
		struct PanDragInfo
		{
			PanDragInfo(
					const GPlatesMaths::PointOnSphere &start_mouse_pos_on_globe_,
					const GPlatesMaths::Rotation &start_view_orientation_) :
				start_mouse_pos_on_globe(start_mouse_pos_on_globe_.position_vector()),
				start_view_orientation(start_view_orientation_),
				view_rotation_relative_to_start(GPlatesMaths::Rotation::create_identity_rotation())
			{  }

			GPlatesMaths::UnitVector3D start_mouse_pos_on_globe;
			GPlatesMaths::Rotation start_view_orientation;

			GPlatesMaths::Rotation view_rotation_relative_to_start;
		};

		/**
		 * Information generated in @a start_drag_rotate_and_tilt and used in subsequent calls to @a update_drag_rotate_and_tilt.
		 */
		struct RotateAndTiltDragInfo
		{
			RotateAndTiltDragInfo(
					// Start mouse window coordinates...
					const double &start_mouse_window_x,
					const double &start_mouse_window_y) :
				rotate_from_mouse_window_coord(start_mouse_window_x),
				tilt_from_mouse_window_coord(start_mouse_window_y)
			{  }

			//
			// The mouse window coordinates to rotate/tilt *from* in the next rotate/tilt update.
			//
			// For the first rotate/tilt update this will be from the start of the drag, and
			// for subsequent rotate/tilt updates this will be the *to* (or destination) mouse coordinates
			// of the previous rotate/tilt update.
			//
			double rotate_from_mouse_window_coord;
			double tilt_from_mouse_window_coord;
		};


		GPlatesGui::GlobeCamera &d_globe_camera;

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
				const GPlatesMaths::PointOnSphere &start_mouse_pos_on_globe);

		void
		update_drag_pan(
				const GPlatesMaths::PointOnSphere &mouse_pos_on_globe);


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

#endif // GPLATES_VIEW_OPERATIONS_GLOBEVIEWOPERATION_H
