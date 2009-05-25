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

#include "ActiveGeometryOperation.h"
#include "GeometryOperationUndo.h"
#include "GeometryBuilderUndoCommands.h"
#include "QueryProximityThreshold.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryParameters.h"
#include "RenderedGeometryProximity.h"
#include "RenderedGeometryUtils.h"
#include "UndoRedo.h"
#include "gui/ChooseCanvasTool.h"
#include "maths/PointOnSphere.h"
#include "maths/ProximityCriteria.h"


GPlatesViewOperations::AddPointGeometryOperation::AddPointGeometryOperation(
		GeometryType::Value build_geom_type,
		GeometryOperationTarget &geometry_operation_target,
		ActiveGeometryOperation &active_geometry_operation,
		RenderedGeometryCollection *rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const QueryProximityThreshold &query_proximity_threshold) :
d_build_geom_type(build_geom_type),
d_geometry_operation_target(&geometry_operation_target),
d_active_geometry_operation(&active_geometry_operation),
d_geometry_builder(NULL),
d_rendered_geometry_collection(rendered_geometry_collection),
d_choose_canvas_tool(&choose_canvas_tool),
d_query_proximity_threshold(&query_proximity_threshold)
{
}

void
GPlatesViewOperations::AddPointGeometryOperation::activate(
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

	d_main_layer_type = main_layer_type;
	d_geometry_builder = geometry_builder;

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

	// Fill the rendered layers with RenderedGeometry objects by querying
	// the GeometryBuilder state.
	update_rendered_geometries();
}

void
GPlatesViewOperations::AddPointGeometryOperation::deactivate()
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}

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

	// Not using this GeometryBuilder anymore.
	d_geometry_builder = NULL;
}

void
GPlatesViewOperations::AddPointGeometryOperation::add_point(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Do nothing if NULL geometry builder.
	if (d_geometry_builder == NULL)
	{
		return;
	}

	// First see if the point to be added is too close to an existing point.
	// If it is then don't add it.
	if (too_close_to_existing_points(oriented_pos_on_sphere,closeness_inclusion_threshold))
	{
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Get number of points in current geometry.
	const int num_geom_points =
		d_geometry_builder->get_num_points_in_current_geometry();

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
					d_geometry_operation_target,
					d_main_layer_type,
					d_choose_canvas_tool,
					&GPlatesGui::ChooseCanvasTool::choose_most_recent_digitise_geometry_tool));

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the vertex is initially inserted.
	UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}

bool
GPlatesViewOperations::AddPointGeometryOperation::too_close_to_existing_points(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// We currently only support one internal geometry so set geom index to zero.
	const GeometryBuilder::GeometryIndex geom_index = 0;

	// Number of points in the geometry (is zero if no geometry).
	const unsigned int num_points_in_geom = (d_geometry_builder->get_num_geometries() > 0)
			? d_geometry_builder->get_num_points_in_geometry(geom_index) : 0;

	if (num_points_in_geom == 0)
	{
		return false;
	}

	GPlatesMaths::ProximityCriteria proximity_criteria(
			oriented_pos_on_sphere,
			closeness_inclusion_threshold);

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

		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type hit =
				point_on_sphere.test_proximity(proximity_criteria);

		// If we got a hit then we're too close to an existing point.
		if (hit)
		{
			return true;
		}
	}

	// Not too close to any existing points.
	return false;
}

void
GPlatesViewOperations::AddPointGeometryOperation::create_rendered_geometry_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layer to draw the last point in geometry underneath the lines.
	d_points_layer_ptr =
		d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
				d_main_layer_type);

	// Create a rendered layer to draw the line segments of polylines and polygons.
	// NOTE: this must be created second to get drawn on top.
	d_lines_layer_ptr =
		d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
				d_main_layer_type);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.
}

void
GPlatesViewOperations::AddPointGeometryOperation::connect_to_geometry_builder_signals()
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
GPlatesViewOperations::AddPointGeometryOperation::disconnect_from_geometry_builder_signals()
{
	// Disconnect all signals from the current geometry builder.
	QObject::disconnect(d_geometry_builder, 0, this, 0);
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
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

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
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_geometry(
		GeometryBuilder::GeometryIndex geom_index)
{
	const GeometryType::Value geom_build_type =
		d_geometry_builder->get_geometry_build_type();

	switch (geom_build_type)
	{
	case GeometryType::POINT:
		update_rendered_point_on_sphere(geom_index);
		break;

	case GeometryType::MULTIPOINT:
		update_rendered_multipoint_on_sphere(geom_index);
		break;

	case GeometryType::POLYLINE:
		update_rendered_polyline_on_sphere(geom_index);
		break;

	case GeometryType::POLYGON:
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
GPlatesViewOperations::AddPointGeometryOperation::update_rendered_polyline_on_sphere(
		GeometryBuilder::GeometryIndex geom_index)
{
	const unsigned int num_points_in_geom =
			d_geometry_builder->get_num_points_in_geometry(geom_index);

	if (num_points_in_geom > 1)
	{
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
				GPlatesMaths::PolylineOnSphere::create_on_heap(
						d_geometry_builder->get_geometry_point_begin(geom_index),
						d_geometry_builder->get_geometry_point_end(geom_index));

		RenderedGeometry polyline_rendered_geom = create_rendered_polyline_on_sphere(
					polyline_on_sphere,
					GeometryOperationParameters::FOCUS_COLOUR,
					GeometryOperationParameters::LINE_WIDTH_HINT);

		// Add to the lines layer.
		d_lines_layer_ptr->add_rendered_geometry(polyline_rendered_geom);

		const GPlatesMaths::PointOnSphere &end_point_on_sphere =
			d_geometry_builder->get_geometry_point(geom_index, num_points_in_geom - 1);

		RenderedGeometry end_point_rendered_geom = create_rendered_point_on_sphere(
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
			d_geometry_builder->get_geometry_point(geom_index, 0);

		RenderedGeometry rendered_geom = create_rendered_point_on_sphere(
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
			d_geometry_builder->get_num_points_in_geometry(geom_index);

	if (num_points_in_geom > 2)
	{
		const GPlatesMaths::PointOnSphere end_segment[2] =
		{
			d_geometry_builder->get_geometry_point(geom_index, 0),
			d_geometry_builder->get_geometry_point(geom_index, num_points_in_geom - 1)
		};

		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type end_segment_polyline_on_sphere =
				GPlatesMaths::PolylineOnSphere::create_on_heap(end_segment, end_segment + 2);

		RenderedGeometry end_segment_polyline_rendered_geom = create_rendered_polyline_on_sphere(
					end_segment_polyline_on_sphere,
					GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
					GeometryOperationParameters::LINE_WIDTH_HINT);

		// Add to the lines layer.
		d_lines_layer_ptr->add_rendered_geometry(end_segment_polyline_rendered_geom);
	}
}
