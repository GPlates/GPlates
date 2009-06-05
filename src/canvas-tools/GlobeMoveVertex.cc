/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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


#include <QDebug>

#include "GlobeMoveVertex.h"
#include "CommonMoveVertex.h"

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/MoveVertexGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"


GPlatesCanvasTools::GlobeMoveVertex::GlobeMoveVertex(
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_):
	GlobeCanvasTool(globe_, globe_canvas_),
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


GPlatesCanvasTools::GlobeMoveVertex::~GlobeMoveVertex()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::GlobeMoveVertex::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		GPlatesCanvasTools::CommonMoveVertex::handle_activation(
			d_geometry_operation_target,
			d_move_vertex_geometry_operation.get());

		d_view_state_ptr->status_message(QObject::tr(
			"Drag to move a vertex of the current geometry."
			" Ctrl+drag to re-orient the globe."));
	}
}


void
GPlatesCanvasTools::GlobeMoveVertex::handle_deactivation()
{
	// Deactivate our MoveVertexGeometryOperation.
	d_move_vertex_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::GlobeMoveVertex::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{

	const double closeness_inclusion_threshold = 
		globe_canvas().current_proximity_inclusion_threshold(initial_pos_on_globe);	

	GPlatesCanvasTools::CommonMoveVertex::handle_left_drag(
		d_is_in_drag,
		d_move_vertex_geometry_operation.get(),
		oriented_initial_pos_on_globe,
		closeness_inclusion_threshold,
		oriented_current_pos_on_globe);
}


void
GPlatesCanvasTools::GlobeMoveVertex::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	// In case clicked and released at same time.
	handle_left_drag(
			initial_pos_on_globe,
			oriented_initial_pos_on_globe,
			was_on_globe,
			current_pos_on_globe,
			oriented_current_pos_on_globe,
			is_on_globe,
			oriented_centre_of_viewport);

	d_move_vertex_geometry_operation->end_drag(oriented_current_pos_on_globe);
	d_is_in_drag = false;

}

void
GPlatesCanvasTools::GlobeMoveVertex::handle_move_without_drag(
	const GPlatesMaths::PointOnSphere &current_pos_on_globe,
	const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
	bool is_on_globe,
	const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	double closeness_inclusion_threshold =
		globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe);

	d_move_vertex_geometry_operation->mouse_move(
		oriented_current_pos_on_globe, closeness_inclusion_threshold);
}
