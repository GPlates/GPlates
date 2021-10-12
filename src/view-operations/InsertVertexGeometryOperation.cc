/* $Id$ */

/**
 * \file 
 * Inserts a vertex into a geometry when the user clicks on an existing line segment (for
 * polylines and polygons) or anywhere for multipoints - also, for polylines and polygons,
 * will add point at beginning or end (whichever is closest to click point) if click point
 * is not on a line segment.
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

#include <memory>
#include <iterator>
#include <QUndoCommand>

#include "InsertVertexGeometryOperation.h"

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
#include "maths/GreatCircleArc.h"
#include "maths/PointOnSphere.h"
#include "maths/ProximityCriteria.h"
#include "utils/GeometryCreationUtils.h"


GPlatesViewOperations::InsertVertexGeometryOperation::InsertVertexGeometryOperation(
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
d_query_proximity_threshold(&query_proximity_threshold)
{
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::activate(
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
	d_line_segments_layer_ptr->set_active(true);
	d_points_layer_ptr->set_active(true);
	d_highlight_layer_ptr->set_active(true);

	// Fill the rendered layers with RenderedGeometry objects by querying
	// the GeometryBuilder state.
	update_rendered_geometries();
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::deactivate()
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

	disconnect_from_geometry_builder_signals();

	// NOTE: we don't deactivate our rendered layers because they still need
	// to be visible even after we've deactivated.  They will remain in existance
	// until activate() is called again on this object.

	// We will however destroy the highlight rendered geometry layer.
	// This is so the highlights are not visible when we switch to the drag or zoom
	// tool. The highlights layer will be recreated and populated when 'activate()'
	// is called again.
	d_highlight_layer_ptr.reset();

	// Not using this GeometryBuilder anymore.
	d_geometry_builder = NULL;
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::left_click(
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

	// See if mouse position is on, or very near, an existing line segment.
	boost::optional<RenderedGeometryProximityHit> closest_line_hit = test_proximity_to_rendered_geom_layer(
			*d_line_segments_layer_ptr, oriented_pos_on_sphere, closeness_inclusion_threshold);

	if (closest_line_hit)
	{
		const unsigned int line_segment_index = closest_line_hit->d_rendered_geom_index;

		insert_vertex_on_line_segment(
				line_segment_index, oriented_pos_on_sphere, closeness_inclusion_threshold);
	}
	else
	{
		// We are not close enough to any line segments.
		insert_vertex_off_line_segment(oriented_pos_on_sphere);
	}

	// Render the highlight line segments to show user where the next mouse click will
	// insert the next vertex.
	// We do this now in case the mouse doesn't move again for a while (ie, if we get no
	// 'mouse_move' event).
	update_highlight_rendered_layer(oriented_pos_on_sphere, closeness_inclusion_threshold);
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::insert_vertex_on_line_segment(
		const unsigned int line_segment_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// First make sure we are not too close to an existing point.
	// If we are then the user will need to zoom in the view in order to
	// insert the vertex that close.

	// Test closeness to the points in the points rendered geometry layer.
	if (!test_proximity_to_rendered_geom_layer(
			*d_points_layer_ptr,  oriented_pos_on_sphere, closeness_inclusion_threshold))
	{
		// Get the index of the point at the start of the line segment
		GeometryBuilder::PointIndex index_of_start_point = d_line_to_point_mapping[line_segment_index];

		// This can be one past the last point when inserting at end of geometry.
		const GeometryBuilder::PointIndex index_of_point_to_insert_before = index_of_start_point + 1;

		// Instead of inserting a vertex at the mouse position we project the mouse
		// position onto the line segment and insert there.
		// This is useful if the user wants to insert directly on the line segment even
		// though the mouse position might be off the line segment by a pixel or two.
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_to_insert =
				project_point_onto_line_segment(index_of_start_point, oriented_pos_on_sphere);

		insert_vertex(index_of_point_to_insert_before, *point_to_insert);
	}
}

GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesViewOperations::InsertVertexGeometryOperation::project_point_onto_line_segment(
		const GeometryBuilder::PointIndex start_point_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// We currently only support one internal geometry so set geom index to zero.
	const GeometryBuilder::GeometryIndex geom_index = 0;

	const GPlatesMaths::PointOnSphere *line_segment_start;
	const GPlatesMaths::PointOnSphere *line_segment_end;

	// Line segment could be the last segment in a polygon in which case the segment start point
	// is the last point in polygon and the segment end point is first point in polygon.
	// Note for polylines this can't happen so this only applies to polygons.
	if (start_point_index == d_geometry_builder->get_num_points_in_geometry(geom_index) - 1)
	{
		line_segment_start = &d_geometry_builder->get_geometry_point(geom_index, start_point_index);
		line_segment_end = &d_geometry_builder->get_geometry_point(geom_index, 0);
	}
	else
	{
		line_segment_start = &d_geometry_builder->get_geometry_point(geom_index, start_point_index);
		line_segment_end = &d_geometry_builder->get_geometry_point(geom_index, start_point_index + 1);
	}


	const GPlatesMaths::GreatCircleArc line_segment = GPlatesMaths::GreatCircleArc::create(
			*line_segment_start, *line_segment_end);

	return line_segment.get_closest_point(oriented_pos_on_sphere);
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::insert_vertex_off_line_segment(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	// We are not close enough to any line segments.
	// NOTE: this also means we are not close enough to any of the points either so
	// there's no danger of inserting a vertex on top of an existing one so we don't
	// need to check for this.

	// We currently only support one internal geometry so set geom index to zero.
	const GeometryBuilder::GeometryIndex geom_index = 0;

	// Number of points in the geometry (is zero if no geometry).
	const unsigned int num_points_in_geom = (d_geometry_builder->get_num_geometries() > 0)
			? d_geometry_builder->get_num_points_in_geometry(geom_index) : 0;

	// Only allow insertion of a vertex if we already have at least one vertex.
	// This is to provide symmetry with the delete vertex tool which won't allow you
	// to delete the last vertex in a geometry.
	if (num_points_in_geom == 0)
	{
		return;
	}

	if (num_points_in_geom == 1)
	{
		// Insert vertex at end of geometry for all geometry types.
		insert_vertex(num_points_in_geom, oriented_pos_on_sphere);
	}
	else if (num_points_in_geom > 1)
	{
		const GeometryType::Value geom_build_type = d_geometry_builder->get_geometry_build_type();

		if (geom_build_type == GeometryType::POLYLINE ||
			geom_build_type == GeometryType::POLYGON)
		{
			// Get index of closest point to mouse position.
			const GeometryBuilder::PointIndex closest_geom_point_index = *get_closest_geometry_point_to(
					oriented_pos_on_sphere);

			// Only allow extending polyline/polygon if one of the end points is closer to
			// the mouse position than all other points in the geometry.
			// Insert vertex at beginning or end depending on which is closest to mouse position.
			if (closest_geom_point_index == 0)
			{
				insert_vertex(0, oriented_pos_on_sphere);
			}
			else if (closest_geom_point_index == num_points_in_geom - 1)
			{
				insert_vertex(num_points_in_geom, oriented_pos_on_sphere);
			}
		}
		else // GeometryType::POINT or GeometryType::MULTIPOINT
		{
			// Insert vertex at end of POINT or MULTIPOINT.
			insert_vertex(num_points_in_geom, oriented_pos_on_sphere);
		}
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::mouse_move(
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

	// Render the highlight line segments to show user where the vertex will get inserted.
	update_highlight_rendered_layer(oriented_pos_on_sphere, closeness_inclusion_threshold);
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::update_highlight_rendered_layer(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// First clear any highlight rendered geometries.
	d_highlight_layer_ptr->clear_rendered_geometries();

	//
	// If clicked point is on a line segment then highlight that line segment.
	//
	// Otherwise:
	// If the geometry type we're trying to build (not necessarily same as what
	//   we've actually got) is a *polyline* then highlight a new temporary line segment from
	//   clicked point to the nearest end of the entire polyline.
	// If the geometry type we're trying to build (not necessarily same as what
	//   we've actually got) is a *polygon* then highlight two new temporary line segments from
	//   clicked point to both ends of the polygon.
	//

	// See if mouse position is on, or very near, an existing line segment.
	boost::optional<RenderedGeometryProximityHit> closest_line_hit = test_proximity_to_rendered_geom_layer(
			*d_line_segments_layer_ptr, oriented_pos_on_sphere, closeness_inclusion_threshold);
	if (closest_line_hit)
	{
		const unsigned int line_segment_index =
				closest_line_hit->d_rendered_geom_index;

		add_rendered_highlight_on_line_segment(
				line_segment_index, oriented_pos_on_sphere, closeness_inclusion_threshold);
	}
	else
	{
		// We are not close enough to any line segments.
		add_rendered_highlight_off_line_segment(oriented_pos_on_sphere);
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::add_rendered_highlight_on_line_segment(
		const unsigned int line_segment_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// First make sure we are not too close to an existing point.
	// If we are then the user will need to zoom in the view in order to
	// insert the vertex that close.

	// Test closeness to the points in the points rendered geometry layer.
	if (!test_proximity_to_rendered_geom_layer(
			*d_points_layer_ptr, oriented_pos_on_sphere, closeness_inclusion_threshold))
	{
		add_rendered_highlight_line_segment(line_segment_index);
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::add_rendered_highlight_off_line_segment(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	//
	// Mouse position is not on an existing line segment.
	//

	// We are not close enough to any line segments.
	// NOTE: this also means we are not close enough to any of the points either so
	// there's no danger of inserting a vertex on top of an existing one so we don't
	// need to check for this.

	const GeometryType::Value geom_build_type = d_geometry_builder->get_geometry_build_type();

	// If geometry we're trying to build is not a polyline or polygon then no
	// highlighting is needed. Highlighting is only done on line segments -
	// which only polylines and polygons have.
	if (geom_build_type != GeometryType::POLYLINE &&
		geom_build_type != GeometryType::POLYGON)
	{
		return;
	}

	// We currently only support one internal geometry so set geom index to zero.
	const GeometryBuilder::GeometryIndex geom_index = 0;

	// Number of points in the geometry (is zero if no geometry).
	const unsigned int num_points_in_geom = (d_geometry_builder->get_num_geometries() > 0)
			? d_geometry_builder->get_num_points_in_geometry(geom_index) : 0;

	// Only allow insertion of a vertex if we already have at least one vertex.
	// This is to provide symmetry with the delete vertex tool which won't allow you
	// to delete the last vertex in a geometry.
	if (num_points_in_geom == 0)
	{
		return;
	}

	if (num_points_in_geom == 1)
	{
		// Only one point in polyline/polygon so far so add a single highlight line segment
		// between that point and the mouse position.
		add_rendered_highlight_line_segment(
				d_geometry_builder->get_geometry_point(geom_index, 0/*point index*/),
				oriented_pos_on_sphere);
	}
	else if (num_points_in_geom > 1)
	{
		// Get index of closest point to mouse position.
		const GeometryBuilder::PointIndex closest_geom_point_index = *get_closest_geometry_point_to(
				oriented_pos_on_sphere);

		// Only allow extending polyline/polygon if one of the end points is closer to
		// the mouse position than all other points in the geometry.
		if (closest_geom_point_index == 0 ||
			closest_geom_point_index == num_points_in_geom - 1)
		{
			// Start and end points of polyline/polygon.
			const GPlatesMaths::PointOnSphere &first_point =
					d_geometry_builder->get_geometry_point(geom_index, 0/*point index*/);
			const GPlatesMaths::PointOnSphere &last_point =
					d_geometry_builder->get_geometry_point(geom_index, num_points_in_geom - 1);

			if (geom_build_type == GeometryType::POLYLINE)
			{
				// Add a highlight line segment from mouse position to closest end of polyline.
				add_rendered_highlight_line_segment(
						(closest_geom_point_index == 0) ? first_point : last_point,
						oriented_pos_on_sphere);
			}
			else // GeometryType::POLYGON
			{
				// Add two highlight line segments from mouse position to both ends of polygon.
				add_rendered_highlight_line_segment(first_point, oriented_pos_on_sphere);
				add_rendered_highlight_line_segment(last_point, oriented_pos_on_sphere);
			}
		}
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::add_rendered_highlight_line_segment(
		const unsigned int highlight_line_segment_index)
{
	// Note: we don't currently support multiple internal geometries so set
	// geometry index to zero. We also assume there is a geometry - we wouldn't have
	// been called if that was not the case though.
	const GeometryBuilder::GeometryIndex geom_index = 0;

	// Line segment could be the last segment in a polygon in which case the segment start point
	// is the last point in polygon and the segment end point is first point in polygon.
	// Note for polylines this can't happen so this only applies to polygons.
	unsigned int highlight_start_point_index = d_line_to_point_mapping[highlight_line_segment_index];
	if (highlight_start_point_index == d_geometry_builder->get_num_points_in_geometry(geom_index) - 1)
	{
		const GPlatesMaths::PointOnSphere &start_point =
				d_geometry_builder->get_geometry_point(geom_index, highlight_start_point_index);
		const GPlatesMaths::PointOnSphere &end_point =
				d_geometry_builder->get_geometry_point(geom_index, 0);

		add_rendered_highlight_line_segment(start_point, end_point);
	}
	else
	{
		// Get point at start of line segment.
		GeometryBuilder::point_const_iterator_type line_segment_begin =
				d_geometry_builder->get_geometry_point_begin(geom_index);
		std::advance(line_segment_begin, highlight_start_point_index);

		GeometryBuilder::point_const_iterator_type line_segment_end = line_segment_begin;
		std::advance(line_segment_end, 2); // Line segment contains two points.

		add_rendered_highlight_line_segment(line_segment_begin, line_segment_end);
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::add_rendered_highlight_line_segment(
		const GPlatesMaths::PointOnSphere &start_point,
		const GPlatesMaths::PointOnSphere &end_point)
{
	// Copy the start and end points of the polygon so we can create a line segment from them.
	GPlatesMaths::PointOnSphere start_end_points[2] =
	{
		start_point,
		end_point
	};

	add_rendered_highlight_line_segment(start_end_points, start_end_points + 2);
}

template <typename ForwardIterPointOnSphere>
void
GPlatesViewOperations::InsertVertexGeometryOperation::add_rendered_highlight_line_segment(
		ForwardIterPointOnSphere begin_point_on_sphere,
		ForwardIterPointOnSphere end_point_on_sphere)
{
	// Attempt to create a valid line segment.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity line_segment_validity;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> line_segment_on_sphere =
			GPlatesUtils::create_polyline_on_sphere(
					begin_point_on_sphere, end_point_on_sphere, line_segment_validity);

	// We need to check for a valid geometry especially since we're creating a single
	// line segment - we could get failure if both points are too close which would result
	// in a thrown exception.
	if (line_segment_validity == GPlatesUtils::GeometryConstruction::VALID)
	{
		RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_polyline_on_sphere(
					*line_segment_on_sphere,
					GeometryOperationParameters::HIGHLIGHT_COLOUR,
					GeometryOperationParameters::HIGHLIGHT_LINE_WIDTH_HINT);

		// Add to the highlight layer.
		d_highlight_layer_ptr->add_rendered_geometry(rendered_geom);
	}
}

#if 0
boost::optional<GPlatesViewOperations::RenderedGeometryProximityHit>
GPlatesViewOperations::InsertVertexGeometryOperation::test_proximity_to_rendered_geom_layer(
		const RenderedGeometryLayer &rendered_geom_layer,
		const GPlatesMaths::PointOnSphere &clicked_pos_on_sphere,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	const double closeness_inclusion_threshold =
		d_query_proximity_threshold->current_proximity_inclusion_threshold(
				clicked_pos_on_sphere);

	GPlatesMaths::ProximityCriteria proximity_criteria(
			oriented_pos_on_sphere,
			closeness_inclusion_threshold);

	sorted_rendered_geometry_proximity_hits_type sorted_hits;
	if (!test_proximity(sorted_hits, rendered_geom_layer, proximity_criteria))
	{
		return boost::none;
	}

	// Only interested in the closest line segment in the layer.
	const RenderedGeometryProximityHit &closest_hit = sorted_hits.front();

	return closest_hit;
}
#endif

boost::optional<GPlatesViewOperations::RenderedGeometryProximityHit>
GPlatesViewOperations::InsertVertexGeometryOperation::test_proximity_to_rendered_geom_layer(
	const RenderedGeometryLayer &rendered_geom_layer,
	const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
	const double &closeness_inclusion_threshold)
{
	GPlatesMaths::ProximityCriteria proximity_criteria(
		oriented_pos_on_sphere,
		closeness_inclusion_threshold);

	sorted_rendered_geometry_proximity_hits_type sorted_hits;
	if (!test_proximity(sorted_hits, proximity_criteria, rendered_geom_layer))
	{
		return boost::none;
	}

	// Only interested in the closest line segment in the layer.
	const RenderedGeometryProximityHit &closest_hit = sorted_hits.front();

	return closest_hit;
}

boost::optional<const GPlatesViewOperations::GeometryBuilder::PointIndex>
GPlatesViewOperations::InsertVertexGeometryOperation::get_closest_geometry_point_to(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	if (d_geometry_builder->get_num_geometries() == 0)
	{
		return boost::none;
	}

	// We currently only support one internal geometry so set geom index to zero.
	const GeometryBuilder::GeometryIndex geom_index = 0;

	const GeometryBuilder::PointIndex num_points_in_geom =
			d_geometry_builder->get_num_points_in_geometry(geom_index);

	// Closeness varies from -1 for antipodal points to 1 for coincident points.
	GPlatesMaths::real_t max_closeness(-1);
	GeometryBuilder::PointIndex closest_pos_on_sphere_index = 0;

	GeometryBuilder::point_const_iterator_type builder_geom_iter;
	GeometryBuilder::PointIndex point_on_sphere_index;
	for (point_on_sphere_index = 0;
		point_on_sphere_index < num_points_in_geom;
		++point_on_sphere_index)
	{
		const GPlatesMaths::PointOnSphere &point_on_sphere = d_geometry_builder->get_geometry_point(
				geom_index, point_on_sphere_index);

		GPlatesMaths::real_t closeness = calculate_closeness(point_on_sphere, oriented_pos_on_sphere);

		if (closeness.dval() > max_closeness.dval())
		{
			max_closeness = closeness;
			closest_pos_on_sphere_index = point_on_sphere_index;
		}
	}

	return closest_pos_on_sphere_index;
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::create_rendered_geometry_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layer to draw the line segments of polylines and polygons.
	d_line_segments_layer_ptr =
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
	d_highlight_layer_ptr =
		d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
				d_main_layer_type);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::connect_to_geometry_builder_signals()
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
GPlatesViewOperations::InsertVertexGeometryOperation::disconnect_from_geometry_builder_signals()
{
	// Disconnect all signals from the current geometry builder.
	QObject::disconnect(d_geometry_builder, 0, this, 0);
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::geometry_builder_stopped_updating_geometry()
{
	// The geometry builder has just potentially done a group of
	// geometry modifications and is now notifying us that it's finished.

	// Just clear and add all RenderedGeometry objects.
	// This could be optimised, if profiling says so, by listening to the other signals
	// generated by GeometryBuilder instead and only making the minimum changes needed.
	update_rendered_geometries();
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::insert_vertex(
		const GeometryBuilder::PointIndex insert_vertex_index,
		const GPlatesMaths::PointOnSphere &insert_pos_on_sphere)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// The command that does the actual inserting of vertex.
	std::auto_ptr<QUndoCommand> insert_vertex_command(
			new GeometryBuilderInsertPointUndoCommand(
					d_geometry_builder,
					insert_vertex_index,
					insert_pos_on_sphere));

	// Command wraps insert vertex command with handing canvas tool choice and
	// insert vertex tool activation.
	std::auto_ptr<QUndoCommand> undo_command(
			new GeometryOperationUndoCommand(
					QObject::tr("insert vertex"),
					insert_vertex_command,
					this,
					d_geometry_operation_target,
					d_main_layer_type,
					d_choose_canvas_tool,
					&GPlatesGui::ChooseCanvasTool::choose_insert_vertex_tool));

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the vertex is initially inserted.
	UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::update_rendered_geometries()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	RenderedGeometryCollection::UpdateGuard update_guard;

	// Clear all RenderedGeometry objects from the render layers first.
	d_line_segments_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();
	d_highlight_layer_ptr->clear_rendered_geometries();

	// Iterate through the internal geometries (currently only one is supported).
	for (GeometryBuilder::GeometryIndex geom_index = 0;
		geom_index < d_geometry_builder->get_num_geometries();
		++geom_index)
	{
		update_rendered_geometry(geom_index);
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::update_rendered_geometry(
		GeometryBuilder::GeometryIndex geom_index)
{
	// All types of geometry have the points drawn the same.
	add_rendered_points(geom_index);

	const GeometryType::Value actual_geom_type =
		d_geometry_builder->get_actual_type_of_geometry(geom_index);

	if (actual_geom_type == GeometryType::POLYLINE ||
		actual_geom_type == GeometryType::POLYGON)
	{
		add_rendered_lines(geom_index, actual_geom_type);
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::add_rendered_lines(
		GeometryBuilder::GeometryIndex geom_index,
		const GeometryType::Value actual_geom_type)
{
	const unsigned int num_points_in_geom = d_geometry_builder->get_num_points_in_geometry(geom_index);
	d_line_to_point_mapping.clear();

	if (num_points_in_geom < 2)
	{
		// We don't have even a single line segment so nothing to do.
		return;
	}

	// Get start and end of point sequence in current geometry.
	GeometryBuilder::point_const_iterator_type builder_geom_begin =
		d_geometry_builder->get_geometry_point_begin(geom_index);

	// Create a separate rendered geometry for each polyline line segment.
	// This is so we can test proximity to individual line segments.
	GeometryBuilder::point_const_iterator_type first_line_segment_start_point = builder_geom_begin;
	GeometryBuilder::point_const_iterator_type last_line_segment_end_point = builder_geom_begin;
	std::advance(last_line_segment_end_point, num_points_in_geom - 1);

	GeometryBuilder::point_const_iterator_type line_segment_iter;
	for (line_segment_iter = first_line_segment_start_point;
		line_segment_iter != last_line_segment_end_point;
		++line_segment_iter)
	{
		GeometryBuilder::point_const_iterator_type line_segment_begin = line_segment_iter;
		GeometryBuilder::point_const_iterator_type line_segment_end = line_segment_begin;
		std::advance(line_segment_end, 2); // Line segment contains two points.

		// Attempt to create a valid line segment.
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity line_segment_validity;
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> line_segment_on_sphere =
				GPlatesUtils::create_polyline_on_sphere(
						line_segment_begin, line_segment_end, line_segment_validity);

		// We need to check for a valid geometry especially since we're creating a single
		// line segment - we could get failure if both points are too close which would result
		// in a thrown exception.
		if (line_segment_validity == GPlatesUtils::GeometryConstruction::VALID)
		{
			RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_polyline_on_sphere(
						*line_segment_on_sphere,
						GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
						GeometryOperationParameters::LINE_WIDTH_HINT);

			// Add to the lines layer.
			d_line_segments_layer_ptr->add_rendered_geometry(rendered_geom);

			// Remember the index of the starting point of this line.
			d_line_to_point_mapping.push_back(line_segment_iter - first_line_segment_start_point);
		}
	}

	// If actual geometry type is a polygon then also add the line segment between
	// start and end vertex.
	if (actual_geom_type == GeometryType::POLYGON)
	{
		// Copy the start and end points of the polygon so we can create a line segment from them.
		GPlatesMaths::PointOnSphere start_end_points[2] =
		{
			*first_line_segment_start_point,
			*last_line_segment_end_point
		};

		// Attempt to create a valid line segment.
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity line_segment_validity;
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> line_segment_on_sphere =
				GPlatesUtils::create_polyline_on_sphere(
						start_end_points, start_end_points + 2, line_segment_validity);

		// We need to check for a valid geometry especially since we're creating a single
		// line segment - we could get failure if both points are too close which would result
		// in a thrown exception.
		if (line_segment_validity == GPlatesUtils::GeometryConstruction::VALID)
		{
			RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_polyline_on_sphere(
						*line_segment_on_sphere,
						GeometryOperationParameters::NOT_IN_FOCUS_COLOUR,
						GeometryOperationParameters::LINE_WIDTH_HINT);

			// Add to the lines layer.
			d_line_segments_layer_ptr->add_rendered_geometry(rendered_geom);

			// Remember the index of the starting point of this line.
			d_line_to_point_mapping.push_back(num_points_in_geom - 1);
		}
	}
}

void
GPlatesViewOperations::InsertVertexGeometryOperation::add_rendered_points(
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

		RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_point_on_sphere(
			point_on_sphere,
			GeometryOperationParameters::FOCUS_COLOUR,
			GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(rendered_geom);
	}
}
