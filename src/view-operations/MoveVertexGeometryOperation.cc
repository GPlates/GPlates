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
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <QUndoCommand>

#include "GeometryBuilderUndoCommands.h"
#include "RenderedGeometryProximity.h"
#include "MoveVertexGeometryOperation.h"
#include "QueryProximityThreshold.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryLayerVisitor.h"
#include "RenderedGeometryUtils.h"
#include "UndoRedo.h"
#include "gui/ChooseCanvasTool.h"
#include "maths/PointOnSphere.h"
#include "maths/ProximityCriteria.h"


namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Undo/redo command for grouping child commands into one command.
		 * The base @a QUndoCommand already does this - we just add
		 * @a RenderedGeometryCollection update guards to ensure only one
		 * update signal is generated within a @a redo or @a undo call.
		 */
		class GroupMoveVertexUndoCommand :
			public QUndoCommand
		{
		public:
			GroupMoveVertexUndoCommand(
					const QString &text_,
					UndoRedo::CommandId command_id,
					MoveVertexGeometryOperation *move_vertex_operation,
					GeometryBuilder *geometry_builder,
					GeometryBuilder::PointIndex selected_vertex_index,
					const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
					bool is_intermediate_move,
					RenderedGeometryCollection::MainLayerType main_layer_type,
					RenderedGeometryCollection &rendered_geom_collection,
					GPlatesGui::ChooseCanvasTool *choose_canvas_tool,
					QUndoCommand *parent_ = 0) :
				QUndoCommand(text_, parent_),
				d_first_redo(true),
				d_command_id(command_id),
				d_move_vertex_operation(move_vertex_operation),
				d_geometry_builder(geometry_builder),
				d_main_layer_type(main_layer_type),
				d_rendered_geom_collection(&rendered_geom_collection),
				d_move_vertex_command(
					// Add child undo command for moving a vertex.
					new GeometryBuilderMovePointUndoCommand(
							geometry_builder,
							selected_vertex_index,
							oriented_pos_on_sphere,
							is_intermediate_move)),
				d_choose_canvas_tool_command(
					// Add child undo command for selecting the move vertex tool.
					// When or if this undo/redo command gets called the move
					// vertex tool may not be active so make sure it gets activated
					// so user can see what's being undone/redone.
					new GPlatesGui::ChooseCanvasToolUndoCommand(
							choose_canvas_tool,
							&GPlatesGui::ChooseCanvasTool::choose_move_vertex_tool))
			{
			}

			virtual
			void
			redo()
			{
				// Delay any notification of changes to the rendered geometry collection
				// until end of current scope block.
				GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

				// Visit child commands.
				//
				// 1) Activate canvas tool - show appropriate high-level GUI stuff.
				// 2) Activate move vertex operation so it's in same state as when
				//    it performed the original operation.
				// 3) Redo the move vertex operation.

				d_choose_canvas_tool_command->redo();

				// Don't do anything the first call to 'redo()' because the
				// move vertex operation is already activated.
				if (!d_first_redo)
				{
					d_move_vertex_operation->deactivate();
					d_move_vertex_operation->activate(d_geometry_builder, d_main_layer_type);
				}

				d_move_vertex_command->redo();

				d_first_redo = false;
			}

			virtual
			void
			undo()
			{
				// Delay any notification of changes to the rendered geometry collection
				// until end of current scope block.
				GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

				// Visit child commands.
				//
				// 1) Activate canvas tool - show appropriate high-level GUI stuff.
				// 2) Activate move vertex operation so it's in same state as when
				//    it performed the original operation.
				// 3) Undo the move vertex operation.
				d_choose_canvas_tool_command->undo();
				d_move_vertex_operation->deactivate();
				d_move_vertex_operation->activate(d_geometry_builder, d_main_layer_type);
				d_move_vertex_command->undo();
			}

			virtual
			int
			id() const
			{
				return d_command_id.get_id();
			}

			/**
			 * Merge this move command with another move command.
			 * Returns @a true if merged in which case other command will be
			 * deleted by Qt and this command will coalesce both commands.
			 */
			virtual
			bool
			mergeWith(
					const QUndoCommand *other_command)
			{
				// If other command is same type as us then coalesce its command into us.
				const GroupMoveVertexUndoCommand *other_group_move_command =
					dynamic_cast<const GroupMoveVertexUndoCommand *>(other_command);

				if (other_group_move_command != NULL)
				{
					// Merge the other move vertex command with ours.
					d_move_vertex_command->mergeWith(
							other_group_move_command->d_move_vertex_command.get());

					// Forget about the other select canvas tool command as it does
					// the same as ours.

					return true;
				}

				return false;
			}

		private:
			bool d_first_redo;
			UndoRedo::CommandId d_command_id;
			MoveVertexGeometryOperation *d_move_vertex_operation;
			GeometryBuilder *d_geometry_builder;
			RenderedGeometryCollection::MainLayerType d_main_layer_type;
			RenderedGeometryCollection *d_rendered_geom_collection;
			boost::scoped_ptr<GeometryBuilderMovePointUndoCommand> d_move_vertex_command;
			boost::scoped_ptr<GPlatesGui::ChooseCanvasToolUndoCommand> d_choose_canvas_tool_command;
		};
	}
}

