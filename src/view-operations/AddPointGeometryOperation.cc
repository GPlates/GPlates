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
#include "GeometryBuilderUndoCommands.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryUtils.h"
#include "UndoRedo.h"
#include "gui/ChooseCanvasTool.h"
#include "maths/PointOnSphere.h"


GPlatesViewOperations::AddPointGeometryOperation::AddPointGeometryOperation(
		GeometryType::Value build_geom_type,
		RenderedGeometryCollection *rendered_geometry_collection,
		RenderedGeometryFactory *rendered_geometry_factory,
		const GeometryOperationRenderParameters &geom_operation_render_parameters,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool) :
d_build_geom_type(build_geom_type),
d_geometry_builder(NULL),
d_rendered_geometry_collection(rendered_geometry_collection),
d_rendered_geometry_factory(rendered_geometry_factory),
d_choose_canvas_tool(&choose_canvas_tool),
d_geom_operation_render_parameters(geom_operation_render_parameters)
{
}

void
GPlatesViewOperations::AddPointGeometryOperation::activate(
		GeometryBuilder *geometry_builder,
		RenderedGeometryCollection::MainLayerType main_layer_type)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	d_main_layer_type = main_layer_type;
	d_geometry_builder = geometry_builder;

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
GPlatesViewOperations::AddPointGeometryOperation::deactivate()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	disconnect_from_geometry_builder_signals();

	// NOTE: we don't deactivate our rendered layers because they still need
	// to be visible even after we've deactivated.  They will remain in existance
	// until activate() is called again on this object.

	// Not using this GeometryBuilder anymore.
	d_geometry_builder = NULL;
}

void
GPlatesViewOperations::AddPointGeometryOperation::add_point(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Get number of points in current geometry.
	const int num_geom_points =
		d_geometry_builder->get_num_points_in_current_geometry();

	// Group two undo commands into one.
	std::auto_ptr<QUndoCommand> undo_command(
		new UndoRedo::GroupUndoCommand(QObject::tr("add point")));

	// Add child undo command for adding a point.
	new GPlatesViewOperations::GeometryBuilderInsertPointUndoCommand(
			d_geometry_builder,
			num_geom_points,
			oriented_pos_on_sphere,
			undo_command.get());

	// Add child undo command for selecting the most recently selected digitise geometry tool.
	// When or if this undo/redo command gets called one of the digitisation tools may not
	// be active so make sure it gets activated so user can see what's being undone/redone.
	new GPlatesGui::ChooseCanvasToolUndoCommand(
			d_choose_canvas_tool,
			&GPlatesGui::ChooseCanvasTool::choose_most_recent_digitise_geometry_tool,
			undo_command.get());

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the point is initially added.
	UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
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

	// Deactivate all rendered geometry layers our the main rendered layer.
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

		RenderedGeometry rendered_geom =
			d_rendered_geometry_factory->create_rendered_point_on_sphere(
					point_on_sphere,
					d_geom_operation_render_parameters.get_focus_colour(),
					d_geom_operation_render_parameters.get_large_point_size_hint());

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

		RenderedGeometry polyline_rendered_geom =
			d_rendered_geometry_factory->create_rendered_polyline_on_sphere(
					polyline_on_sphere,
					d_geom_operation_render_parameters.get_focus_colour(),
					d_geom_operation_render_parameters.get_line_width_hint());

		// Add to the lines layer.
		d_lines_layer_ptr->add_rendered_geometry(polyline_rendered_geom);

		const GPlatesMaths::PointOnSphere &end_point_on_sphere =
			d_geometry_builder->get_geometry_point(geom_index, num_points_in_geom - 1);

		RenderedGeometry end_point_rendered_geom =
			d_rendered_geometry_factory->create_rendered_point_on_sphere(
					end_point_on_sphere,
					d_geom_operation_render_parameters.get_not_in_focus_colour(),
					d_geom_operation_render_parameters.get_large_point_size_hint());

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(end_point_rendered_geom);
	}
	else if (num_points_in_geom == 1)
	{
		// We have one point.
		const GPlatesMaths::PointOnSphere &point_on_sphere =
			d_geometry_builder->get_geometry_point(geom_index, 0);

		RenderedGeometry rendered_geom =
			d_rendered_geometry_factory->create_rendered_point_on_sphere(
					point_on_sphere,
					d_geom_operation_render_parameters.get_not_in_focus_colour(),
					d_geom_operation_render_parameters.get_regular_point_size_hint());

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

		RenderedGeometry end_segment_polyline_rendered_geom =
			d_rendered_geometry_factory->create_rendered_polyline_on_sphere(
					end_segment_polyline_on_sphere,
					d_geom_operation_render_parameters.get_not_in_focus_colour(),
					d_geom_operation_render_parameters.get_line_width_hint());

		// Add to the lines layer.
		d_lines_layer_ptr->add_rendered_geometry(end_segment_polyline_rendered_geom);
	}
}
