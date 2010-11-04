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

#include "ZoomMap.h"

#include "gui/MapTransform.h"
#include "gui/ViewportZoom.h"

#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::ZoomMap::ZoomMap(
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesGui::MapTransform &map_transform_,
		GPlatesGui::ViewportZoom &viewport_zoom_) :
	MapCanvasTool(map_canvas_, map_view_, map_transform_),
	d_viewport_window_ptr(&viewport_window_),
	d_map_transform_ptr(&map_transform_),
	d_viewport_zoom_ptr(&viewport_zoom_)
{  }


void
GPlatesCanvasTools::ZoomMap::handle_activation()
{
	if (map_view().isVisible())
	{
		d_viewport_window_ptr->status_message(QObject::tr(
					"Click to zoom in."
					" Shift+click to zoom out."
					" Ctrl+drag to pan the map."));
	}
}


void
GPlatesCanvasTools::ZoomMap::recentre_map(
		const QPointF &point_on_scene)
{
	d_map_transform_ptr->set_centre_of_viewport(point_on_scene);
}


void
GPlatesCanvasTools::ZoomMap::handle_left_click(
		const QPointF &point_on_scene, 
		bool is_on_surface)
{
	if (is_on_surface)
	{
		recentre_map(point_on_scene);
		d_viewport_zoom_ptr->zoom_in();
	}
}



void
GPlatesCanvasTools::ZoomMap::handle_shift_left_click(
		const QPointF &point_on_scene, 
		bool is_on_surface)
{
	if (is_on_surface)
	{
		recentre_map(point_on_scene);
		d_viewport_zoom_ptr->zoom_out();
	}
}

