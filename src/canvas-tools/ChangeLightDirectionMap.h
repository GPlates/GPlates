/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_CANVAS_TOOLS_CHANGELIGHTINGMAP_H
#define GPLATES_CANVAS_TOOLS_CHANGELIGHTINGMAP_H

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

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to change the light direction.
	 *
	 * NOTE: Currently this tool does nothing since changing the light direction in a 2D map view
	 * has not yet been implemented.
	 */
	class ChangeLightDirectionMap :
			public GPlatesGui::MapCanvasTool
	{
	public:

		/**
		 * Create a ChangeLightDirectionMap instance.
		 */
		ChangeLightDirectionMap(
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesGui::MapTransform &map_transform_);

	private:

		/**
		 * Used to activate/deactivate focused geometry highlight rendered layer.
		 */
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * This is the window that has the status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		GPlatesGui::MapTransform *d_map_transform_ptr;
	};
}

#endif // GPLATES_CANVAS_TOOLS_CHANGELIGHTINGMAP_H
