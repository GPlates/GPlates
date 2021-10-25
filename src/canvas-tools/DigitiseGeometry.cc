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
#include "view-operations/RenderedGeometryCollection.h"


GPlatesCanvasTools::DigitiseGeometry::DigitiseGeometry(
		const status_bar_callback_type &status_bar_callback,
		GPlatesMaths::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryBuilder &geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold) :
	CanvasTool(status_bar_callback),
	d_default_geom_type(geom_type),
	d_geometry_builder(geometry_builder),
	d_add_point_geometry_operation(
		   new GPlatesViewOperations::AddPointGeometryOperation(
					geom_type,
					geometry_builder,
					geometry_operation_state,
					rendered_geometry_collection,
					main_rendered_layer_type,
					canvas_tool_workflows,
					query_proximity_threshold))
{  }


GPlatesCanvasTools::DigitiseGeometry::~DigitiseGeometry()
{
	// boost::scoped_ptr destructor needs complete type.
}

void
GPlatesCanvasTools::DigitiseGeometry::handle_activation()
{
	// In addition to adding points - our dual responsibility is to change
	// the type of geometry the builder is attempting to build.
	//
	// Set type to build - ignore returned undo operation (this is handled
	// at a higher level).
	d_geometry_builder.set_geometry_type_to_build(d_default_geom_type);

	// Activate our AddPointGeometryOperation - it will add points to the specified GeometryBuilder
	// and add RenderedGeometry objects to the specified main render layer.
	d_add_point_geometry_operation->activate();

	if (d_default_geom_type == GPlatesMaths::GeometryType::MULTIPOINT)
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
