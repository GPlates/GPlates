/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


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
			public GPlatesUtils::ReferenceCount<GlobeCanvasTool>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GlobeCanvasTool,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GlobeCanvasTool,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

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
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_):
			d_globe_ptr(&globe_),
			d_globe_canvas_ptr(&globe_canvas_),
			d_is_in_reorientation_op(false)
		{  }

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
		 * Handle a left mouse-button click.
		 *
		 * @a click_pos_on_globe is the position of the click on the globe, without taking
		 * the globe-orientation into account:  (0, 0) is always in the centre of the
		 * canvas; (0, -90) is always on the left-most point of the globe in the canvas;
		 * (0, 90) is always on the right-most point of the globe in the canvas; etc.  This
		 * position is used to determine the proximity inclusion threshold of clicks.
		 *
		 * @a oriented_click_pos_on_globe is the position of the click on the oriented
		 * globe.  This is the position which should be compared to geometries on the globe
		 * when testing for hits.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * positions reported in the previous two parameters will be the closest positions
		 * on the globe to the actual mouse pointer position on-screen.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed.
		 *
		 * @a initial_pos_on_globe is the position on the globe (without taking
		 * globe-orientation into account) at which the mouse pointer was located when the
		 * mouse button was pressed and held.
		 *
		 * @a oriented_initial_pos_on_globe is the position on the oriented globe.  This is
		 * the position which should be compared to geometries on the globe when testing
		 * for hits.
		 *
		 * Note that the mouse pointer may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a was_on_globe will be false, and
		 * the positions reported in the previous two parameters will be the closest
		 * positions on the globe to the actual mouse pointer position on-screen.
		 *
		 * @a current_pos_on_globe is the position on the globe (without taking
		 * globe-orientation into account) at which the mouse pointer is currently located.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported in the previous parameter will be the closest position on the
		 * globe to the actual mouse pointer position on-screen.
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
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag.
		 *
		 * @a initial_pos_on_globe is the position on the globe (without taking
		 * globe-orientation into account) at which the mouse pointer was located when the
		 * mouse button was pressed and held.
		 *
		 * @a oriented_initial_pos_on_globe is the position on the oriented globe.  This is
		 * the position which should be compared to geometries on the globe when testing
		 * for hits.
		 *
		 * Note that the mouse pointer may not actually have been on the globe:  If the
		 * mouse pointer was not actually on the globe, @a was_on_globe will be false, and
		 * the positions reported in the previous two parameters will be the closest
		 * positions on the globe to the actual mouse pointer position on-screen.
		 *
		 * @a current_pos_on_globe is the position on the globe (without taking
		 * globe-orientation into account) at which the mouse pointer is currently located.
		 *
		 * Note that the mouse pointer may not actually be on the globe:  If the mouse
		 * pointer is not actually on the globe, @a is_on_globe will be false, and the
		 * position reported in the previous parameter will be the closest position on the
		 * globe to the actual mouse pointer position on-screen.
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
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
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
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key is
		 * held.
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
		handle_shift_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift
		 * key is held.
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
		handle_shift_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
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
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Control key is
		 * held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a left_release_after_drag instead.
		 *
		 * The default implementation of this function re-orients the globe.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
		{
			reorient_globe_by_drag_update(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Control
		 * key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * The default implementation of this function re-orients the globe.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_ctrl_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
		{
			reorient_globe_by_drag_release(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
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
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key and a
		 * Control key are held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a left_release_after_drag instead.
		 *
		 * The default implementation of this function rotates the globe.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_shift_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
		{
			rotate_globe_by_drag_update(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift key
		 * and Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * The default implementation of this function rotates the globe.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_shift_ctrl_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
		{
			rotate_globe_by_drag_release(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
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
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
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
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		/**
		 * Re-orient the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		reorient_globe_by_drag_release(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		/**
		 * Rotate the globe around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_globe_by_drag_update(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		/**
		 * Rotate the globe around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_globe_by_drag_release(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

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
		 * Whether or not this canvas tool is currently in the midst of a globe
		 * re-orientation operation.
		 */
		bool d_is_in_reorientation_op;
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOL_H
