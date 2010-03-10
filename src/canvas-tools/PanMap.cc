/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include "PanMap.h"

#include "gui/MapTransform.h"
#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ViewportWindow.h"

void
GPlatesCanvasTools::PanMap::handle_activation()
{
	if (map_view().isVisible())
	{
		d_view_state_ptr->status_message(QObject::tr(
			"Drag to pan the map."
			" Shift+drag to rotate the map."));
	}

}

void
GPlatesCanvasTools::PanMap::handle_deactivation()
{

}

void
GPlatesCanvasTools::PanMap::handle_left_click(
	const QPointF &point_on_scene, 
	bool is_on_surface)
{

}

void
GPlatesCanvasTools::PanMap::handle_left_drag(
	const QPointF &initial_point_on_scene,
	bool was_on_surface,
	const QPointF &current_point_on_scene,
	bool is_on_surface,
	const QPointF &translation)
{
	map_transform().translate_maps(translation.x(), translation.y());
}

void
GPlatesCanvasTools::PanMap::handle_shift_left_drag(
	const QPointF &initial_point_on_scene,
	bool was_on_surface,
	const QPointF &current_point_on_scene,
	bool is_on_surface,
	const QPointF &translation)
{
	rotate_map_by_drag(initial_point_on_scene,was_on_surface,current_point_on_scene,is_on_surface,translation);	
}

void
GPlatesCanvasTools::PanMap::handle_shift_left_release_after_drag(
	const QPointF &initial_point_on_scene,
	bool was_on_surface,
	const QPointF &current_point_on_scene,
	bool is_on_surface)
{

}

void
GPlatesCanvasTools::PanMap::handle_shift_left_click(
	const QPointF &point_on_scene,
	bool is_on_surface)
{

}
