/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_CANVASTOOL_H
#define GPLATES_GUI_CANVASTOOL_H

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


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
	 * The currently-activated CanvasTool is referenced by an instance of CanvasToolChoice.
	 */
	class CanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<CanvasTool,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<CanvasTool,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * Construct a CanvasTool instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		explicit
		CanvasTool(
				Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_):
			d_ref_count(0),
			d_globe_ptr(&globe_),
			d_globe_canvas_ptr(&globe_canvas_),
			d_is_in_reorientation_op(false)
		{  }

		virtual
		~CanvasTool() = 0;

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
				bool is_on_globe)
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
				bool is_on_globe)
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
				bool is_on_globe)
		{
			reorient_globe_by_drag_update(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe);
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
				bool is_on_globe)
		{
			reorient_globe_by_drag_release(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe);
		}

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

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
				bool is_on_globe);

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
				bool is_on_globe);

	private:
		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

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

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		CanvasTool(
				const CanvasTool &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		CanvasTool &
		operator=(
				const CanvasTool &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const CanvasTool *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const CanvasTool *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}
}

#endif  // GPLATES_GUI_CANVASTOOL_H
