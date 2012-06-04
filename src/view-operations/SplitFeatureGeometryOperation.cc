/* $Id$ */

/**
 * \file 
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

#include "SplitFeatureGeometryOperation.h"

#include "GeometryBuilderUndoCommands.h"
#include "GeometryOperationUndo.h"
#include "RenderedGeometryProximity.h"
#include "QueryProximityThreshold.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryLayerVisitor.h"
#include "RenderedGeometryParameters.h"
#include "RenderedGeometryUtils.h"
#include "UndoRedo.h"
#include "SplitFeatureUndoCommand.h"

#include "canvas-tools/GeometryOperationState.h"

#include "gui/ChooseCanvasToolUndoCommand.h"

#include "maths/GreatCircleArc.h"
#include "maths/PointOnSphere.h"
#include "maths/ProximityCriteria.h"

#include "utils/GeometryCreationUtils.h"


GPlatesViewOperations::SplitFeatureGeometryOperation::SplitFeatureGeometryOperation(
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesModel::ModelInterface model_interface,	
		GeometryBuilder &geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		const QueryProximityThreshold &query_proximity_threshold) :
	d_feature_focus(feature_focus),
	d_model_interface(model_interface),
	d_geometry_builder(geometry_builder),
	d_geometry_operation_state(geometry_operation_state),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_main_rendered_layer_type(main_rendered_layer_type),
	d_canvas_tool_workflows(canvas_tool_workflows),
	d_query_proximity_threshold(query_proximity_threshold)
{ }

void
GPlatesViewOperations::SplitFeatureGeometryOperation::activate()
{
	// Let others know we're the currently activated GeometryOperation.
	d_geometry_operation_state.set_active_geometry_operation(this);

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
GPlatesViewOperations::SplitFeatureGeometryOperation::deactivate()
{
	// Let others know there's no currently activated GeometryOperation.
	d_geometry_operation_state.set_no_active_geometry_operation();

	disconnect_from_geometry_builder_signals();

	// Get rid of all render layers, not just the highlighting, even if switching to drag or zoom tool
	// (which normally previously would display the most recent tool's layers).
	// This is because once we are deactivated we won't be able to update the render layers when/if
	// the reconstruction time changes.
	// This means the user won't see this tool's render layers while in the drag or zoom tool.
	d_line_segments_layer_ptr->set_active(false);
	d_points_layer_ptr->set_active(false);
	d_highlight_layer_ptr->set_active(false);
	d_line_segments_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();
	d_highlight_layer_ptr->clear_rendered_geometries();
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::left_click(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// See if mouse position is on, or very near, an existing line segment.
	boost::optional<RenderedGeometryProximityHit> closest_line_hit = test_proximity_to_rendered_geom_layer(
			*d_line_segments_layer_ptr, oriented_pos_on_sphere, closeness_inclusion_threshold);

	if (!closest_line_hit)
	{
		// We are not close enough to any line segments so return early.
		return;
	}

	const unsigned int line_segment_index = closest_line_hit->d_rendered_geom_index;

	split_feature(
			line_segment_index, oriented_pos_on_sphere, closeness_inclusion_threshold);

#if 0 //dont't do this. this will disable undo

	// Force a reconstruction because the geometry of the deleted feature is
	// lingering around like a ghost.
	d_view_state->get_reconstruct().reconstruct();

#endif

	// Render the highlight line segments to show user where the next mouse click split the feature geometry.
	// We do this now in case the mouse doesn't move again for a while (ie, if we get no 'mouse_move' event).
	update_highlight_rendered_layer(oriented_pos_on_sphere, closeness_inclusion_threshold);
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::split_feature(
		const unsigned int line_segment_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Test closeness to the points in the points rendered geometry layer.
	boost::optional<GPlatesViewOperations::RenderedGeometryProximityHit> hit= 
		test_proximity_to_rendered_geom_layer(
				*d_points_layer_ptr,  oriented_pos_on_sphere, closeness_inclusion_threshold);
	if (!hit)
	{
		// Get the index of the point at the start of the line segment
		GeometryBuilder::PointIndex index_of_start_point = 
			d_line_to_point_mapping[line_segment_index];

		// This can be one past the last point when inserting at end of geometry.
		const GeometryBuilder::PointIndex index_of_point_to_insert_before =
			index_of_start_point + 1;
		
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_to_insert =
				project_point_onto_line_segment(index_of_start_point, oriented_pos_on_sphere);

		split_feature(index_of_point_to_insert_before, *point_to_insert);
	}
	else
	{
		GeometryBuilder::PointIndex index_of_point = 
			*get_closest_geometry_point_to(oriented_pos_on_sphere);
		split_feature(index_of_point, boost::none);
	}
}

GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesViewOperations::SplitFeatureGeometryOperation::project_point_onto_line_segment(
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
	if (start_point_index == d_geometry_builder.get_num_points_in_geometry(geom_index) - 1)
	{
		line_segment_start = &d_geometry_builder.get_geometry_point(geom_index, start_point_index);
		line_segment_end = &d_geometry_builder.get_geometry_point(geom_index, 0);
	}
	else
	{
		line_segment_start = &d_geometry_builder.get_geometry_point(geom_index, start_point_index);
		line_segment_end = &d_geometry_builder.get_geometry_point(geom_index, start_point_index + 1);
	}


	const GPlatesMaths::GreatCircleArc line_segment = GPlatesMaths::GreatCircleArc::create(
			*line_segment_start, *line_segment_end);

	return line_segment.get_closest_point(oriented_pos_on_sphere);
}


void
GPlatesViewOperations::SplitFeatureGeometryOperation::mouse_move(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Render the highlight line segments to show user where the vertex will get inserted.
	update_highlight_rendered_layer(oriented_pos_on_sphere, closeness_inclusion_threshold);
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::update_highlight_rendered_layer(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// First clear any highlight rendered geometries.
	d_highlight_layer_ptr->clear_rendered_geometries();

	//
	// If clicked point is on a line segment then highlight that line segment.
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
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::add_rendered_highlight_on_line_segment(
		const unsigned int line_segment_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
	// Avoid highlighting line segment if too close to an existing point.
	// This is to discourage the user from splitting a feature near an existing point -
	// the user can still split the feature there though.

	// Test closeness to the points in the points rendered geometry layer.
	if (!test_proximity_to_rendered_geom_layer(
			*d_points_layer_ptr, oriented_pos_on_sphere, closeness_inclusion_threshold))
	{
		add_rendered_highlight_line_segment(line_segment_index);
	}
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::add_rendered_highlight_line_segment(
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
	if (highlight_start_point_index == d_geometry_builder.get_num_points_in_geometry(geom_index) - 1)
	{
		const GPlatesMaths::PointOnSphere &start_point =
				d_geometry_builder.get_geometry_point(geom_index, highlight_start_point_index);
		const GPlatesMaths::PointOnSphere &end_point =
				d_geometry_builder.get_geometry_point(geom_index, 0);

		add_rendered_highlight_line_segment(start_point, end_point);
	}
	else
	{
		// Get point at start of line segment.
		GeometryBuilder::point_const_iterator_type line_segment_begin =
				d_geometry_builder.get_geometry_point_begin(geom_index);
		std::advance(line_segment_begin, highlight_start_point_index);

		GeometryBuilder::point_const_iterator_type line_segment_end = line_segment_begin;
		std::advance(line_segment_end, 2); // Line segment contains two points.

		add_rendered_highlight_line_segment(line_segment_begin, line_segment_end);
	}
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::add_rendered_highlight_line_segment(
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
GPlatesViewOperations::SplitFeatureGeometryOperation::add_rendered_highlight_line_segment(
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

boost::optional<GPlatesViewOperations::RenderedGeometryProximityHit>
GPlatesViewOperations::SplitFeatureGeometryOperation::test_proximity_to_rendered_geom_layer(
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
GPlatesViewOperations::SplitFeatureGeometryOperation::get_closest_geometry_point_to(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
	if (d_geometry_builder.get_num_geometries() == 0)
	{
		return boost::none;
	}

	// We currently only support one internal geometry so set geom index to zero.
	const GeometryBuilder::GeometryIndex geom_index = 0;

	const GeometryBuilder::PointIndex num_points_in_geom =
			d_geometry_builder.get_num_points_in_geometry(geom_index);

	// Closeness varies from -1 for antipodal points to 1 for coincident points.
	GPlatesMaths::real_t max_closeness(-1);
	GeometryBuilder::PointIndex closest_pos_on_sphere_index = 0;

	GeometryBuilder::point_const_iterator_type builder_geom_iter;
	GeometryBuilder::PointIndex point_on_sphere_index;
	for (point_on_sphere_index = 0;
		point_on_sphere_index < num_points_in_geom;
		++point_on_sphere_index)
	{
		const GPlatesMaths::PointOnSphere &point_on_sphere = d_geometry_builder.get_geometry_point(
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
GPlatesViewOperations::SplitFeatureGeometryOperation::create_rendered_geometry_layers()
{
	// Create a rendered layer to draw the line segments of polylines and polygons.
	d_line_segments_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// Create a rendered layer to draw the points in the geometry on top of the lines.
	// NOTE: this must be created second to get drawn on top.
	d_points_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// Create a rendered layer to draw a single point in the geometry on top of the usual points
	// when the mouse cursor hovers over one of them.
	// NOTE: this must be created third to get drawn on top of the points.
	d_highlight_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.
}
// 
void
GPlatesViewOperations::SplitFeatureGeometryOperation::connect_to_geometry_builder_signals()
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
GPlatesViewOperations::SplitFeatureGeometryOperation::disconnect_from_geometry_builder_signals()
{
	// Disconnect all signals from the current geometry builder.
	QObject::disconnect(&d_geometry_builder, 0, this, 0);
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::geometry_builder_stopped_updating_geometry()
{
	// The geometry builder has just potentially done a group of
	// geometry modifications and is now notifying us that it's finished.

	// Just clear and add all RenderedGeometry objects.
	// This could be optimised, if profiling says so, by listening to the other signals
	// generated by GeometryBuilder instead and only making the minimum changes needed.
	update_rendered_geometries();
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::split_feature(
		const GeometryBuilder::PointIndex insert_vertex_index,
		boost::optional<const GPlatesMaths::PointOnSphere> insert_pos_on_sphere)
{
	// The command that does the actual splitting feature.
	std::auto_ptr<QUndoCommand> split_feature_command(
			new SplitFeatureUndoCommand(
					d_feature_focus,
					d_model_interface,
					insert_vertex_index,
					insert_pos_on_sphere));

	// Command wraps insert vertex command with handing canvas tool choice and
	std::auto_ptr<QUndoCommand> undo_command(
			new GeometryOperationUndoCommand(
					QObject::tr("split feature"),
					split_feature_command,
					this,
					d_canvas_tool_workflows));

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the vertex is initially inserted.
	UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::update_rendered_geometries()
{
	// Clear all RenderedGeometry objects from the render layers first.
	d_line_segments_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();
	d_highlight_layer_ptr->clear_rendered_geometries();

	// Iterate through the internal geometries (currently only one is supported).
	for (GeometryBuilder::GeometryIndex geom_index = 0;
		geom_index < d_geometry_builder.get_num_geometries();
		++geom_index)
	{
		update_rendered_geometry(geom_index);
	}
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::update_rendered_geometry(
		GeometryBuilder::GeometryIndex geom_index)
{
	// All types of geometry have the points drawn the same.
	add_rendered_points(geom_index);

	const GeometryType::Value actual_geom_type =
		d_geometry_builder.get_actual_type_of_geometry(geom_index);

	if (actual_geom_type == GeometryType::POLYLINE ||
		actual_geom_type == GeometryType::POLYGON)
	{
		add_rendered_lines(geom_index, actual_geom_type);
	}
}

void
GPlatesViewOperations::SplitFeatureGeometryOperation::add_rendered_lines(
		GeometryBuilder::GeometryIndex geom_index,
		const GeometryType::Value actual_geom_type)
{
	const unsigned int num_points_in_geom = d_geometry_builder.get_num_points_in_geometry(geom_index);
	d_line_to_point_mapping.clear();

	if (num_points_in_geom < 2)
	{
		// We don't have even a single line segment so nothing to do.
		return;
	}

	// Get start and end of point sequence in current geometry.
	GeometryBuilder::point_const_iterator_type builder_geom_begin =
		d_geometry_builder.get_geometry_point_begin(geom_index);

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
GPlatesViewOperations::SplitFeatureGeometryOperation::add_rendered_points(
		GeometryBuilder::GeometryIndex geom_index)
{
	GeometryBuilder::point_const_iterator_type builder_geom_begin =
		d_geometry_builder.get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder.get_geometry_point_end(geom_index);

	if (builder_geom_begin == builder_geom_end)
	{
		return;
	}

	GeometryBuilder::point_const_iterator_type builder_geom_iter;
	for (builder_geom_iter = builder_geom_begin;
		builder_geom_iter != builder_geom_end;
		++builder_geom_iter)
	{
		const GPlatesMaths::PointOnSphere &point_on_sphere = *builder_geom_iter;

		RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_point_on_sphere(
			point_on_sphere,
			GeometryOperationParameters::SPLIT_FEATURE_MIDDLE_POINT_COLOUR,
			GeometryOperationParameters::LARGE_POINT_SIZE_HINT);

		// Add to the points layer.
		d_points_layer_ptr->add_rendered_geometry(rendered_geom);
	}

	//
	// Draw the coloured end points last so they are always drawn on top.
	//

	// Start point.
	const GPlatesMaths::PointOnSphere &start_point_on_sphere = *builder_geom_begin;
	RenderedGeometry start_point_rendered_geom = RenderedGeometryFactory::create_rendered_point_on_sphere(
			start_point_on_sphere,
			GeometryOperationParameters::SPLIT_FEATURE_START_POINT_COLOUR,
			GeometryOperationParameters::LARGE_POINT_SIZE_HINT);
	d_points_layer_ptr->add_rendered_geometry(start_point_rendered_geom);

	// End point.
	const GPlatesMaths::PointOnSphere &end_point_on_sphere = *(builder_geom_end - 1);
	RenderedGeometry end_point_rendered_geom = RenderedGeometryFactory::create_rendered_point_on_sphere(
			end_point_on_sphere,
			GeometryOperationParameters::SPLIT_FEATURE_END_POINT_COLOUR,
			GeometryOperationParameters::LARGE_POINT_SIZE_HINT);
	d_points_layer_ptr->add_rendered_geometry(end_point_rendered_geom);
}
