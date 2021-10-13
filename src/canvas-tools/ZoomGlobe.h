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


namespace GPlatesQtWidgets
{
	class GlobeCanvas;
	class ViewportWindow;
}

namespace GPlatesPresentation
{
	class ViewState;
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
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesPresentation::ViewState &view_state_):
			GlobeCanvasTool(globe_, globe_canvas_),
			d_viewport_window(&viewport_window_),
			d_view_state(view_state_)
		{  }

		virtual
		void
		handle_activation();

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe);

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
