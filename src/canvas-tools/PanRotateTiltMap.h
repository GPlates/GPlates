/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_PANMAP_H
#define GPLATES_CANVASTOOLS_PANMAP_H

#include "gui/MapCanvasTool.h"


namespace GPlatesQtWidgets
{
	class MapCanvas;
	class ViewportWindow;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to pan, rotate and tilt the map by dragging.
	 */
	class PanRotateTiltMap:
			public GPlatesGui::MapCanvasTool
	{
	public:

		/**
		 * Create a PanRotateTiltMap instance.
		 */
		explicit
		PanRotateTiltMap(
				GPlatesQtWidgets::MapCanvas &map_canvas_,
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
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;


		void
		handle_shift_left_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

		void
		handle_shift_left_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const GPlatesMaths::PointOnSphere &initial_position_on_globe,
				bool was_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const GPlatesMaths::PointOnSphere &current_position_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe) override;

	private:

		/**
		 * This is the View State used to pass messages to the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;
	};
}

#endif  // GPLATES_CANVASTOOLS_PANMAP_H
