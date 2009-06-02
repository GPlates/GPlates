/* $Id$ */

/**
 * \file 
 * Enables/disables canvas tools.
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

#include "EnableCanvasTool.h"

#include "FeatureFocus.h"

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::EnableCanvasTool::EnableCanvasTool(
		GPlatesQtWidgets::ViewportWindow &viewport_window,
		const GPlatesGui::FeatureFocus &feature_focus,
		const GPlatesViewOperations::GeometryOperationTarget &geom_operation_target) :
d_viewport_window(&viewport_window),
d_feature_focus(&feature_focus),
d_feature_geom_is_in_focus(feature_focus.focused_feature().is_valid()),
d_geom_operation_target(&geom_operation_target)
{
	connect_to_feature_focus(feature_focus);

	// Connect to the geometry builders used to digitise/modify temporary new geometry
	// and focused feature geometry.
	connect_to_geometry_builder(
			*d_geom_operation_target->get_digitise_new_geometry_builder());
	connect_to_geometry_builder(
			*d_geom_operation_target->get_focused_feature_geometry_builder());

	connect_to_geometry_operation_target(geom_operation_target);
}

void
GPlatesGui::EnableCanvasTool::initialise()
{
	// Set initial enable/disable state of canvas tools.
	d_feature_geom_is_in_focus = d_feature_focus->associated_rfg();

	update();

	// These tools are always enabled.
	d_viewport_window->enable_drag_globe_tool(true);
	d_viewport_window->enable_zoom_globe_tool(true);
	d_viewport_window->enable_click_geometry_tool(true);
	d_viewport_window->enable_digitise_polyline_tool(true);
	d_viewport_window->enable_digitise_multipoint_tool(true);
	d_viewport_window->enable_digitise_polygon_tool(true);
}

void
GPlatesGui::EnableCanvasTool::connect_to_feature_focus(
		const GPlatesGui::FeatureFocus &feature_focus)
{
	QObject::connect(
		&feature_focus,
		SIGNAL(focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
		this,
		SLOT(feature_focus_changed(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
}

void
GPlatesGui::EnableCanvasTool::connect_to_geometry_builder(
		const GPlatesViewOperations::GeometryBuilder &geometry_builder)
{
	// Connect to the current geometry builder's signals.

	// GeometryBuilder has just finished updating geometry.
	QObject::connect(
			&geometry_builder,
			SIGNAL(stopped_updating_geometry_excluding_intermediate_moves()),
			this,
			SLOT(geometry_builder_stopped_updating_geometry_excluding_intermediate_moves()));
}

void
GPlatesGui::EnableCanvasTool::connect_to_geometry_operation_target(
		const GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target)
{
	// Connect to the geometry operation target's signals.

	QObject::connect(
			&geometry_operation_target,
			SIGNAL(switched_geometry_builder(
					GPlatesViewOperations::GeometryOperationTarget &,
					GPlatesViewOperations::GeometryBuilder *)),
			this,
			SLOT(switched_geometry_builder(
					GPlatesViewOperations::GeometryOperationTarget &,
					GPlatesViewOperations::GeometryBuilder *)));
}

void
GPlatesGui::EnableCanvasTool::feature_focus_changed(
		GPlatesModel::FeatureHandle::weak_ref /*focused_feature*/,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	d_feature_geom_is_in_focus = focused_geometry;

	update();
}

void
GPlatesGui::EnableCanvasTool::geometry_builder_stopped_updating_geometry_excluding_intermediate_moves()
{
	// We use this to determine if a geometry, that's being operated on or will potentially
	// be operated on, has got vertices or not.

	update();
}

void
GPlatesGui::EnableCanvasTool::switched_geometry_builder(
		GPlatesViewOperations::GeometryOperationTarget &,
		GPlatesViewOperations::GeometryBuilder *)
{
	// The targeted geometry builder has changed.
	// This means we need to check its number of vertices as this can
	// affect whether a canvas tool is enabled or not.

	update();
}

void
GPlatesGui::EnableCanvasTool::update()
{
	update_move_geometry_tool();
	update_move_vertex_tool();
	update_insert_vertex_tool();
	update_delete_vertex_tool();
	update_manipulate_pole_tool();
	update_build_topology_tool();
	update_edit_topology_tool();
}

void
GPlatesGui::EnableCanvasTool::update_move_geometry_tool()
{
	d_viewport_window->enable_move_geometry_tool(d_feature_geom_is_in_focus);
}

void
GPlatesGui::EnableCanvasTool::update_move_vertex_tool()
{
	//
	// Enable the move vertex tool if there are vertices.
	// 

	// Get the number of vertices and geometry type if tool is chosen next.
	unsigned int num_vertices;
	GPlatesViewOperations::GeometryType::Value geometry_type;
	boost::tie(num_vertices, geometry_type) = get_target_geometry_parameters_if_tool_chosen_next(
			GPlatesCanvasTools::CanvasToolType::MOVE_VERTEX);

	const bool enable = (num_vertices > 0);

	d_viewport_window->enable_move_vertex_tool(enable);
}

