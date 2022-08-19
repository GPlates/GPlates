/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_ZOOMGLOBE_H
#define GPLATES_CANVASTOOLS_ZOOMGLOBE_H

#include "gui/GlobeCanvasTool.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapCanvas;
	class ViewportWindow;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to zoom into a point on the globe by clicking.
	 */
	class ZoomGlobe:
			public GPlatesGui::GlobeCanvasTool
	{
	public:

		/**
		 * Create a ZoomGlobe instance.
		 */
		explicit
		ZoomGlobe(
				GPlatesQtWidgets::GlobeAndMapCanvas &globe_canvas_,
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
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

		void
		handle_shift_left_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe) override;

	private:

		void
		recentre_globe(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe);

		/**
		 * This is the View State used to pass messages to the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window;

		/**
		 * This is the view state (in the presentation tier).
		 */
		GPlatesPresentation::ViewState &d_view_state;
	};
}

#endif  // GPLATES_CANVASTOOLS_ZOOMGLOBE_H
