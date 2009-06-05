/* $Id$ */

/**
 * \file 
 * Moves points/vertices in a geometry as the user selects a vertex and drags it.
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

#include <memory>
#include <QDebug>
#include <QUndoCommand>

#include "MoveVertexGeometryOperation.h"

#include "ActiveGeometryOperation.h"
#include "GeometryBuilderUndoCommands.h"
#include "GeometryOperationUndo.h"
#include "RenderedGeometryProximity.h"
#include "QueryProximityThreshold.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryLayerVisitor.h"
#include "RenderedGeometryParameters.h"
#include "RenderedGeometryUtils.h"
#include "UndoRedo.h"
#include "gui/ChooseCanvasTool.h"
#include "maths/PointOnSphere.h"
#include "maths/ProximityCriteria.h"


GPlatesViewOperations::MoveVertexGeometryOperation::MoveVertexGeometryOperation(
		GeometryOperationTarget &geometry_operation_target,
		ActiveGeometryOperation &active_geometry_operation,
		RenderedGeometryCollection *rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const QueryProximityThreshold &query_proximity_threshold) :
d_geometry_builder(NULL),
d_geometry_operation_target(&geometry_operation_target),
d_active_geometry_operation(&active_geometry_operation),
d_rendered_geometry_collection(rendered_geometry_collection),
d_choose_canvas_tool(&choose_canvas_tool),
d_query_proximity_threshold(&query_proximity_threshold),
d_selected_vertex_index(0),
d_is_vertex_selected(false)
{
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::activate(
		GeometryBuilder *geometry_builder,
		RenderedGeometryCollection::MainLayerType main_layer_type)
{
	// Do nothing if NULL geometry builder.
	if (geometry_builder == NULL)
	{
		return;
	}

	// Let others know we're the currently activated GeometryOperation.
	d_active_geometry_operation->set_active_geometry_operation(this);

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	d_geometry_builder = geometry_builder;
	d_main_layer_type = main_layer_type;

	// Activate the main rendered layer.
	d_rendered_geometry_collection->set_main_layer_active(main_layer_type);

	// Deactivate all rendered geometry layers of our main rendered layer.
	// This hides what other tools have drawn into our main rendered layer.
	RenderedGeometryUtils::deactivate_rendered_geometry_layers(
			*d_rendered_geometry_collection, main_layer_type);

	connect_to_geometry_builder_signals();

	// Create the rendered geometry layers required by the GeometryBuilder state
	// and activate/deactivate appropriate layers.
	create_rendered_geometry_layers();

	// Activate our render layers so they become visible.
	d_lines_layer_ptr->set_active(true);
	d_points_layer_ptr->set_active(true);
	d_highlight_point_layer_ptr->set_active(true);

	// Fill the rendered layers with RenderedGeometry objects by querying
	// the GeometryBuilder state.
	update_rendered_geometries();
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::deactivate()
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}

	emit_unhighlight_signal(d_geometry_builder);

	// Let others know there's no currently activated GeometryOperation.
	d_active_geometry_operation->set_no_active_geometry_operation();

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	if (d_geometry_builder != NULL)
	{
		disconnect_from_geometry_builder_signals();
	}

	// NOTE: we don't deactivate our rendered layers because they still need
	// to be visible even after we've deactivated.  They will remain in existance
	// until activate() is called again on this object.

	// User will have to click another vertex when this operation activates again.
	d_is_vertex_selected = false;

	// Not using this GeometryBuilder anymore.
	d_geometry_builder = NULL;
}

#if 0
void
GPlatesViewOperations::MoveVertexGeometryOperation::start_drag(
		const GPlatesMaths::PointOnSphere &clicked_pos_on_sphere,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	//
	// See if the user selected a vertex with their mouse click.
	//

	boost::optional<RenderedGeometryProximityHit> closest_hit = test_proximity_to_points(
			clicked_pos_on_sphere, oriented_pos_on_sphere);
	if (closest_hit)
	{
		// The index of the vertex selected corresponds to index of vertex in
		// the geometry.
		// NOTE: this will have to be changed when multiple internal geometries are
		// possible in the GeometryBuilder.
		d_selected_vertex_index = closest_hit->d_rendered_geom_index;

		// Get a unique command id so that all move vertex commands in the
		// current mouse drag will be merged together.
		// This id will be released for reuse when the last copy of it is destroyed.
		d_move_vertex_command_id = UndoRedo::instance().get_unique_command_id();

		d_is_vertex_selected = true;

		// Highlight the vertex the mouse is currently hovering over.
		update_highlight_rendered_point(d_selected_vertex_index);
	}
}
#else


void
GPlatesViewOperations::MoveVertexGeometryOperation::start_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	//
	// See if the user selected a vertex with their mouse click.
	//


	boost::optional<RenderedGeometryProximityHit> closest_hit = test_proximity_to_points(
		oriented_pos_on_sphere, closeness_inclusion_threshold);

	if (closest_hit)
	{
		// The index of the vertex selected corresponds to index of vertex in
		// the geometry.
		// NOTE: this will have to be changed when multiple internal geometries are
		// possible in the GeometryBuilder.
		d_selected_vertex_index = closest_hit->d_rendered_geom_index;

		// Get a unique command id so that all move vertex commands in the
		// current mouse drag will be merged together.
		// This id will be released for reuse when the last copy of it is destroyed.
		d_move_vertex_command_id = UndoRedo::instance().get_unique_command_id();

		d_is_vertex_selected = true;

		// Highlight the vertex the mouse is currently hovering over.
		update_highlight_rendered_point(d_selected_vertex_index);
	}

}
#endif

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// If a vertex was selected when user first clicked mouse then move the vertex.
	if (d_is_vertex_selected)
	{
		move_vertex(oriented_pos_on_sphere, true/*is_intermediate_move*/);

		// Highlight the vertex the mouse is currently hovering over.
		update_highlight_rendered_point(d_selected_vertex_index);
	}
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::end_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}

	// If a vertex was selected when user first clicked mouse then move the vertex.
	if (d_is_vertex_selected)
	{
		// Do the final move vertex command to signal that this is the final
		// move of this drag.
		move_vertex(oriented_pos_on_sphere, false/*is_intermediate_move*/);

		// Highlight the vertex the mouse is currently hovering over.
		update_highlight_rendered_point(d_selected_vertex_index);
	}

	// Release our handle on the command id.
	d_move_vertex_command_id = UndoRedo::CommandId();

	d_is_vertex_selected = false;
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::mouse_move(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}

	//
	// See if the mouse cursor is near a vertex and highlight it if it is.
	//

	// Clear any currently highlighted point first.
	d_highlight_point_layer_ptr->clear_rendered_geometries();

	boost::optional<RenderedGeometryProximityHit> closest_hit = test_proximity_to_points(
			oriented_pos_on_sphere, closeness_inclusion_threshold);
	if (closest_hit)
	{
		const GeometryBuilder::PointIndex highlight_vertex_index = closest_hit->d_rendered_geom_index;

		update_highlight_rendered_point(highlight_vertex_index);

		// Currently only one internal geometry is supported so set geometry index to zero.
		const GeometryBuilder::GeometryIndex geometry_index = 0;

		emit_highlight_point_signal(d_geometry_builder,
				geometry_index,
				highlight_vertex_index,
				GeometryOperationParameters::HIGHLIGHT_COLOUR);
	}
	else
	{
		emit_unhighlight_signal(d_geometry_builder);
	}
}