void
GPlatesGui::EnableCanvasTool::update_insert_vertex_tool()
{
	//
	// Enable the insert vertex tool if inserting a vertex won't change the type of
	// geometry. In other words disable in the following situations:
	//   * Geometry is a point.
	//
	// Note that upon insertion of a new vertex a polyline stays a polyline and
	// a polygon stays a polygon.
	//
	// Also for newly digitised geometry the desired geometry type could be a polygon
	// but the user has only added one or two vertices. So it's not really a polygon yet
	// but since the desired geometry type is a polygon we allow the insertion.
	// 

	// Get the number of vertices and geometry type if tool is chosen next.
	unsigned int num_vertices;
	GPlatesViewOperations::GeometryType::Value geometry_type;
	boost::tie(num_vertices, geometry_type) = get_target_geometry_parameters_if_tool_chosen_next(
			GPlatesCanvasTools::CanvasToolType::INSERT_VERTEX);

	bool enable;
	switch (geometry_type)
	{
	case GPlatesViewOperations::GeometryType::NONE:
	case GPlatesViewOperations::GeometryType::POINT:
		enable = false;
		break;

	case GPlatesViewOperations::GeometryType::MULTIPOINT:
	case GPlatesViewOperations::GeometryType::POLYLINE:
	case GPlatesViewOperations::GeometryType::POLYGON:
		enable = (num_vertices > 0);
		break;

	default:
		enable = false;
		break;
	}

	d_viewport_window->enable_insert_vertex_tool(enable);
}

void
GPlatesGui::EnableCanvasTool::update_delete_vertex_tool()
{
	//
	// Enable the delete vertex tool if deleting a vertex won't change the type of
	// geometry. In other words disable in the following situations:
	//   * Geometry is a point or multipoint with one vertex.
	//   * Geometry is a polyline with two vertices.
	//   * Geometry is a polygon with three vertices.
	//

	// Get the number of vertices and geometry type if tool is chosen next.
	unsigned int num_vertices;
	GPlatesViewOperations::GeometryType::Value geometry_type;
	boost::tie(num_vertices, geometry_type) = get_target_geometry_parameters_if_tool_chosen_next(
			GPlatesCanvasTools::CanvasToolType::DELETE_VERTEX);

	bool enable;
	switch (geometry_type)
	{
	case GPlatesViewOperations::GeometryType::NONE:
	case GPlatesViewOperations::GeometryType::POINT:
		enable = false;
		break;

	case GPlatesViewOperations::GeometryType::MULTIPOINT:
		enable = (num_vertices > 1);
		break;

	case GPlatesViewOperations::GeometryType::POLYLINE:
		enable = (num_vertices > 2);
		break;

	case GPlatesViewOperations::GeometryType::POLYGON:
		enable = (num_vertices > 3);
		break;

	default:
		enable = false;
		break;
	}

	d_viewport_window->enable_delete_vertex_tool(enable);
}

void
GPlatesGui::EnableCanvasTool::update_manipulate_pole_tool()
{
	d_viewport_window->enable_manipulate_pole_tool(d_feature_geom_is_in_focus);
}

void
GPlatesGui::EnableCanvasTool::update_build_topology_tool()
{
	// only enable tool if a feature is focused
	d_viewport_window->enable_build_topology_tool(d_feature_geom_is_in_focus);
}

void
GPlatesGui::EnableCanvasTool::update_edit_topology_tool()
{
	// only enable tool if a feature is focused
	d_viewport_window->enable_edit_topology_tool(d_feature_geom_is_in_focus);
}


boost::tuple<unsigned int, GPlatesViewOperations::GeometryType::Value>
GPlatesGui::EnableCanvasTool::get_target_geometry_parameters_if_tool_chosen_next(
		GPlatesCanvasTools::CanvasToolType::Value next_canvas_tool) const
{
	// Find out what geometry builder would be targeted if we switched to the
	// canvas tool of type 'next_canvas_tool'.
	GPlatesViewOperations::GeometryBuilder *geom_builder_for_next_canvas_tool =
			d_geom_operation_target->get_geometry_builder_if_canvas_tool_is_chosen_next(
					next_canvas_tool);

	// See if the geometry builder has more than one vertex.
	if (geom_builder_for_next_canvas_tool &&
		geom_builder_for_next_canvas_tool->get_num_geometries() > 0)
	{
		// We currently only support a single internal geometry so set geom index to zero.
		const unsigned int num_vertices =
				geom_builder_for_next_canvas_tool->get_num_points_in_geometry(0/*geom_index*/);

		// Note that if the target geometry is newly digitised geometry then the geometry type
		// is the type the user is trying to build and not the actual type of the current geometry.
		// For example, user could be digitising a polygon but has only added two points so far -
		// so the geometry type is polygon but the actual current type is polyline. When they add
		// another point the actual type will match the geometry type.
		const GPlatesViewOperations::GeometryType::Value geometry_type =
				geom_builder_for_next_canvas_tool->get_geometry_build_type();

		return boost::make_tuple(num_vertices, geometry_type);
	}

	return boost::make_tuple(0, GPlatesViewOperations::GeometryType::NONE);
}


