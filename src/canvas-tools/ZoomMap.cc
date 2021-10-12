/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#include "ZoomMap.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ViewportWindow.h"

void
GPlatesCanvasTools::ZoomMap::handle_activation()
{
	if (map_view().isVisible())
	{
		d_view_state_ptr->status_message(QObject::tr(
			"Click to zoom in."
			" Shift+click to zoom out."
			" Ctrl+drag to pan the map."));
	}
}


void
GPlatesCanvasTools::ZoomMap::handle_left_click(
	const QPointF &point_on_scene, 
	bool is_on_surface)
{

	if (is_on_surface)
	{
		map_view().centerOn(point_on_scene);
		map_view().update_centre_of_viewport();
		map_view().viewport_zoom().zoom_in();
	}
}



void
GPlatesCanvasTools::ZoomMap::handle_shift_left_click(
	const QPointF &point_on_scene, 
	bool is_on_surface)
{
	map_view().viewport_zoom().zoom_out();
}

