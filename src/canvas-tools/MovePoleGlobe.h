/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_CANVAS_TOOLS_MOVEPOLEGLOBE_H
#define GPLATES_CANVAS_TOOLS_MOVEPOLEGLOBE_H

#include "gui/GlobeCanvasTool.h"

#include "view-operations/MovePoleOperation.h"


namespace GPlatesQtWidgets
{
	class GlobeAndMapCanvas;
	class ViewportWindow;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the globe canvas tool used to move the pole location used by ManipulatePole tool for adjusting rotations.
	 */
	class MovePoleGlobe:
			public GPlatesGui::GlobeCanvasTool
	{
	public:

		/**
		 * Create a MovePoleGlobe instance.
		 */
		explicit
		MovePoleGlobe(
				const GPlatesViewOperations::MovePoleOperation::non_null_ptr_type &move_pole_operation,
				GPlatesQtWidgets::GlobeAndMapCanvas &globe_canvas_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_);

		void
		handle_activation() override;

		void
		handle_deactivation() override;


		void
		handle_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				const QPointF &screen_position,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

	private:

		/**
		 * This is the View State used to pass messages to the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window;

		/**
		 * Handles changes to the pole location for us.
		 */
		GPlatesViewOperations::MovePoleOperation::non_null_ptr_type d_move_pole_operation;

		/**
		 * Whether or not this tool is currently in the midst of a drag.
		 */
		bool d_is_in_drag;

	};
}

#endif // GPLATES_CANVAS_TOOLS_MOVEPOLEGLOBE_H
