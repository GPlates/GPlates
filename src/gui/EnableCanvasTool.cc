/* $Id$ */

/**
 * \file 
 * Enables/disables canvas tools.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "ChooseCanvasTool.h"
#include "FeatureFocus.h"

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::EnableCanvasTool::EnableCanvasTool(
		GPlatesQtWidgets::ViewportWindow &viewport_window,
		const GPlatesGui::FeatureFocus &feature_focus,
		const GPlatesViewOperations::GeometryOperationTarget &geom_operation_target,
		const GPlatesGui::ChooseCanvasTool &choose_canvas_tool) :
d_viewport_window(&viewport_window),
d_feature_focus(&feature_focus),
d_feature_geom_is_in_focus(feature_focus.focused_feature().is_valid()),
d_current_canvas_tool_type(GPlatesCanvasTools::CanvasToolType::NONE),
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

	connect_to_choose_canvas_tool(choose_canvas_tool);
}

void
GPlatesGui::EnableCanvasTool::initialise()
{
	// Set initial enable/disable state of canvas tools.
	d_feature_geom_is_in_focus = d_feature_focus->associated_reconstruction_geometry();

	update();

	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update()' above then you
	// don't need to enable/disable it here.
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
				GPlatesGui::FeatureFocus &)),
		this,
		SLOT(feature_focus_changed(
				GPlatesGui::FeatureFocus &)));
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
GPlatesGui::EnableCanvasTool::connect_to_choose_canvas_tool(
		const GPlatesGui::ChooseCanvasTool &choose_canvas_tool)
{
	// Connect to the choose canvas tool's signals.

	QObject::connect(
			&choose_canvas_tool,
			SIGNAL(chose_canvas_tool(
					GPlatesGui::ChooseCanvasTool &,
					GPlatesCanvasTools::CanvasToolType::Value)),
			this,
			SLOT(chose_canvas_tool(
					GPlatesGui::ChooseCanvasTool &,
					GPlatesCanvasTools::CanvasToolType::Value)));
}

void
GPlatesGui::EnableCanvasTool::feature_focus_changed(
		GPlatesGui::FeatureFocus &feature_focus)
{
	d_feature_geom_is_in_focus = feature_focus.associated_reconstruction_geometry();

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
GPlatesGui::EnableCanvasTool::chose_canvas_tool(
		GPlatesGui::ChooseCanvasTool &,
		GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type)
{
	// The current canvas tool has just changed.
	d_current_canvas_tool_type = canvas_tool_type;

	update();
}

void
GPlatesGui::EnableCanvasTool::update()
{
	update_move_geometry_tool();
	update_move_vertex_tool();
	update_insert_vertex_tool();
	update_split_feature_tool();
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
	// If we're currently using the build or edit topology tool then disable this tool.
	// This is because the topology tools set/modify the focused feature but for their
	// own purpose of adding topology sections and not for specifying a focused feature
	// for other tools to target.
	if (d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::BUILD_TOPOLOGY ||
		d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::EDIT_TOPOLOGY)
	{
		d_viewport_window->enable_move_vertex_tool(false);
		return;
	}

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
	// If we're currently using the build or edit topology tool then disable this tool.
	// This is because the topology tools set/modify the focused feature but for their
	// own purpose of adding topology sections and not for specifying a focused feature
	// for other tools to target.
	if (d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::BUILD_TOPOLOGY ||
		d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::EDIT_TOPOLOGY)
	{
		d_viewport_window->enable_insert_vertex_tool(false);
		return;
	}

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
GPlatesGui::EnableCanvasTool::update_split_feature_tool()
{
	if (
		d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYLINE ||
		d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::DIGITISE_MULTIPOINT ||
		d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::DIGITISE_POLYGON)
	{
		d_viewport_window->enable_split_feature_tool(false);
		return;
	}

	if(!d_feature_focus->is_valid())
	{
		d_viewport_window->enable_split_feature_tool(false);
		return;
	}


	unsigned int num_vertices;
	GPlatesViewOperations::GeometryType::Value geometry_type;
	boost::tie(num_vertices, geometry_type) = get_target_geometry_parameters_if_tool_chosen_next(
		GPlatesCanvasTools::CanvasToolType::SPLIT_FEATURE);

	bool enable;
	switch (geometry_type)
	{
	case GPlatesViewOperations::GeometryType::NONE:
	case GPlatesViewOperations::GeometryType::POINT:
	case GPlatesViewOperations::GeometryType::MULTIPOINT:
	case GPlatesViewOperations::GeometryType::POLYGON:
		enable = false;
		break;

	
	case GPlatesViewOperations::GeometryType::POLYLINE:
		enable = (num_vertices > 1);
		break;

	default:
		enable = false;
		break;
	}

	d_viewport_window->enable_split_feature_tool(enable);
}

void
GPlatesGui::EnableCanvasTool::update_delete_vertex_tool()
{
	// If we're currently using the build or edit topology tool then disable this tool.
	// This is because the topology tools set/modify the focused feature but for their
	// own purpose of adding topology sections and not for specifying a focused feature
	// for other tools to target.
	if (d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::BUILD_TOPOLOGY ||
		d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::EDIT_TOPOLOGY)
	{
		d_viewport_window->enable_delete_vertex_tool(false);
		return;
	}

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
	// If we're currently using the build or edit topology tool then disable this tool.
	// This is because the topology tools set/modify the focused feature but for their
	// own purpose of adding topology sections and not for specifying a focused feature
	// for other tools to target.
	if (d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::BUILD_TOPOLOGY ||
		d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::EDIT_TOPOLOGY)
	{
		d_viewport_window->enable_manipulate_pole_tool(false);
		return;
	}

	d_viewport_window->enable_manipulate_pole_tool(d_feature_geom_is_in_focus);
}

void
GPlatesGui::EnableCanvasTool::update_build_topology_tool()
{
	bool enable_build_topology_tool = false;
	
	// The build topology tool is enabled whenever it is the current tool
	// regardless of whether a feature is focused or not - this is because
	// the feature focus is used to add topology sections so it's always
	// focusing, unfocusing, etc while the tool is being used.
	if (d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::BUILD_TOPOLOGY)
	{
		enable_build_topology_tool = true;
	}
	// If the build topology and edit topology tools are not the current tool then it is only
	// enabled if a feature is *not* focused.
	else if (d_current_canvas_tool_type != GPlatesCanvasTools::CanvasToolType::EDIT_TOPOLOGY)
	{
		if (!d_feature_geom_is_in_focus)
		{
			enable_build_topology_tool = true;
		}
	}

	d_viewport_window->enable_build_topology_tool(enable_build_topology_tool);
}

void
GPlatesGui::EnableCanvasTool::update_edit_topology_tool()
{
	bool enable_edit_topology_tool = false;

	// The edit topology tool is enabled whenever it is the current tool
	// regardless of whether a feature is focused or not - this is because
	// the feature focus is used to add topology sections so it's always
	// focusing, unfocusing, etc while the tool is being used.
	if (d_current_canvas_tool_type == GPlatesCanvasTools::CanvasToolType::EDIT_TOPOLOGY)
	{
		enable_edit_topology_tool = true;
	}
	// If the edit topology tool is not the current tool then it is only
	// enabled if a feature is focused and that feature is a
	// topological closed plate polygon.
	else if (d_feature_geom_is_in_focus)
	{
		if (d_feature_focus->is_valid())
		{
			// Check feature type via qstrings
			//
			// FIXME: Do this check based on feature properties rather than feature type.
			// So if something looks like a TCPB (because it has a topology polygon property)
			// then treat it like one. For this to happen we first need TopologicalNetwork to
			// use a property type different than TopologicalPolygon.
			//
			static const QString topology_boundary_type_name ("TopologicalClosedPlateBoundary");
			static const QString topology_network_type_name ("TopologicalNetwork");
			QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
					d_feature_focus->focused_feature()->feature_type().get_name() );

			// Only activate for topologies.
			if (feature_type_name == topology_boundary_type_name ||
				feature_type_name == topology_network_type_name)
			{
				enable_edit_topology_tool = true;
			}
		}
	}

	d_viewport_window->enable_edit_topology_tool(enable_edit_topology_tool);
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


