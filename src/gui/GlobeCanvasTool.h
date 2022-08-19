/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBECANVASTOOL_H
#define GPLATES_GUI_GLOBECANVASTOOL_H

#include <boost/noncopyable.hpp>
#include <QPointF>

#include "maths/PointOnSphere.h"


namespace GPlatesQtWidgets
{
	class GlobeAndMapCanvas;
}

namespace GPlatesViewOperations
{
	class GlobeViewOperation;
}

namespace GPlatesGui
{
	/**
	 * This class is the abstract base of all globe canvas tools.
	 *
	 * This serves the role of the abstract State class in the State Pattern in Gamma et al.
	 */
	class GlobeCanvasTool :
			public boost::noncopyable
	{
	public:

		/**
		 * Construct a GlobeCanvasTool instance.
		 */
		explicit
		GlobeCanvasTool(
				GPlatesQtWidgets::GlobeAndMapCanvas &globe_canvas_,
				GPlatesViewOperations::GlobeViewOperation &globe_view_operation_);

		virtual
		~GlobeCanvasTool() = 0;

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
		 * @a press_position_on_globe is the position of the press on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_left_press(
				int screen_width,
				int screen_height,
				const QPointF &press_screen_position,
				const GPlatesMaths::PointOnSphere &press_position_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a left mouse-button click.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_position_on_globe is the position of the click on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
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
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
		{  }


		/**
		 * Handle a left mouse-button click while a Shift key is held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_position_on_globe is the position of the click on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_shift_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key is held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_shift_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
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
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_shift_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
		{  }


		/**
		 * Handle a left mouse-button click while a Alt key is held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_position_on_globe is the position of the click on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_alt_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Alt key is held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_alt_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
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
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_alt_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
		{  }


		/**
		 * Handle a left mouse-button click while a Control key is held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_position_on_globe is the position of the click on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Control key is held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * The default implementation of this function pans the globe.
		 */
		virtual
		void
		handle_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Control key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_ctrl_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * The default implementation of this function pans the globe.
		 */
		virtual
		void
		handle_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);


		/**
		 * Handle a left mouse-button click while a Shift key and a Control key are held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_position_on_globe is the position of the click on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_shift_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key and a Control key are held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * The default implementation of this function rotates and tilts the globe.
		 */
		virtual
		void
		handle_shift_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift key and a Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_shift_ctrl_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * The default implementation of this function rotates and tilts the globe.
		 */
		virtual
		void
		handle_shift_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);


		/**
		 * Handle a left mouse-button click while a Alt key and a Control key are held.
		 *
		 * @a click_screen_position is the position of the click on the screen (viewport window).
		 * @a click_position_on_globe is the position of the click on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_alt_ctrl_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Alt key and a Control key are held.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_alt_ctrl_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Alt key and a Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_alt_ctrl_left_drag instead.
		 *
		 * @a (initial/current)_screen_position is the initial/current position on the screen (viewport window).
		 * @a (initial/current)_position_on_globe is the initial/current position on the globe.
		 *
		 * Note that the mouse pointers may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a (was/is)_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_alt_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
		{  }


		/**
		 * Handle a mouse movement when left mouse-button is NOT down.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about).
		 *
		 * @a screen_position is the position on the screen (viewport window).
		 * @a position_on_globe is the position on the globe.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 */
		virtual
		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				const QPointF &screen_position,
				const GPlatesMaths::PointOnSphere &position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe)
		{  }

	protected:

		GPlatesQtWidgets::GlobeAndMapCanvas &
		globe_canvas() const
		{
			return d_globe_canvas;
		}

		/**
		 * Pan (re-orient) the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse button drag handler.
		 */
		void
		pan_globe_by_drag_update(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);

		/**
		 * Pan (re-orient) the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse button drag handler.
		 */
		void
		pan_globe_by_drag_release(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);

		/**
		 * Rotate and tilt the globe around the centre of the viewport by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift + left-mouse button drag handler.
		 */
		void
		rotate_and_tilt_globe_by_drag_update(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);

		/**
		 * Rotate and tilt the globe around the centre of the viewport by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift + left-mouse button drag handler.
		 */
		void
		rotate_and_tilt_globe_by_drag_release(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);

	private:

		//! The globe canvas.
		GPlatesQtWidgets::GlobeAndMapCanvas &d_globe_canvas;

		/**
		 * Used to pan/rotate/tilt the globe view (converts mouse drags to globe camera view changes).
		 *
		 * We reference the sole GlobeViewOperation shared by all globe canvas tools for manipulating the view.
		 */
		GPlatesViewOperations::GlobeViewOperation &d_globe_view_operation;
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOL_H