GPlatesViewOperations::MoveVertexGeometryOperation::MoveVertexGeometryOperation(
		RenderedGeometryCollection *rendered_geometry_collection,
		RenderedGeometryFactory *rendered_geometry_factory,
		const GeometryOperationRenderParameters &geom_operation_render_parameters,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const QueryProximityThreshold &query_proximity_threshold) :
d_geometry_builder(NULL),
d_rendered_geometry_collection(rendered_geometry_collection),
d_rendered_geometry_factory(rendered_geometry_factory),
d_choose_canvas_tool(&choose_canvas_tool),
d_geom_operation_render_parameters(geom_operation_render_parameters),
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
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	d_geometry_builder = geometry_builder;
	d_main_layer_type = main_layer_type;

	// Activate the main rendered layer.
	d_rendered_geometry_collection->set_main_layer_active(main_layer_type);

	connect_to_geometry_builder_signals();

	// Create the rendered geometry layers required by the GeometryBuilder state
	// and activate/deactivate appropriate layers.
	create_rendered_geometry_layers();

	// Fill the rendered layers with RenderedGeometry objects by querying
	// the GeometryBuilder state.
	update_rendered_geometries();
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::deactivate()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	disconnect_from_geometry_builder_signals();

	// NOTE: we don't deactivate our rendered layers because they still need
	// to be visible even after we've deactivated.  They will remain in existance
	// until activate() is called again on this object.

	// User will have to click another vertex when this operation activates again.
	d_is_vertex_selected = false;

	// Not using this GeometryBuilder anymore.
	d_geometry_builder = NULL;
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::start_drag(
		const GPlatesMaths::PointOnSphere &clicked_pos_on_sphere,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	//
	// See if the user selected a vertex with their mouse click.
	//

	const double closeness_inclusion_threshold =
		d_query_proximity_threshold->current_proximity_inclusion_threshold(
				clicked_pos_on_sphere);

	GPlatesMaths::ProximityCriteria proximity_criteria(
			oriented_pos_on_sphere,
			closeness_inclusion_threshold);

	sorted_digitisation_proximity_hits_type sorted_hits;
	if (test_proximity(sorted_hits, *d_points_layer_ptr, proximity_criteria))
	{
		// Only interested in the closest vertex in the layer.
		const RenderedGeometryProximityHit &closest_hit = sorted_hits.front();

		// The index of the vertex selected corresponds to index of vertex in
		// the geometry.
		// NOTE: this will have to be changed when multiple internal geometries are
		// possible in the GeometryBuilder.
		d_selected_vertex_index = closest_hit.d_rendered_geom_index;

		// Get a unique command id so that all move vertex commands in the
		// current mouse drag will be merged together.
		// This id will be released for reuse when the last copy of it is destroyed.
		d_move_vertex_command_id = UndoRedo::instance().get_unique_command_id();

		d_is_vertex_selected = true;
	}
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// If a vertex was selected when user first clicked mouse then move the vertex.
	if (d_is_vertex_selected)
	{
		move_vertex(oriented_pos_on_sphere, true/*is_intermediate_move*/);
	}
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::end_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// If a vertex was selected when user first clicked mouse then move the vertex.
	if (d_is_vertex_selected)
	{
		// Do the final move vertex command to signal that this is the final
		// move of this drag.
		move_vertex(oriented_pos_on_sphere, false/*is_intermediate_move*/);
	}

	// Release our handle on the command id.
	d_move_vertex_command_id = UndoRedo::CommandId();

	d_is_vertex_selected = false;
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
	// Group two undo commands into one - move vertex and select move-vertex
	// canvas tool.
	std::auto_ptr<QUndoCommand> undo_command(
		new GroupMoveVertexUndoCommand(
		QObject::tr("move vertex"),
		d_move_vertex_command_id,
		this,
		d_geometry_builder,
		d_selected_vertex_index,
		oriented_pos_on_sphere,
		is_intermediate_move,
		d_main_layer_type,
		*d_rendered_geometry_collection,
		d_choose_canvas_tool));

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

	// Deactivate all rendered geometry layers of the main rendered layer 'main_layer_type'.
	deactivate_rendered_geometry_layers(*d_rendered_geometry_collection, d_main_layer_type);

	// Activate our render layers so they become visible.
	d_lines_layer_ptr->set_active(true);
	d_points_layer_ptr->set_active(true);

	// Clear all RenderedGeometry objects from the render layers first.
	d_lines_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();

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

	RenderedGeometry rendered_geom =
		d_rendered_geometry_factory->create_rendered_polyline_on_sphere(
				polyline_on_sphere,
				d_geom_operation_render_parameters.get_not_in_focus_colour(),
				d_geom_operation_render_parameters.get_line_width_hint());

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

	RenderedGeometry rendered_geom =
		d_rendered_geometry_factory->create_rendered_polygon_on_sphere(
				polygon_on_sphere,
				d_geom_operation_render_parameters.get_not_in_focus_colour(),
				d_geom_operation_render_parameters.get_line_width_hint());

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

		RenderedGeometry rendered_geom =
			d_rendered_geometry_factory->create_rendered_point_on_sphere(
			point_on_sphere,
			d_geom_operation_render_parameters.get_focus_colour(),
			d_geom_operation_render_parameters.get_large_point_size_hint());

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(rendered_geom);
	}
}
