/* $Id$ */

/**
 * \file 
 * Adds points to a geometry as the user clicks a position on the globe.
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
#include <QUndoCommand>

#include "AddPointGeometryOperation.h"

#include "GeometryOperationUndo.h"
#include "GeometryBuilderUndoCommands.h"
#include "QueryProximityThreshold.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryParameters.h"
#include "RenderedGeometryProximity.h"
#include "RenderedGeometryUtils.h"
#include "UndoRedo.h"

#include "canvas-tools/GeometryOperationState.h"

#include "gui/ChooseCanvasToolUndoCommand.h"

#include "maths/PointOnSphere.h"
#include "maths/ProximityCriteria.h"


GPlatesViewOperations::AddPointGeometryOperation::AddPointGeometryOperation(
		GPlatesMaths::GeometryType::Value build_geom_type,
		GeometryBuilder &geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		const QueryProximityThreshold &query_proximity_threshold) :
	d_build_geom_type(build_geom_type),
	d_geometry_builder(geometry_builder),
	d_geometry_operation_state(geometry_operation_state),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_main_rendered_layer_type(main_rendered_layer_type),
	d_canvas_tool_workflows(canvas_tool_workflows),
	d_query_proximity_threshold(query_proximity_threshold)
{
}

void
GPlatesViewOperations::AddPointGeometryOperation::activate()
{
	// Let others know we're the currently activated GeometryOperation.
	d_geometry_operation_state.set_active_geometry_operation(this);

	connect_to_geometry_builder_signals();

	// Create the rendered geometry layers required by the GeometryBuilder state
	// and activate/deactivate appropriate layers.
	create_rendered_geometry_layers();

	// Activate our render layers so they become visible.
	d_lines_layer_ptr->set_active(true);
	d_points_layer_ptr->set_active(true);

	// Fill the rendered layers with RenderedGeometry objects by querying
	// the GeometryBuilder state.
	update_rendered_geometries();
}

void
GPlatesViewOperations::AddPointGeometryOperation::deactivate()
{
	// Let others know there's no currently activated GeometryOperation.
	d_geometry_operation_state.set_no_active_geometry_operation();

	disconnect_from_geometry_builder_signals();

	// Get rid of all render layers even if switching to drag or zoom tool
	// (which normally previously would display the most recent tool's layers).
	// This is because once we are deactivated we won't be able to update the render layers when/if
	// the reconstruction time changes.
	// This means the user won't see this tool's render layers while in the drag or zoom tool.
	d_lines_layer_ptr->set_active(false);
	d_points_layer_ptr->set_active(false);
	d_lines_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();
}

void
GPlatesViewOperations::AddPointGeometryOperation::add_point(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Get number of points in current geometry.
	const int num_geom_points =
		d_geometry_builder.get_num_points_in_current_geometry();

	// First see if the point to be added is too close to the last added point.
	// If it is then don't add it.
	if (num_geom_points)
	{
		GPlatesViewOperations::GeometryBuilder::GeometryIndex geom_index =
			d_geometry_builder.get_current_geometry_index();
		GPlatesMaths::PointOnSphere last_point_added =
			*(d_geometry_builder.get_geometry_point_end(geom_index) - 1);
		if (last_point_added == oriented_pos_on_sphere)
		{
			return;
		}
	}

	// The command that does the actual adding of the point.
	std::auto_ptr<QUndoCommand> add_point_command(
			new GeometryBuilderInsertPointUndoCommand(
					d_geometry_builder,
					num_geom_points,
					oriented_pos_on_sphere));

	// Command wraps add point command with handing canvas tool choice and
	// add point tool activation.
	std::auto_ptr<QUndoCommand> undo_command(
			new GeometryOperationUndoCommand(
					QObject::tr("add point"),
					add_point_command,
					this,
					d_canvas_tool_workflows));

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the vertex is initially inserted.
	UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}

void
GPlatesViewOperations::AddPointGeometryOperation::create_rendered_geometry_layers()
{
	// Create a rendered layer to draw the last point in geometry underneath the lines.
	d_points_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// Create a rendered layer to draw the line segments of polylines and polygons.
	// NOTE: this must be created second to get drawn on top.
	d_lines_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.
}

void
GPlatesViewOperations::AddPointGeometryOperation::connect_to_geometry_builder_signals()
{
	// Connect to the current geometry builder's signals.

	// GeometryBuilder has just finished updating geometry.
	QObject::connect(
			&d_geometry_builder,
			SIGNAL(stopped_updating_geometry()),
			this,
			SLOT(geometry_builder_stopped_updating_geometry()));
}

void
GPlatesViewOperations::AddPointGeometryOperation::disconnect_from_geometry_builder_signals()
{
	// Disconnect all signals from the current geometry builder.
	QObject::disconnect(&d_geometry_builder, 0, this, 0);
}

void
GPlatesViewOperations::AddPointGeometryOperation::geometry_builder_stopped_updating_geometry()
{
	// The geometry builder has just potentially done a group of
	// geometry modifications and is now notifying us that it's finished.

	// Just clear and add all RenderedGeometry objects.
	// This could be optimised, if profiling says so, by listening to the other signals
	// generated by GeometryBuilder instead and only making the minimum changes needed.
	update_rendered_geometries();
}

void
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_geometries()
{
	// Clear all RenderedGeometry objects from the render layers first.
	d_lines_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();

	for (GeometryBuilder::GeometryIndex geom_index = 0;
		geom_index < d_geometry_builder.get_num_geometries();
		++geom_index)
	{
		update_rendered_geometry(geom_index);
	}
}

void
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_geometry(
		GeometryBuilder::GeometryIndex geom_index)
{
	const GPlatesMaths::GeometryType::Value geom_build_type =
		d_geometry_builder.get_geometry_build_type();

	switch (geom_build_type)
	{
	case GPlatesMaths::GeometryType::POINT:
		update_rendered_point_on_sphere(geom_index);
		break;

	case GPlatesMaths::GeometryType::MULTIPOINT:
		update_rendered_multipoint_on_sphere(geom_index);
		break;

	case GPlatesMaths::GeometryType::POLYLINE:
		update_rendered_polyline_on_sphere(geom_index);
		break;

	case GPlatesMaths::GeometryType::POLYGON:
		update_rendered_polygon_on_sphere(geom_index);
		break;

	default:
		// Do nothing.
		break;
	}
}

void
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_point_on_sphere(
		GeometryBuilder::GeometryIndex geom_index)
{
	update_rendered_multipoint_on_sphere(geom_index);
}

void
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_multipoint_on_sphere(
		GeometryBuilder::GeometryIndex geom_index)
{
	GeometryBuilder::point_const_iterator_type builder_geom_begin =
		d_geometry_builder.get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder.get_geometry_point_end(geom_index);

	GeometryBuilder::point_const_iterator_type builder_geom_iter;
	for (builder_geom_iter = builder_geom_begin;
		builder_geom_iter != builder_geom_end;
		++builder_geom_iter)
	{
		const GPlatesMaths::PointOnSphere &point_on_sphere = *builder_geom_iter;

		RenderedGeometry rendered_geom =
				RenderedGeometryFactory::create_rendered_point_on_sphere(
						point_on_sphere,
						GeometryOperationParameters::FOCUS_COLOUR,
						GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(rendered_geom);
	}
}

void
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_polyline_on_sphere(
		GeometryBuilder::GeometryIndex geom_index)
{
	const unsigned int num_points_in_geom =
			d_geometry_builder.get_num_points_in_geometry(geom_index);

	if (num_points_in_geom > 1)
	{
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
				GPlatesMaths::PolylineOnSphere::create_on_heap(
						d_geometry_builder.get_geometry_point_begin(geom_index),
						d_geometry_builder.get_geometry_point_end(geom_index));

		RenderedGeometry polyline_rendered_geom =
				RenderedGeometryFactory::create_rendered_polyline_on_sphere(
						polyline_on_sphere,
						GeometryOperationParameters::FOCUS_COLOUR,
						GeometryOperationParameters::LINE_WIDTH_HINT);

		// Add to the lines layer.
		d_lines_layer_ptr->add_rendered_geometry(polyline_rendered_geom);

		const GPlatesMaths::PointOnSphere &end_point_on_sphere =
			d_geometry_builder.get_geometry_point(geom_index, num_points_in_geom - 1);

		RenderedGeometry end_point_rendered_geom =
				RenderedGeometryFactory::create_rendered_point_on_sphere(
						end_point_on_sphere,
						GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
						GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(end_point_rendered_geom);
	}
	else if (num_points_in_geom == 1)
	{
		// We have one point.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
			d_geometry_builder.get_geometry_point(geom_index, 0);

		RenderedGeometry rendered_geom =
				RenderedGeometryFactory::create_rendered_point_on_sphere(
						point_on_sphere,
						GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
						GeometryOperationParameters::REGULAR_POINT_SIZE_HINT);

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(rendered_geom);
	}
}

void
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_polygon_on_sphere(
		GeometryBuilder::GeometryIndex geom_index)
{
	// First part of polygon looks same as polyline.
	update_rendered_polyline_on_sphere(geom_index);

	// If we have three or more points then last segment of polygon
	// is drawn.
	const unsigned int num_points_in_geom =
			d_geometry_builder.get_num_points_in_geometry(geom_index);

	if (num_points_in_geom > 2)
	{
		const GPlatesMaths::PointOnSphere end_segment[2] =
		{
			d_geometry_builder.get_geometry_point(geom_index, 0),
			d_geometry_builder.get_geometry_point(geom_index, num_points_in_geom - 1)
		};

		// Attempt to create a valid line segment.
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity line_segment_validity;
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> end_segment_polyline_on_sphere =
				GPlatesUtils::create_polyline_on_sphere(
						end_segment, end_segment + 2, line_segment_validity);

		// We need to check for a valid geometry especially since we're creating a single
		// line segment - we could get failure if both points are too close which would result
		// in a thrown exception.
		if (line_segment_validity == GPlatesUtils::GeometryConstruction::VALID)
		{
		RenderedGeometry end_segment_polyline_rendered_geom =
				RenderedGeometryFactory::create_rendered_polyline_on_sphere(
							*end_segment_polyline_on_sphere,
						GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
						GeometryOperationParameters::LINE_WIDTH_HINT);

		// Add to the lines layer.
		d_lines_layer_ptr->add_rendered_geometry(end_segment_polyline_rendered_geom);
		}
	}
}
