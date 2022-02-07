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

#include "MoveVertex.h"

#include "view-operations/MoveVertexGeometryOperation.h"
#include "view-operations/MoveVertexGeometryOperation.h"
#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesMaths
{
	class PointOnSphere;
}

GPlatesCanvasTools::MoveVertex::MoveVertex(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::GeometryBuilder &geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		GPlatesGui::FeatureFocus &feature_focus) :
	CanvasTool(status_bar_callback),
	d_move_vertex_geometry_operation(
			new GPlatesViewOperations::MoveVertexGeometryOperation(
					geometry_builder,
					geometry_operation_state,
					modify_geometry_state,
					rendered_geometry_collection,
					main_rendered_layer_type,
					canvas_tool_workflows,
					query_proximity_threshold,
					feature_focus)),
	d_is_in_drag(false)
{  }


GPlatesCanvasTools::MoveVertex::~MoveVertex()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::MoveVertex::handle_activation()
{
	// Activate our MoveVertexGeometryOperation.
	d_move_vertex_geometry_operation->activate();

	set_status_bar_message(QT_TR_NOOP("Drag to move a vertex of the current geometry."));
}


void
GPlatesCanvasTools::MoveVertex::handle_deactivation()
{
	// Deactivate our MoveVertexGeometryOperation.
	d_move_vertex_geometry_operation->deactivate();
}

void
GPlatesCanvasTools::MoveVertex::handle_left_press(
		const GPlatesMaths::PointOnSphere &point_on_sphere, 
		bool is_on_earth, 
		double proximity_inclusion_threshold)
{
	if (is_on_earth)
	{
		d_move_vertex_geometry_operation.get()->left_press(
			point_on_sphere,
			proximity_inclusion_threshold);
	}
	
}

void
GPlatesCanvasTools::MoveVertex::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere, 
		bool is_on_earth, 
		double proximity_inclusion_threshold)
{
	d_move_vertex_geometry_operation.get()->release_click();
}


void
GPlatesCanvasTools::MoveVertex::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	handle_left_drag(
			d_is_in_drag,
			d_move_vertex_geometry_operation.get(),
			initial_point_on_sphere,
			initial_proximity_inclusion_threshold,
			current_point_on_sphere);
}

void
GPlatesCanvasTools::MoveVertex::handle_left_drag(
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

void
GPlatesCanvasTools::MoveVertex::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	// In case clicked and released at same time.
	handle_left_drag(
			initial_point_on_sphere,
			was_on_earth,
			initial_proximity_inclusion_threshold,
			current_point_on_sphere,
			is_on_earth,
			current_proximity_inclusion_threshold,
			centre_of_viewport);

	d_move_vertex_geometry_operation->end_drag(current_point_on_sphere);
	d_is_in_drag = false;
}

void
GPlatesCanvasTools::MoveVertex::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_move_vertex_geometry_operation->mouse_move(
			point_on_sphere,
			proximity_inclusion_threshold);
}
