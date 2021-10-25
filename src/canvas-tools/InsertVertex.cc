/* $Id$ */

/**
 * \file Derived @a CanvasTool to insert vertices into temporary or focused feature geometry.
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

#include "InsertVertex.h"

#include "view-operations/InsertVertexGeometryOperation.h"


GPlatesCanvasTools::InsertVertex::InsertVertex(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::GeometryBuilder &geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold) :
	CanvasTool(status_bar_callback),
	d_insert_vertex_geometry_operation(
			new GPlatesViewOperations::InsertVertexGeometryOperation(
					geometry_builder,
					geometry_operation_state,
					rendered_geometry_collection,
					main_rendered_layer_type,
					canvas_tool_workflows,
					query_proximity_threshold))
{  }


GPlatesCanvasTools::InsertVertex::~InsertVertex()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::InsertVertex::handle_activation()
{
	// Activate our InsertVertexGeometryOperation.
	d_insert_vertex_geometry_operation->activate();

	set_status_bar_message(QT_TR_NOOP("Click to insert a vertex into the current geometry."));
}


void
GPlatesCanvasTools::InsertVertex::handle_deactivation()
{
	// Deactivate our InsertVertexGeometryOperation.
	d_insert_vertex_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::InsertVertex::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_insert_vertex_geometry_operation->left_click(
			point_on_sphere,
			proximity_inclusion_threshold);
}

void
GPlatesCanvasTools::InsertVertex::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	d_insert_vertex_geometry_operation->mouse_move(
			current_point_on_sphere,
			current_proximity_inclusion_threshold);
}

void
GPlatesCanvasTools::InsertVertex::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_insert_vertex_geometry_operation->mouse_move(
			point_on_sphere,
			proximity_inclusion_threshold);
}
