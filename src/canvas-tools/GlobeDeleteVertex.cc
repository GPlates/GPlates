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

#include "GlobeDeleteVertex.h"

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/DeleteVertexGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"


GPlatesCanvasTools::GlobeDeleteVertex::GlobeDeleteVertex(
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
	d_delete_vertex_geometry_operation(
		new GPlatesViewOperations::DeleteVertexGeometryOperation(
				geometry_operation_target,
				active_geometry_operation,
				&rendered_geometry_collection,
				choose_canvas_tool,
				query_proximity_threshold))
{
}


GPlatesCanvasTools::GlobeDeleteVertex::~GlobeDeleteVertex()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesCanvasTools::GlobeDeleteVertex::handle_activation()
{
	if (globe_canvas().isVisible())
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

		// FIXME:  We may have to adjust the message if we are using a Map View.
		d_view_state_ptr->status_message(QObject::tr(
			"Click to delete a vertex of the current geometry."
			" Ctrl+drag to re-orient the globe."));
	}
}


void
GPlatesCanvasTools::GlobeDeleteVertex::handle_deactivation()
{
	// Deactivate our GlobeDeleteVertexGeometryOperation.
	d_delete_vertex_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::GlobeDeleteVertex::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{

	double closeness_inclusion_threshold =
		globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);

	d_delete_vertex_geometry_operation->left_click(
			oriented_click_pos_on_globe,
			closeness_inclusion_threshold);
}

void
GPlatesCanvasTools::GlobeDeleteVertex::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	double closeness_inclusion_threshold =
		globe_canvas().current_proximity_inclusion_threshold(current_pos_on_globe);

	d_delete_vertex_geometry_operation->mouse_move(
			oriented_current_pos_on_globe,closeness_inclusion_threshold);
}
