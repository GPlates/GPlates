/* $Id$ */

/**
 * \file Derived @a CanvasTool to move vertices of temporary or focused feature geometry.
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

#include "MoveVertex.h"

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/MoveVertexGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"


GPlatesCanvasTools::MoveVertex::MoveVertex(
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_):
	CanvasTool(globe_, globe_canvas_),
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


GPlatesCanvasTools::MoveVertex::~MoveVertex()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::MoveVertex::handle_activation()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Ask which GeometryBuilder we are to operate on.
	// Note: we must pass the type of canvas tool in (see GeometryOperationTarget for explanation).
	// Returned GeometryBuilder should not be NULL but might be if tools are not
	// enable/disabled properly.
	GPlatesViewOperations::GeometryBuilder *geometry_builder =
			d_geometry_operation_target->get_and_set_current_geometry_builder_for_newly_activated_tool(
					GPlatesCanvasTools::CanvasToolType::MOVE_VERTEX);

	// Ask which main rendered layer we are to operate on.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_layer_type =
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER;

	// Activate our MoveVertexGeometryOperation.
	d_move_vertex_geometry_operation->activate(geometry_builder, main_layer_type);

	// FIXME:  We may have to adjust the message if we are using a Map View.
	d_view_state_ptr->status_message(QObject::tr(
			"Drag to move a vertex of the current geometry."
			" Ctrl+drag to re-orient the globe."));
}


void
GPlatesCanvasTools::MoveVertex::handle_deactivation()
{
	// Deactivate our MoveVertexGeometryOperation.
	d_move_vertex_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::MoveVertex::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_drag)
	{
		d_move_vertex_geometry_operation->start_drag(
				initial_pos_on_globe,
				oriented_initial_pos_on_globe);
		d_is_in_drag = true;
	}

	d_move_vertex_geometry_operation->update_drag(oriented_current_pos_on_globe);
}


void
GPlatesCanvasTools::MoveVertex::handle_left_release_after_drag(
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
GPlatesCanvasTools::MoveVertex::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	d_move_vertex_geometry_operation->mouse_move(
			current_pos_on_globe, oriented_current_pos_on_globe);
}
