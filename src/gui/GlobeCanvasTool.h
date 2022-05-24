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


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
}

namespace GPlatesViewOperations
{
	class GlobeViewOperation;
}

namespace GPlatesGui
{
	/**
	 * This class is the abstract base of all canvas tools.
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
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
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
		 * @a press_pos_on_globe is the position of the click on the globe.
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
				double press_screen_x,
				double press_screen_y,
				const GPlatesMaths::PointOnSphere &press_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a left mouse-button click.
		 *
		 * @a click_pos_on_globe is the position of the click on the globe.
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
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed.
		 *
		 * @a initial_pos_on_globe is the position on the globe at which the mouse pointer
		 * was located when the mouse button was pressed and held.
		 *
		 * Note that the mouse pointer may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a was_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * @a current_pos_on_globe is the position on the globe at which the mouse pointer
		 * is currently located.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a left_release_after_drag instead.
		 */
		virtual
		void
		handle_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag.
		 *
		 * @a initial_pos_on_globe is the position on the globe at which the mouse pointer
		 * was located when the mouse button was pressed and held.
		 *
		 * Note that the mouse pointer may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a was_on_globe will be false, and
		 * the position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * @a current_pos_on_globe is the position on the globe at which the mouse pointer
		 * is currently located.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported will be the closest position on the globe to the actual
		 * mouse pointer position on-screen.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_left_drag instead.
		 */
		virtual
		void
		handle_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }


		/**
		 * Handle a left mouse-button click while a Shift key is held.
		 */
		virtual
		void
		handle_shift_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key is
		 * held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a handle_shift_left_release_after_drag instead.
		 */
		virtual
		void
		handle_shift_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift
		 * key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_shift_left_drag instead.
		 */
		virtual
		void
		handle_shift_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }


		/**
		 * Handle a left mouse-button click while a Alt key is held.
		 */
		virtual
		void
		handle_alt_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Alt key is
		 * held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a handle_alt_left_release_after_drag instead.
		 */
		virtual
		void
		handle_alt_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Alt
		 * key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_alt_left_drag instead.
		 */
		virtual
		void
		handle_alt_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }


		/**
		 * Handle a left mouse-button click while a Control key is held.
		 */
		virtual
		void
		handle_ctrl_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Control key is
		 * held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a handle_ctrl_left_release_after_drag instead.
		 *
		 * The default implementation of this function pans (re-orients) the globe.
		 */
		virtual
		void
		handle_ctrl_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Control
		 * key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_ctrl_left_drag instead.
		 *
		 * The default implementation of this function pans (re-orients) the globe.
		 */
		virtual
		void
		handle_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);


		/**
		 * Handle a left mouse-button click while a Shift key and a Control key are held.
		 */
		virtual
		void
		handle_shift_ctrl_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key and a
		 * Control key are held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a handle_shift_ctrl_left_release_after_drag instead.
		 *
		 * The default implementation of this function rotates and tilts the globe.
		 */
		virtual
		void
		handle_shift_ctrl_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift key
		 * and Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_shift_ctrl_left_drag instead.
		 *
		 * The default implementation of this function rotates and tilts the globe.
		 */
		virtual
		void
		handle_shift_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);


		/**
		 * Handle a left mouse-button click while a Alt key and a Control key are held.
		 */
		virtual
		void
		handle_alt_ctrl_left_click(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Alt key and a
		 * Control key are held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a handle_alt_ctrl_left_release_after_drag instead.
		 */
		virtual
		void
		handle_alt_ctrl_left_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Alt key
		 * and Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a handle_alt_ctrl_left_drag instead.
		 */
		virtual
		void
		handle_alt_ctrl_left_release_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }


		/**
		 * Handle a mouse movement when left mouse-button is NOT down.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about).
		 */
		virtual
		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{  }

	protected:

		GPlatesQtWidgets::GlobeCanvas &
		globe_canvas() const
		{
			return d_globe_canvas;
		}

		/**
		 * Pan (re-orient) the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		pan_globe_by_drag_update(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);

		/**
		 * Pan (re-orient) the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		pan_globe_by_drag_release(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);

		/**
		 * Rotate and tilt the globe around the centre of the viewport by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_and_tilt_globe_by_drag_update(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);

		/**
		 * Rotate and tilt the globe around the centre of the viewport by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_and_tilt_globe_by_drag_release(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);

	private:

		//! The globe canvas.
		GPlatesQtWidgets::GlobeCanvas &d_globe_canvas;

		/**
		 * Used to pan/rotate/tilt the globe view (converts mouse drags to globe camera view changes).
		 *
		 * We reference the sole GlobeViewOperation shared by all globe canvas tools for manipulating the view.
		 */
		GPlatesViewOperations::GlobeViewOperation &d_globe_view_operation;
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOL_H
