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

#ifndef GPLATES_CANVAS_TOOLS_MOVEPOLEMAP_H
#define GPLATES_CANVAS_TOOLS_MOVEPOLEMAP_H

#include "gui/MapCanvasTool.h"

#include "view-operations/MovePoleOperation.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class MovePoleWidget;
	class ViewportWindow;
}


namespace GPlatesCanvasTools
{
	/**
	 * This is the map canvas tool used to move the pole location used by ManipulatePole tool for adjusting rotations.
	 */
	class MovePoleMap :
			public GPlatesGui::MapCanvasTool
	{
	public:

		/**
		 * Create a MovePoleMap instance.
		 */
		MovePoleMap(
				const GPlatesViewOperations::MovePoleOperation::non_null_ptr_type &move_pole_operation,
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesPresentation::ViewState &view_state_);

		virtual
		void
		handle_activation();

		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_move_without_drag(
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

	private:

		/**
		 * This is the window that has the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

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

#endif // GPLATES_CANVAS_TOOLS_MOVEPOLEMAP_H
