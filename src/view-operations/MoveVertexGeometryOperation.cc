/* $Id$ */

/**
 * \file 
 * Moves points/vertices in a geometry as the user selects a vertex and drags it.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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

#include "GeometryBuilderUndoCommands.h"
#include "GeometryOperationUndo.h"
#include "QueryProximityThreshold.h"
#include "RenderedGeometryProximity.h"
#include "RenderedGeometryFactory.h"
#include "RenderedGeometryLayerVisitor.h"
#include "RenderedGeometryParameters.h"
#include "RenderedGeometryUtils.h"
#include "UndoRedo.h"

#include "app-logic/ReconstructionGeometryUtils.h"

#include "canvas-tools/GeometryOperationState.h"
#include "canvas-tools/ModifyGeometryState.h"

#include "gui/CanvasToolWorkflows.h"
#include "gui/FeatureFocus.h"

#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/ProximityCriteria.h"
#include "maths/ProximityHitDetail.h"

#include "presentation/ViewState.h"

#include "qt-widgets/TaskPanel.h"
#include "qt-widgets/ViewportWindow.h"



GPlatesViewOperations::MoveVertexGeometryOperation::MoveVertexGeometryOperation(
		GeometryBuilder &geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesCanvasTools::ModifyGeometryState &modify_geometry_state,
		RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		const QueryProximityThreshold &query_proximity_threshold,
		GPlatesGui::FeatureFocus &feature_focus) :
	d_geometry_builder(geometry_builder),
	d_geometry_operation_state(geometry_operation_state),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_main_rendered_layer_type(main_rendered_layer_type),
	d_canvas_tool_workflows(canvas_tool_workflows),
	d_query_proximity_threshold(query_proximity_threshold),
	d_selected_vertex_index(0),
	d_is_vertex_selected(false),
	d_is_vertex_highlighted(false),
	d_should_check_nearby_vertices(false),
	d_nearby_vertex_threshold(0.),
	d_feature_focus(feature_focus) 
{
	// For updating move-nearby-vertex parameters from the task panel widget. 
	QObject::connect(
			&modify_geometry_state,
			SIGNAL(snap_vertices_setup_changed(
					bool,double,bool,GPlatesModel::integer_plate_id_type)),
			this,
			SLOT(handle_snap_vertices_setup_changed(
					bool,double,bool,GPlatesModel::integer_plate_id_type)));
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::activate()
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
	d_highlight_point_layer_ptr->set_active(true);

	// Fill the rendered layers with RenderedGeometry objects by querying
	// the GeometryBuilder state.
	update_rendered_geometries();
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::deactivate()
{
	emit_unhighlight_signal(&d_geometry_builder);

	// Let others know there's no currently activated GeometryOperation.
	d_geometry_operation_state.set_no_active_geometry_operation();

	disconnect_from_geometry_builder_signals();

	// Get rid of all render layers, not just the highlighting, even if switching to drag or zoom tool
	// (which normally previously would display the most recent tool's layers).
	// This is because once we are deactivated we won't be able to update the render layers when/if
	// the reconstruction time changes.
	// This means the user won't see this tool's render layers while in the drag or zoom tool.
	d_lines_layer_ptr->set_active(false);
	d_points_layer_ptr->set_active(false);
	d_highlight_point_layer_ptr->set_active(false);
	d_lines_layer_ptr->clear_rendered_geometries();
	d_points_layer_ptr->clear_rendered_geometries();
	d_highlight_point_layer_ptr->clear_rendered_geometries();

	// User will have to click another vertex when this operation activates again.
	d_is_vertex_selected = false;
	d_is_vertex_highlighted = false;
}


void
GPlatesViewOperations::MoveVertexGeometryOperation::start_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
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


void
GPlatesViewOperations::MoveVertexGeometryOperation::update_drag(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere)
{
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
	
	d_geometry_builder.clear_secondary_geometries();
	// This will clear any secondary geometry highlighting and re-draw the "normal" move vertex geometries.
	update_rendered_geometries();
	
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::mouse_move(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		const double &closeness_inclusion_threshold)
{
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

		emit_highlight_point_signal(&d_geometry_builder,
				geometry_index,
				highlight_vertex_index,
				GeometryOperationParameters::HIGHLIGHT_COLOUR);
				
		d_is_vertex_highlighted = true;
		d_selected_vertex_index = highlight_vertex_index;
	}
	else
	{
		emit_unhighlight_signal(&d_geometry_builder);
		d_is_vertex_highlighted = false;
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
	if (!test_proximity(sorted_hits, proximity_criteria, *d_points_layer_ptr))
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
	// Create a rendered layer to draw the line segments of polylines and polygons.
	d_lines_layer_ptr =
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
	d_highlight_point_layer_ptr =
		d_rendered_geometry_collection.create_child_rendered_layer_and_transfer_ownership(
				d_main_rendered_layer_type);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::connect_to_geometry_builder_signals()
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
GPlatesViewOperations::MoveVertexGeometryOperation::disconnect_from_geometry_builder_signals()
{
	// Disconnect all signals from the current geometry builder.
	QObject::disconnect(&d_geometry_builder, 0, this, 0);
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
	update_rendered_secondary_geometries();
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::move_vertex(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere,
		bool is_intermediate_move)
{
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
					d_canvas_tool_workflows,
					d_move_vertex_command_id));

	// Push command onto undo list.
	// Note: the command's redo() gets executed inside the push() call and this is where
	// the vertex is initially moved.
	UndoRedo::instance().get_active_undo_stack().push(undo_command.release());
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_rendered_geometries()
{
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
		geom_index < d_geometry_builder.get_num_geometries();
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

	const GPlatesMaths::GeometryType::Value actual_geom_type =
		d_geometry_builder.get_actual_type_of_geometry(geom_index);

	switch (actual_geom_type)
	{
	case GPlatesMaths::GeometryType::POLYLINE:
		add_rendered_lines_for_polyline_on_sphere(geom_index);
		break;

	case GPlatesMaths::GeometryType::POLYGON:
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
		d_geometry_builder.get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder.get_geometry_point_end(geom_index);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
		GPlatesMaths::PolylineOnSphere::create_on_heap(builder_geom_begin, builder_geom_end);

	RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_polyline_on_sphere(
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
		d_geometry_builder.get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder.get_geometry_point_end(geom_index);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
		GPlatesMaths::PolygonOnSphere::create_on_heap(builder_geom_begin, builder_geom_end);

	RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_polygon_on_sphere(
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
		d_geometry_builder.get_geometry_point_begin(geom_index);
	GeometryBuilder::point_const_iterator_type builder_geom_end =
		d_geometry_builder.get_geometry_point_end(geom_index);

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
			d_geometry_builder.get_geometry_point(geometry_index, highlight_point_index);

	RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_point_on_sphere(
		highlight_point_on_sphere,
		GeometryOperationParameters::HIGHLIGHT_COLOUR,
		GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

	d_highlight_point_layer_ptr->add_rendered_geometry(rendered_geom);
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_secondary_geometries(
	const GPlatesMaths::PointOnSphere &point_on_sphere)
{
	d_geometry_builder.clear_secondary_geometries();

	GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type sorted_hits;
	
	double proximity_inclusion_threshold = d_nearby_vertex_threshold; 

	GPlatesMaths::ProximityCriteria criteria(point_on_sphere, proximity_inclusion_threshold);
	GPlatesViewOperations::test_vertex_proximity(
		sorted_hits,
		d_rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER,
		criteria);
			
	sorted_rendered_geometry_proximity_hits_type::const_iterator 
		it = sorted_hits.begin(),
		end = sorted_hits.end();
		
		
	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focus_rg =
			d_feature_focus.associated_reconstruction_geometry();	
	
	// We may want to extend this to store all geometries that have a vertex inside the proximity
	// threshold, rather than just the geometry which has the closest vertex. 
	boost::optional<RenderedGeometry> closest_non_focus_rendered_geom;
	double closest_closeness = 0.;
	unsigned int closest_vertex_index = 0;
	
	for (; it != end ; ++it)
	{
		RenderedGeometry rg = it->d_rendered_geom_layer->get_rendered_geometry(
			it->d_rendered_geom_index);
		
		ReconstructionGeometryFinder finder;
		rg.accept_visitor(finder);
		boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
				recon_geom = finder.get_reconstruction_geometry();
			

		// The focus geometry itself will return a hit from the test_vertex_proximity test,
		// so check that it's not the focus geometry before checking the closeness.
		if (recon_geom && *recon_geom != focus_rg)
		{
			if (it->d_proximity_hit_detail->index())
			{
				unsigned int vertex_index = *(it->d_proximity_hit_detail->index());
				//qDebug() << "Found non-focused geometry, vertex no: " << vertex_index;
				if (it->d_proximity_hit_detail->closeness() > closest_closeness)
				{
					closest_non_focus_rendered_geom.reset(rg);
					closest_vertex_index = vertex_index;
					closest_closeness = it->d_proximity_hit_detail->closeness();
				}
			}
			else
			{
				//qDebug() << "Found non-focused geometry, no vertex information";
			}
		}
	}
	
	// We have found a geometry with a vertex in range; add it to the geometry builder.
	// FIXME: may want to extend this to store multiple geometries that have
	// a vertex close to the highlighted vertex. Right now we deal only with the geometry that has 
	// the closest within-range vertex.
	if (closest_non_focus_rendered_geom)
	{
		ReconstructionGeometryFinder recon_geom_finder;
		closest_non_focus_rendered_geom->accept_visitor(recon_geom_finder);
		boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
				recon_geom = recon_geom_finder.get_reconstruction_geometry();
			

		if (recon_geom)
		{
			const boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> rfg =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							const GPlatesAppLogic::ReconstructedFeatureGeometry>(recon_geom.get());
			if (rfg)
			{
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id =
						rfg.get()->reconstruction_plate_id();
				if (d_should_use_plate_id_filter)
				{ 
					if ( d_filter_plate_id &&
						plate_id &&
						(*plate_id == *d_filter_plate_id))
						{
							d_geometry_builder.add_secondary_geometry(*recon_geom,closest_vertex_index);
						}
				}
				else
				{
				// No plate-id filter selected, so add the geometry. 
						d_geometry_builder.add_secondary_geometry(*recon_geom,closest_vertex_index);
				}
			}
		}
	}
	

}

void
GPlatesViewOperations::MoveVertexGeometryOperation::release_click()
{
	d_geometry_builder.clear_secondary_geometries();
	
	// This will clear the rendered geometry layers and re-draw the "normal" move vertex geometries.
	update_rendered_geometries();
	if (d_is_vertex_highlighted)
	{
		update_highlight_rendered_point(d_selected_vertex_index);
	}
}


void
GPlatesViewOperations::MoveVertexGeometryOperation::update_rendered_secondary_geometries()
{
	if (d_is_vertex_highlighted)
	{
		// FIXME: We're only grabbing the first of the secondary_geometries here. 
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geom = 
			d_geometry_builder.get_secondary_geometry();

		if (geom)
		{
			RenderedGeometryLayerFiller filler(d_points_layer_ptr,d_lines_layer_ptr);
			(*geom)->accept_visitor(filler);	
		}
	
	}

}

void
GPlatesViewOperations::MoveVertexGeometryOperation::left_press(
		const GPlatesMaths::PointOnSphere &oriented_pos_on_sphere, 
		const double &closeness_inclusion_threshold)
{
#if 0
	qDebug() << "is_vertex_highlighted: " << d_is_vertex_highlighted;
	qDebug() << "should check: " << d_should_check_nearby_vertices;	
#endif
	d_geometry_builder.clear_secondary_geometries();
	// If we're near a vertex in the focused geometry, then check other geometries in the model too. 
	if (d_is_vertex_highlighted && d_should_check_nearby_vertices)
	{
		// Use the highlighted point (rather than the mouse point) for searching for secondary geometries.
		const GPlatesMaths::PointOnSphere &highlight_point_on_sphere =
			d_geometry_builder.get_geometry_point(0, d_selected_vertex_index);
			
		update_secondary_geometries(highlight_point_on_sphere);
		update_rendered_secondary_geometries();
		update_highlight_secondary_vertices();
		//FIXME: find a better colour for highlighting the secondary geometries.
		//FIMXE: highlight the nearest vertex in any of the secondary geometries.
	}
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::handle_snap_vertices_setup_changed(
		bool should_check_nearby_vertices,
		double threshold,
		bool should_use_plate_id,
		GPlatesModel::integer_plate_id_type plate_id)
{
	d_should_check_nearby_vertices = should_check_nearby_vertices;
	d_nearby_vertex_threshold = std::cos(GPlatesMaths::convert_deg_to_rad(threshold));
	d_should_use_plate_id_filter = should_use_plate_id;
	d_filter_plate_id.reset(plate_id);
}

void
GPlatesViewOperations::MoveVertexGeometryOperation::update_highlight_secondary_vertices()
{
	boost::optional<GPlatesMaths::PointOnSphere> point = d_geometry_builder.get_secondary_vertex();
	
	if (point)
	{
		//qDebug() << "Found secondary highlight point";
		RenderedGeometry rendered_geom = RenderedGeometryFactory::create_rendered_point_on_sphere(
			*point,
			GeometryOperationParameters::HIGHLIGHT_COLOUR,
			GeometryOperationParameters::EXTRA_LARGE_POINT_SIZE_HINT);

		d_highlight_point_layer_ptr->add_rendered_geometry(rendered_geom);
	}
}
