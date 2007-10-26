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
	 */
	class CanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<CanvasTool>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<CanvasTool> non_null_ptr_type;

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
		 * Handle a left mouse-button click.
		 *
		 * This is a no-op implementation which may be overridden in a derived class.
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
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a left_release_after_drag instead.
		 *
		 * This is a no-op implementation which may be overridden in a derived class.
		 */
		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * This is a no-op implementation which may be overridden in a derived class.
		 */
		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe)
		{  }

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
		GPlatesQtWidgets::GlobeCanvas &
		globe_canvas() const
		{
			return *d_globe_canvas_ptr;
		}

		void
		reorient_globe_by_drag_update(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe);

		void
		reorient_globe_by_drag_release(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
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
