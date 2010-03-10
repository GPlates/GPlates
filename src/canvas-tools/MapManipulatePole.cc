/* $Id: MapInsertVertex.cc 5702 2009-04-30 13:08:26Z rwatson $ */

/**
 * \file Derived @a CanvasTool to insert vertices into temporary or focused feature geometry.
 * $Revision: 5702 $
 * $Date: 2009-04-30 23:08:26 +1000 (to, 30 apr 2009) $
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "MapManipulatePole.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesCanvasTools::MapManipulatePole::MapManipulatePole(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesQtWidgets::ReconstructionPoleWidget &pole_widget,
		GPlatesGui::MapTransform &map_transform_):
	MapCanvasTool(map_canvas_, map_view_, map_transform_),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_view_state_ptr(&view_state_),
	d_pole_widget_ptr(&pole_widget),
	d_is_in_drag(false)
{
}


GPlatesCanvasTools::MapManipulatePole::~MapManipulatePole()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::MapManipulatePole::handle_activation()
{
	if (map_view().isVisible())
	{
		d_view_state_ptr->status_message(QObject::tr(
			"Pole manipulation tool is not yet available on the map. Use the globe projection to manipulate a pole. "
			" Ctrl+drag to pan the map."));		
	}
}


void
GPlatesCanvasTools::MapManipulatePole::handle_deactivation()
{
	
}


