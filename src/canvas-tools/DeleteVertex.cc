/* $Id$ */

/**
 * \file Derived @a CanvasTool to delete vertices from a temporary or focused feature geometry.
 * $Revision$
 * $Date$
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

#include "DeleteVertex.h"

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/DeleteVertexGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"


GPlatesCanvasTools::DeleteVertex::DeleteVertex(
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold):
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_geometry_operation_target(&geometry_operation_target),
	d_delete_vertex_geometry_operation(
		new GPlatesViewOperations::DeleteVertexGeometryOperation(
				geometry_operation_target,
				active_geometry_operation,
				&rendered_geometry_collection,
				choose_canvas_tool,
				query_proximity_threshold))
{
}


GPlatesCanvasTools::DeleteVertex::~DeleteVertex()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::DeleteVertex::handle_activation()
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
		GPlatesCanvasTools::CanvasToolType::DELETE_VERTEX);

	// Ask which main rendered layer we are to operate on.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_layer_type =
		GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER;

	// Activate our InsertVertexGeometryOperation.
	d_delete_vertex_geometry_operation->activate(geometry_builder, main_layer_type);

	set_status_bar_message(QT_TR_NOOP("Click to delete a vertex of the current geometry."));
}


void
GPlatesCanvasTools::DeleteVertex::handle_deactivation()
{
	// Deactivate our DeleteVertexGeometryOperation.
	d_delete_vertex_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::DeleteVertex::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_delete_vertex_geometry_operation->left_click(
			point_on_sphere,
			proximity_inclusion_threshold);
}

void
GPlatesCanvasTools::DeleteVertex::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_delete_vertex_geometry_operation->mouse_move(
			point_on_sphere,
			proximity_inclusion_threshold);
}
