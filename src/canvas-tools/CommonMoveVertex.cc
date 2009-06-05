/* $Id$ */

/**
 * \file Derived @a CanvasTool to move vertices of temporary or focused feature geometry.
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

#include <QDebug>

#include "CommonMoveVertex.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/MoveVertexGeometryOperation.h"
#include "view-operations/RenderedGeometryCollection.h"


void
GPlatesCanvasTools::CommonMoveVertex::handle_activation(
	GPlatesViewOperations::GeometryOperationTarget *geometry_operation_target,
	GPlatesViewOperations::MoveVertexGeometryOperation *move_vertex_geometry_operation)
{	
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Ask which GeometryBuilder we are to operate on.
	// Note: we must pass the type of canvas tool in (see GeometryOperationTarget for explanation).
	// Returned GeometryBuilder should not be NULL but might be if tools are not
	// enable/disabled properly.
	GPlatesViewOperations::GeometryBuilder *geometry_builder =
			geometry_operation_target->get_and_set_current_geometry_builder_for_newly_activated_tool(
					GPlatesCanvasTools::CanvasToolType::MOVE_VERTEX);

	// Ask which main rendered layer we are to operate on.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_layer_type =
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER;

	// Activate our MoveVertexGeometryOperation.
	move_vertex_geometry_operation->activate(geometry_builder, main_layer_type);

}

void
GPlatesCanvasTools::CommonMoveVertex::handle_left_drag(
	bool &is_in_drag,
	GPlatesViewOperations::MoveVertexGeometryOperation *move_vertex_geometry_operation,
	const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
	const double &closeness_inclusion_threshold,
	const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe)
{

	if ( ! is_in_drag)
	{
		move_vertex_geometry_operation->start_drag(
			oriented_initial_pos_on_globe,
			closeness_inclusion_threshold);

		is_in_drag = true;
	}

	move_vertex_geometry_operation->update_drag(oriented_current_pos_on_globe);

}
