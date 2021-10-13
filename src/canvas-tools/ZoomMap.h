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


namespace GPlatesGui
{
	class MapTransform;
	class ViewportZoom;
}

namespace GPlatesQtWidgets
{
	class MapCanvas;
	class MapView;
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
				GPlatesQtWidgets::MapView &map_view_,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesGui::MapTransform &map_transform_,
				GPlatesGui::ViewportZoom &viewport_zoom_);

		virtual
		void
		handle_activation();

		
		virtual
		void
		handle_left_click(
				const QPointF &point_on_scene,
				bool is_on_surface);

		virtual
		void
		handle_shift_left_click(
				const QPointF &point_on_scene,
				bool is_on_surface);

	private:

		void
		recentre_map(
				const QPointF &point_on_scene);

		/**
		 * This is the window that has the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		GPlatesGui::MapTransform *d_map_transform_ptr;
		GPlatesGui::ViewportZoom *d_viewport_zoom_ptr;
	};
}

#endif  // GPLATES_CANVASTOOLS_ZOOMMAP_H
