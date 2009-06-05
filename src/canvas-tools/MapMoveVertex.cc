/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#include "MapMoveVertex.h"
#include "CommonMoveVertex.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "view-operations/MoveVertexGeometryOperation.h"

GPlatesCanvasTools::MapMoveVertex::MapMoveVertex(
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		const GPlatesQtWidgets::ViewportWindow &view_state_):
	MapCanvasTool(map_canvas_, map_view_),
	d_view_state_ptr(&view_state_),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_geometry_operation_target(&geometry_operation_target),
	d_move_vertex_geometry_operation(
		new GPlatesViewOperations::MoveVertexGeometryOperation(
				geometry_operation_target,
				active_geometry_operation,
				&rendered_geometry_collection,
				choose_canvas_tool,
				query_proximity_threshold)),
	d_is_in_drag(false)
{
}


GPlatesCanvasTools::MapMoveVertex::~MapMoveVertex()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::MapMoveVertex::handle_activation()
{
	if (map_view().isVisible())
	{

		GPlatesCanvasTools::CommonMoveVertex::handle_activation(
			d_geometry_operation_target,
			d_move_vertex_geometry_operation.get());


		d_view_state_ptr->status_message(QObject::tr(
			"Drag to move a vertex of the current geometry."
			" Ctrl+drag to pan the map."));
	}
}


void
GPlatesCanvasTools::MapMoveVertex::handle_deactivation()
{
	// Deactivate our MoveVertexGeometryOperation.
	
	d_move_vertex_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::MapMoveVertex::handle_left_drag(
	const QPointF &initial_point_on_scene,
	bool was_on_surface,
	const QPointF &current_point_on_scene,
	bool is_on_surface,
	const QPointF &translation)
{
	// FIXME: Handle the user trying to drag a vertex off the edge of the map better.
	// Currently the moved vertex stays at the point on the edge of the map where it last was,
	// until the mouse is dragged back onto the map. It looks a bit nicer, and is more 
	// consistent with globe behaviour, if the edge point moves in sync with the mouse.

	if (!is_on_surface)
	{
		return;
	}

	double lon = initial_point_on_scene.x();
	double lat = initial_point_on_scene.y();

	boost::optional<GPlatesMaths::LatLonPoint> initial_llp = 
		map_canvas().projection().inverse_transform(lon,lat);
	
	if (!initial_llp)
	{
		return;
	}
	GPlatesMaths::PointOnSphere initial_pos_on_globe = GPlatesMaths::make_point_on_sphere(*initial_llp);

	lon = current_point_on_scene.x();
	lat = current_point_on_scene.y();

	boost::optional<GPlatesMaths::LatLonPoint> current_llp = 
		map_canvas().projection().inverse_transform(lon,lat);

	if (current_llp)
	{
		GPlatesMaths::PointOnSphere current_pos_on_globe = GPlatesMaths::make_point_on_sphere(*current_llp);

		double closeness_inclusion_threshold = map_view().current_proximity_inclusion_threshold(initial_pos_on_globe);


		GPlatesCanvasTools::CommonMoveVertex::handle_left_drag(
			d_is_in_drag,
			d_move_vertex_geometry_operation.get(),
			initial_pos_on_globe,
			closeness_inclusion_threshold,
			current_pos_on_globe);
	}

}


void
GPlatesCanvasTools::MapMoveVertex::handle_left_release_after_drag(
	const QPointF &initial_point_on_scene,
	bool was_on_surface,
	const QPointF &current_point_on_scene,
	bool is_on_surface,
	const QPointF &translation)
{

	handle_left_drag(
		initial_point_on_scene,
		was_on_surface,
		current_point_on_scene,
		is_on_surface,
		translation);

	double lon = current_point_on_scene.x();
	double lat = current_point_on_scene.y();

	boost::optional<GPlatesMaths::LatLonPoint> current_llp = 
		map_canvas().projection().inverse_transform(lon,lat);
	
	if (current_llp)
	{
		GPlatesMaths::PointOnSphere current_pos_on_globe = GPlatesMaths::make_point_on_sphere(*current_llp);

		d_move_vertex_geometry_operation->end_drag(current_pos_on_globe);
		d_is_in_drag = false;
	}

}


void
GPlatesCanvasTools::MapMoveVertex::handle_move_without_drag(
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{

	if (!is_on_surface)
	{
		return;
	}

	double lon = current_point_on_scene.x();
	double lat = current_point_on_scene.y();

	boost::optional<GPlatesMaths::LatLonPoint> llp = 
		map_canvas().projection().inverse_transform(lon,lat);

	if (llp)
	{
		GPlatesMaths::PointOnSphere point_on_sphere = GPlatesMaths::make_point_on_sphere(*llp);

		double closeness_inclusion_threshold = map_view().current_proximity_inclusion_threshold(point_on_sphere);

		d_move_vertex_geometry_operation->mouse_move(
			point_on_sphere, closeness_inclusion_threshold);
	}
	
}
