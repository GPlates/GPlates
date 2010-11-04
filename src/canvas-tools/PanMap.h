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


namespace GPlatesGui
{
	class MapTransform;
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
	class PanMap:
			public GPlatesGui::MapCanvasTool
	{
	public:

		/**
		 * Create a PanMap instance.
		 */
		explicit
		PanMap(
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::MapTransform &map_transform_):
			MapCanvasTool(map_canvas_, map_view_, map_transform_),
			d_view_state_ptr(&view_state_)
		{  }

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
				const QPointF &translation
		);


		virtual
		void
		handle_shift_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

	private:

		/**
		 * This is the View State used to pass messages to the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;
	};
}

#endif  // GPLATES_CANVASTOOLS_PANMAP_H
