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

#include "view-operations/GlobeViewOperation.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
}

namespace GPlatesGui
{
	class Globe;

	/**
	 * This class is the abstract base of all canvas tools.
	 *
	 * This serves the role of the abstract State class in the State Pattern in Gamma et al.
	 *
	 * The currently-activated GlobeCanvasTool is referenced by an instance of GlobeCanvasToolChoice.
	 */
	class GlobeCanvasTool :
			public boost::noncopyable
	{
	public:

		/**
		 * Construct a GlobeCanvasTool instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		explicit
		GlobeCanvasTool(
				Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_);

		virtual
		~GlobeCanvasTool() = 0;

		/**
		 * Handle the activation (selection) of this tool.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_activation()
		{  }

		/**
		 * Handle the deactivation of this tool (a different tool has been selected).
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_shift_left_drag instead.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_alt_left_drag instead.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 * The default implementation of this function re-orients the globe.  This
		 * implementation may be overridden in a derived class.
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
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{
			reorient_globe_by_drag_update(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Control
		 * key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_ctrl_left_drag instead.
		 *
		 * The default implementation of this function re-orients the globe.  This
		 * implementation may be overridden in a derived class.
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
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{
			reorient_globe_by_drag_release(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}

		/**
		 * Handle a left mouse-button click while a Shift key and a Control key are held.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 * The default implementation of this function rotates the globe.  This
		 * implementation may be overridden in a derived class.
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
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{
			rotate_globe_by_drag_update(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift key
		 * and Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_shift_ctrl_left_drag instead.
		 *
		 * The default implementation of this function rotates the globe.  This
		 * implementation may be overridden in a derived class.
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
				const GPlatesMaths::PointOnSphere &centre_of_viewport)
		{
			rotate_globe_by_drag_release(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}

		/**
		 * Handle a left mouse-button click while a Alt key and a Control key are held.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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
		 *
		 * The default implementation of this function tilts the globe.  This
		 * implementation may be overridden in a derived class.
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
		{
			tilt_globe_by_drag_update(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Alt key
		 * and Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_alt_ctrl_left_drag instead.
		 *
		 * The default implementation of this function tilts the globe.  This
		 * implementation may be overridden in a derived class.
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
		{
			tilt_globe_by_drag_release(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
		}

		/**
		 * Handle a mouse movement when left mouse-button is NOT down.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about).
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
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

		Globe &
		globe() const
		{
			return *d_globe_ptr;
		}

		GPlatesQtWidgets::GlobeCanvas &
		globe_canvas() const
		{
			return *d_globe_canvas_ptr;
		}

		/**
		 * Re-orient the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		reorient_globe_by_drag_update(
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
		 * Re-orient the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		reorient_globe_by_drag_release(
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
		 * Rotate the globe around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_globe_by_drag_update(
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
		 * Rotate the globe around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_globe_by_drag_release(
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
		 * Tilt the globe around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Alt +
		 * left-mouse button drag handler.
		 */
		void
		tilt_globe_by_drag_update(
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
		 * Tilt the globe around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Alt +
		 * left-mouse button drag handler.
		 */
		void
		tilt_globe_by_drag_release(
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

		/**
		 * The globe which will be re-oriented by globe re-orientation operations.
		 */
		Globe *d_globe_ptr;

		/**
		 * The globe canvas which will need to be updated after globe re-orientation.
		 */
		GPlatesQtWidgets::GlobeCanvas *d_globe_canvas_ptr;

		/**
		 * Used to orient/tilt the globe view.
		 */
		GPlatesViewOperations::GlobeViewOperation d_globe_view_operation;

		/**
		 * Whether or not this canvas tool is currently in the midst of a globe
		 * re-orientation operation.
		 */
		bool d_is_in_reorientation_op;
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOL_H
