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

#ifndef GPLATES_CANVASTOOLS_ZOOMMAP_H
#define GPLATES_CANVASTOOLS_ZOOMMAP_H

#include "gui/MapCanvasTool.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class MapCanvas;
	class ViewportWindow;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to re-orient the globe by dragging.
	 */
	class ZoomMap :
			public GPlatesGui::MapCanvasTool
	{
	public:

		/**
		 * Create a PanMap instance.
		 */
		explicit
		ZoomMap(
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_);


		void
		handle_activation() override;

		void
		handle_deactivation() override;

		
		void
		handle_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;

		void
		handle_shift_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const GPlatesMaths::PointOnSphere &click_position_on_globe,
				bool is_on_globe) override;


	private:

		void
		recentre_map(
				const GPlatesMaths::PointOnSphere &position_on_globe);

		/**
		 * This is the window that has the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/**
		 * This is the view state (in the presentation tier).
		 */
		GPlatesPresentation::ViewState &d_view_state;
	};
}

#endif  // GPLATES_CANVASTOOLS_ZOOMMAP_H
