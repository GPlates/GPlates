/* $Id$ */

/**
 * \file 
 * Interface for choosing canvas tools.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "ChooseCanvasTool.h"

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::ChooseCanvasTool::ChooseCanvasTool(
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
d_most_recent_tool_type(NONE),
d_most_recent_digitise_geom_tool_type(NONE),
d_viewport_window(&viewport_window)
{
}

void
GPlatesGui::ChooseCanvasTool::choose_most_recent_digitise_geometry_tool()
{
	switch (d_most_recent_digitise_geom_tool_type)
	{
	case DIGITISE_POLYLINE:
		choose_digitise_polyline_tool();
		break;

	case DIGITISE_MULTIPOINT:
		choose_digitise_multipoint_tool();
		break;

	case DIGITISE_POLYGON:
		choose_digitise_polygon_tool();
		break;

	default:
		// A digitise geometry tool has never been chosen yet so do nothing.
		break;
	}
}

void
GPlatesGui::ChooseCanvasTool::choose_drag_globe_tool()
{
	d_viewport_window->choose_drag_globe_tool();

	d_most_recent_tool_type = DRAG_GLOBE;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_zoom_globe_tool()
{
	d_viewport_window->choose_zoom_globe_tool();

	d_most_recent_tool_type = ZOOM_GLOBE;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_click_geometry_tool()
{
	d_viewport_window->choose_click_geometry_tool();

	d_most_recent_tool_type = CLICK_GEOMETRY;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_digitise_polyline_tool()
{
	d_viewport_window->choose_digitise_polyline_tool();

	d_most_recent_tool_type = DIGITISE_POLYLINE;
	d_most_recent_digitise_geom_tool_type = DIGITISE_POLYLINE;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_digitise_multipoint_tool()
{
	d_viewport_window->choose_digitise_multipoint_tool();

	d_most_recent_tool_type = DIGITISE_MULTIPOINT;
	d_most_recent_digitise_geom_tool_type = DIGITISE_MULTIPOINT;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_digitise_polygon_tool()
{
	d_viewport_window->choose_digitise_polygon_tool();

	d_most_recent_tool_type = DIGITISE_POLYGON;
	d_most_recent_digitise_geom_tool_type = DIGITISE_POLYGON;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_move_geometry_tool()
{
	d_viewport_window->choose_move_geometry_tool();

	d_most_recent_tool_type = MOVE_GEOMETRY;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_move_vertex_tool()
{
	d_viewport_window->choose_move_vertex_tool();

	d_most_recent_tool_type = MOVE_VERTEX;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}

void
GPlatesGui::ChooseCanvasTool::choose_manipulate_pole_tool()
{
	d_viewport_window->choose_manipulate_pole_tool();

	d_most_recent_tool_type = MANIPULATE_POLE;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}


void
GPlatesGui::ChooseCanvasTool::choose_create_topology_tool()
{
	d_viewport_window->choose_create_topology_tool();

	d_most_recent_tool_type = CREATE_TOPOLOGY;

	emit chose_canvas_tool(*this, d_most_recent_tool_type);
}


GPlatesGui::ChooseCanvasToolUndoCommand::ChooseCanvasToolUndoCommand(
		GPlatesGui::ChooseCanvasTool *choose_canvas_tool,
		choose_canvas_tool_method_type choose_canvas_tool_method,
		QUndoCommand *parent) :
QUndoCommand(parent),
d_choose_canvas_tool(choose_canvas_tool),
d_choose_canvas_tool_method(choose_canvas_tool_method),
d_first_redo(true)
{
}

void
GPlatesGui::ChooseCanvasToolUndoCommand::redo()
{
	// Don't do anything the first call to 'redo()' because we're
	// already in the right tool (we just added a point).
	if (d_first_redo)
	{
		d_first_redo = false;
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Choose the canvas tool.
	(d_choose_canvas_tool->*d_choose_canvas_tool_method)();
}

void
GPlatesGui::ChooseCanvasToolUndoCommand::undo()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Choose the canvas tool.
	(d_choose_canvas_tool->*d_choose_canvas_tool_method)();
}
