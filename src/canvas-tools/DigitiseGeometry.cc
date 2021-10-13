/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include "DigitiseGeometry.h"

#include "view-operations/AddPointGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryCollection.h"

GPlatesCanvasTools::DigitiseGeometry::DigitiseGeometry(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold) :
	CanvasTool(status_bar_callback),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_geometry_operation_target(&geometry_operation_target),
	d_canvas_tool_type(canvas_tool_type),
	d_default_geom_type(geom_type),
	d_add_point_geometry_operation(
		   new GPlatesViewOperations::AddPointGeometryOperation(
			   geom_type,
			   geometry_operation_target,
			   active_geometry_operation,
			   &rendered_geometry_collection,
			   choose_canvas_tool,
			   query_proximity_threshold))
{  }


GPlatesCanvasTools::DigitiseGeometry::~DigitiseGeometry()
{
	// boost::scoped_ptr destructor needs complete type.
}

void
GPlatesCanvasTools::DigitiseGeometry::handle_activation()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Ask which GeometryBuilder we are to operate on.
	// Note: we must pass the type of canvas tool in (see GeometryOperationTarget for explanation).
	// Returned GeometryBuilder should not be NULL but might be if tools are not
	// enable/disabled properly.
	GPlatesViewOperations::GeometryBuilder *geometry_builder =
			d_geometry_operation_target->get_and_set_current_geometry_builder_for_newly_activated_tool(
					d_canvas_tool_type);

	// Ask which main rendered layer we are to operate on.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_layer_type =
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER;

	// In addition to adding points - our dual responsibility is to change
	// the type of geometry the builder is attempting to build.
	//
	// Set type to build - ignore returned undo operation (this is handled
	// at a higher level).
	geometry_builder->set_geometry_type_to_build(d_default_geom_type);

	// Activate our AddPointGeometryOperation - it will add points to the
	// specified GeometryBuilder and add RenderedGeometry objects
	// to the specified main render layer.
	d_add_point_geometry_operation->activate(geometry_builder, main_layer_type);

	if (d_default_geom_type == GPlatesViewOperations::GeometryType::MULTIPOINT)
	{
		set_status_bar_message(QT_TR_NOOP("Click to draw a new point."));
	}
	else
	{
		set_status_bar_message(QT_TR_NOOP("Click to draw a new vertex."));
	}
}

void
GPlatesCanvasTools::DigitiseGeometry::handle_deactivation()
{
	// Deactivate our AddPointGeometryOperation.
	d_add_point_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::DigitiseGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_add_point_geometry_operation->add_point(
			point_on_sphere,
			proximity_inclusion_threshold);
}

