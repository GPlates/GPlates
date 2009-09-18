/* $Id$ */

/**
 * @file 
 * Contains definition of CanvasToolAdapterForGlobe
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H
#define GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H

#include <QString>
#include <boost/scoped_ptr.hpp>

#include "CanvasTool.h"
#include "gui/GlobeCanvasTool.h"

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
}

namespace GPlatesGui
{
	class Globe;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesCanvasTools
{

	/**
	 * Adapter class for CanvasTool -> GlobeCanvasTool
	 */
	class CanvasToolAdapterForGlobe:
			public GPlatesGui::GlobeCanvasTool
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<CanvasToolAdapterForGlobe,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<CanvasToolAdapterForGlobe,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * Create a CanvasToolAdapterForGlobe instance.
		 */
		static
		const non_null_ptr_type
		create(
				CanvasTool *canvas_tool_ptr,
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state);
		
		virtual
		void
		handle_activation();

		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

		virtual
		void
		handle_shift_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_shift_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_ctrl_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

		virtual
		void
		handle_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_ctrl_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_shift_ctrl_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

		virtual
		void
		handle_shift_ctrl_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_shift_ctrl_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		virtual
		void
		handle_move_without_drag(
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		CanvasToolAdapterForGlobe (
				CanvasTool *canvas_tool_ptr,
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_);
		
	private:

		//! A pointer to the CanvasTool instance that we wrap around
		boost::scoped_ptr<CanvasTool> d_canvas_tool_ptr;

		//! Listeners to status bar updates from the CanvasTool
		StatusBarListenerForViewport d_status_bar_listener;

	};
}

#endif  // GPLATES_CANVASTOOLS_CANVASTOOLADAPTERFORGLOBE_H
