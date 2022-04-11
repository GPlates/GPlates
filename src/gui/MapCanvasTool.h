/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C)  2008, 2009 Geological Survey of Norway
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

#ifndef GPLATES_GUI_MAPCANVASTOOL_H
#define GPLATES_GUI_MAPCANVASTOOL_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QPointF>

#include "maths/PointOnSphere.h"


namespace GPlatesQtWidgets
{
	class MapCanvas;
}

namespace GPlatesViewOperations
{
	class MapViewOperation;
}

namespace GPlatesGui
{
	class MapProjection;

	/**
	 * This class is the abstract base of all map canvas tools.
	 *
	 * This serves the role of the abstract State class in the State Pattern in Gamma et al.
	 */
	class MapCanvasTool :
			public boost::noncopyable
	{
	public:

		/**
		 * Construct a MapCanvasTool instance.
		 */
		explicit
		MapCanvasTool(
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesViewOperations::MapViewOperation &map_view_operation_);

		virtual
		~MapCanvasTool() = 0;

		/**
		 * Handle the activation (selection) of this tool.
		 */
		virtual
		void
		handle_activation()
		{  }

		/**
		 * Handle the deactivation of this tool (a different tool has been selected).
		 */
		virtual
		void
		handle_deactivation()
		{  }
		

		/**
		 * Handle a left mouse-button press.
		 *
		 * @a press_screen_position is the position of the press on the screen (viewport window).
		 * @a press_map_position is position of the press on the map plane (z=0), or none if not on plane.
		 * @a press_position_on_globe is the position of the press on the globe, or none if not on globe.
		 * Note: If @a press_position_on_globe is valid then @a press_map_position is also valid.
		 */
		virtual
		void
		handle_left_press(
				int screen_width,
				int screen_height,
				const QPointF &press_screen_position,
				const boost::optional<QPointF> &press_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &press_position_on_globe)
		{  }

		/**
		 * Handle a left mouse-button click.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_map_position is position of the click on the map plane (z=0), or none if not on plane.
		 * @a click_position_on_globe is the position of the click on the globe, or none if not on globe.
		 * Note: If @a click_position_on_globe is valid then @a click_map_position is also valid.
		 */
		virtual
		void
		handle_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 */
		virtual
		void
		handle_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 */
		virtual
		void
		handle_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{  }


		/**
		 * Handle a left mouse-button click while a Shift key is held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_map_position is position of the click on the map plane (z=0), or none if not on plane.
		 * @a click_position_on_globe is the position of the click on the globe, or none if not on globe.
		 * Note: If @a click_position_on_globe is valid then @a click_map_position is also valid.
		 */
		virtual
		void
		handle_shift_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key is held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 */
		virtual
		void
		handle_shift_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_shift_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 */
		virtual
		void
		handle_shift_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{  }


		/**
		 * Handle a left mouse-button click while a Alt key is held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_map_position is position of the click on the map plane (z=0), or none if not on plane.
		 * @a click_position_on_globe is the position of the click on the globe, or none if not on globe.
		 * Note: If @a click_position_on_globe is valid then @a click_map_position is also valid.
		 */
		virtual
		void
		handle_alt_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Alt key is held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 */
		virtual
		void
		handle_alt_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Alt key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_alt_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 */
		virtual
		void
		handle_alt_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{  }


		/**
		 * Handle a left mouse-button click while a Control key is held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_map_position is position of the click on the map plane (z=0), or none if not on plane.
		 * @a click_position_on_globe is the position of the click on the globe, or none if not on globe.
		 * Note: If @a click_position_on_globe is valid then @a click_map_position is also valid.
		 */
		virtual
		void
		handle_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Control key is held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 *
		 * The default implementation of this function pans the map.
		 */
		virtual
		void
		handle_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{
			pan_map_by_drag_update(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Control key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_ctrl_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 *
		 * The default implementation of this function pans the map.
		 */
		virtual
		void
		handle_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{
			pan_map_by_drag_release(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
		}


		/**
		 * Handle a left mouse-button click while a Shift key and a Control key are held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_map_position is position of the click on the map plane (z=0), or none if not on plane.
		 * @a click_position_on_globe is the position of the click on the globe, or none if not on globe.
		 * Note: If @a click_position_on_globe is valid then @a click_map_position is also valid.
		 */
		virtual
		void
		handle_shift_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key and a Control key are held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 *
		 * The default implementation of this function rotates the map.
		 */
		virtual
		void
		handle_shift_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{
			rotate_map_by_drag_update(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift key and a Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_shift_ctrl_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 *
		 * The default implementation of this function rotates the map.
		 */
		virtual
		void
		handle_shift_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{
			rotate_map_by_drag_release(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
		}


		/**
		 * Handle a left mouse-button click while a Alt key and a Control key are held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_map_position is position of the click on the map plane (z=0), or none if not on plane.
		 * @a click_position_on_globe is the position of the click on the globe, or none if not on globe.
		 * Note: If @a click_position_on_globe is valid then @a click_map_position is also valid.
		 */
		virtual
		void
		handle_alt_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Alt key and a Control key are held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 *
		 * The default implementation of this function rotates the map.
		 */
		virtual
		void
		handle_alt_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{
			tilt_map_by_drag_update(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Alt key and a Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_alt_ctrl_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_map_position is initial/current position on the map plane (z=0), or none if not on plane.
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe, or none if not on globe.
		 * Note: If @a (initial/current)_position_on_globe is valid then @a (initial/current)_map_position is also valid.
		 *
		 * The default implementation of this function rotates the map.
		 */
		virtual
		void
		handle_alt_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{
			tilt_map_by_drag_release(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
		}


		/**
		* Handle a mouse movement when left mouse-button is NOT down.
		*
		* This function should be invoked in response to intermediate updates of the
		* mouse-pointer position (as the mouse-pointer is moved about).
		 *
		 * @a screen_position is the position on the screen (viewport window).
		 * @a position_on_globe is the position on the globe, or none if not on globe.
		 * @a map_position is position on the map plane (z=0), or none if not on plane.
		 * Note: If @a position_on_globe is valid then @a map_position is also valid.
		*/
		virtual
		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				const QPointF &screen_position,
				const boost::optional<QPointF>&map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
		{  }

	protected:

		GPlatesQtWidgets::MapCanvas &
		map_canvas() const
		{
			return d_map_canvas;
		}

		/**
		 * Pan the map by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse button drag handler.
		 */
		void
		pan_map_by_drag_update(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe);

		/**
		 * Pan the map by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse button drag handler.
		 */
		void
		pan_map_by_drag_release(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe);

		/**
		 * Rotate the map around the centre of the viewport (in map 2D plane) by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift + left-mouse button drag handler.
		 */
		void
		rotate_map_by_drag_update(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe);

		/**
		 * Rotate the map around the centre of the viewport (in map 2D plane) by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift + left-mouse button drag handler.
		 */
		void
		rotate_map_by_drag_release(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe);

		/**
		 * Tilt the map around the centre of the viewport by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Alt + left-mouse button drag handler.
		 */
		void
		tilt_map_by_drag_update(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe);

		/**
		 * Tilt the map around the centre of the viewport by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Alt + left-mouse button drag handler.
		 */
		void
		tilt_map_by_drag_release(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe);

	private:

		//! The map canvas.
		GPlatesQtWidgets::MapCanvas &d_map_canvas;

		/**
		 * Used to orient/tilt the map view (converts mouse drags to map camera view changes).
		 *
		 * We reference the sole MapViewOperation shared by all map canvas tools for manipulating the view.
		 */
		GPlatesViewOperations::MapViewOperation &d_map_view_operation;
	};
}

#endif  // GPLATES_GUI_MAPCANVASTOOL_H