boost::optional<GPlatesViewOperations::RenderedGeometryProximityHit>
GPlatesViewOperations::MoveVertexGeometryOperation::test_proximity_to_points(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{

	GPlatesMaths::ProximityCriteria proximity_criteria(
			oriented_pos_on_sphere,
			closeness_inclusion_threshold);

	sorted_rendered_geometry_proximity_hits_type sorted_hits;
	if (!test_proximity(sorted_hits, *d_points_layer_ptr, proximity_criteria))
	{
		return boost::none;
	}

	// Only interested in the closest vertex in the layer.
	const RenderedGeometryProximityHit &closest_hit = sorted_hits.front();

	return closest_hit;
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::create_rendered_geometry_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layer to draw the line segments of polylines and polygons.
	d_lines_layer_ptr =
		d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
				d_main_layer_type);

	// Create a rendered layer to draw the points in the geometry on top of the lines.
	// NOTE: this must be created second to get drawn on top.
	d_points_layer_ptr =
		d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
				d_main_layer_type);

	// Create a rendered layer to draw a single point in the geometry on top of the usual points
	// when the mouse cursor hovers over one of them.
	// NOTE: this must be created third to get drawn on top of the points.
	d_highlight_point_layer_ptr =
		d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
				d_main_layer_type);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::connect_to_geometry_builder_signals()
{
	// Connect to the current geometry builder's signals.

	// GeometryBuilder has just finished updating geometry.
	QObject::connect(
			d_geometry_builder,
			SIGNAL(stopped_updating_geometry()),
			this,
			SLOT(geometry_builder_stopped_updating_geometry()));
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::disconnect_from_geometry_builder_signals()
{
	// Disconnect all signals from the current geometry builder.
	QObject::disconnect(d_geometry_builder, 0, this, 0);
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::geometry_builder_stopped_updating_geometry()
{
	// The geometry builder has just potentially done a group of
	// geometry modifications and is now notifying us that it's finished.

	// Just clear and add all RenderedGeometry objects.
	// This could be optimised, if profiling says so, by listening to the other signals
	// generated by GeometryBuilder instead and only making the minimum changes needed.
	update_rendered_geometries();
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::move_vertex(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		bool is_intermediate_move)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// The command that does the actual moving of vertex.
	std::auto_ptr<QUndoCommand> move_vertex_command(
			new GeometryBuilderMovePointUndoCommand(
					d_geometry_builder,
					d_selected_vertex_index,
					oriented_pos_on_sphere,
					is_intermediate_move));

	// Command wraps move vertex command with handing canvas tool choice and
	// move vertex tool activation.
	std::auto_ptr<QUndoCommand> undo_command(
			new GeometryOperationUndoCommand(
					QObject::tr("move vertex"),
					move_vertex_command,
					this,
					d_geometry_operation_target,
					d_main_layer_type,
					d_choose_canvas_tool,
					&GPlatesGui::ChooseCanvasTool::choose_move_vertex_tool,
					d_move_vertex_command_id));

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the vertex is initially moved.
	UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_rendered_geometries()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Clear all RenderedGeometry objects from the render layers first.
	d_lines_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();
	d_highlight_point_layer_ptr->clear_rendered_geometries();

	// If a vertex is currently selected then draw it highlighted.
	if (d_is_vertex_selected)
	{
		update_highlight_rendered_point(d_selected_vertex_index);
	}

	// Iterate through the internal geometries (currently only one is supported).
	for (GeometryBuilder::GeometryIndex geom_index = 0;
		geom_index < d_geometry_builder->get_num_geometries();
		++geom_index)
	{
		update_rendered_geometry(geom_index);
	}
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_rendered_geometry(
		GeometryBuilder::GeometryIndex geom_index)
{
	// All types of geometry have the points drawn the same.
	add_rendered_points(geom_index);

	const GeometryType::Value actual_geom_type =
		d_geometry_builder->get_actual_type_of_geometry(geom_index);

	switch (actual_geom_type)
	{
	case GeometryType::POLYLINE:
		add_rendered_lines_for_polyline_on_sphere(geom_index);
		break;

	case GeometryType::POLYGON:
		add_rendered_lines_for_polygon_on_sphere(geom_index);
		break;

	default:
		// Do nothing.
		break;
	}
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::add_rendered_lines_for_polyline_on_sphere(
		GeometryBuilder::GeometryIndex geom_index)
{
	// Get start and end of point sequence in current geometry.
	GeometryBuilder::point_const_iterator_type builder_geom_begin =
		d_geometry_builder->get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder->get_geometry_point_end(geom_index);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
		GPlatesMaths::PolylineOnSphere::create_on_heap(builder_geom_begin, builder_geom_end);

	RenderedGeometry rendered_geom = create_rendered_polyline_on_sphere(
				polyline_on_sphere,
				GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
				GeometryOperationParameters::LINE_WIDTH_HINT);

	// Add to the lines layer.
	d_lines_layer_ptr->add_rendered_geometry(rendered_geom);
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::add_rendered_lines_for_polygon_on_sphere(
		GeometryBuilder::GeometryIndex geom_index)
{
	// Get start and end of point sequence in current geometry.
	GeometryBuilder::point_const_iterator_type builder_geom_begin =
		d_geometry_builder->get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder->get_geometry_point_end(geom_index);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
		GPlatesMaths::PolygonOnSphere::create_on_heap(builder_geom_begin, builder_geom_end);

	RenderedGeometry rendered_geom = create_rendered_polygon_on_sphere(
				polygon_on_sphere,
				GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
				GeometryOperationParameters::LINE_WIDTH_HINT);

	// Add to the lines layer.
	d_lines_layer_ptr->add_rendered_geometry(rendered_geom);
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::add_rendered_points(
		GeometryBuilder::GeometryIndex geom_index)
{
	GeometryBuilder::point_const_iterator_type builder_geom_begin =
		d_geometry_builder->get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder->get_geometry_point_end(geom_index);

	GeometryBuilder::point_const_iterator_type builder_geom_iter;
	for (builder_geom_iter = builder_geom_begin;
		builder_geom_iter != builder_geom_end;
		++builder_geom_iter)
	{
		const GPlatesMaths::PointOnSphere &point_on_sphere = *builder_geom_iter;

		RenderedGeometry rendered_geom = create_rendered_point_on_sphere(
			point_on_sphere,
			GeometryOperationParameters::FOCUS_COLOUR,
			GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(rendered_geom);
	}
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_highlight_rendered_point(
		const GeometryBuilder::PointIndex highlight_point_index)
{
	// Clear any geometry before adding.
	d_highlight_point_layer_ptr->clear_rendered_geometries();

	// Currently only one internal geometry is supported so set geometry index to zero.
	const GeometryBuilder::GeometryIndex geometry_index = 0;

	// Get the highlighted point.
	const GPlatesMaths::PointOnSphere &highlight_point_on_sphere =
			d_geometry_builder->get_geometry_point(geometry_index, highlight_point_index);

	RenderedGeometry rendered_geom = create_rendered_point_on_sphere(
		highlight_point_on_sphere,
		GeometryOperationParameters::HIGHLIGHT_COLOUR,
		GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

	d_highlight_point_layer_ptr->add_rendered_geometry(rendered_geom);
}
