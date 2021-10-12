/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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


#include "CommonDigitiseGeometry.h"
#include "view-operations/AddPointGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryCollection.h"

void
GPlatesCanvasTools::CommonDigitiseGeometry::handle_activation(
	GPlatesViewOperations::GeometryOperationTarget *geometry_operation_target,
	GPlatesViewOperations::GeometryType::Value &default_geom_type,
	GPlatesViewOperations::AddPointGeometryOperation *add_point_geometry_operation,
	GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type
	)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Ask which GeometryBuilder we are to operate on.
	// Note: we must pass the type of canvas tool in (see GeometryOperationTarget for explanation).
	// Returned GeometryBuilder should not be NULL but might be if tools are not
	// enable/disabled properly.
	GPlatesViewOperations::GeometryBuilder *geometry_builder =
			geometry_operation_target->get_and_set_current_geometry_builder_for_newly_activated_tool(
					canvas_tool_type);

	// Ask which main rendered layer we are to operate on.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_layer_type =
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER;

	// In addition to adding points - our dual responsibility is to change
	// the type of geometry the builder is attempting to build.
	//
	// Set type to build - ignore returned undo operation (this is handled
	// at a higher level).
	geometry_builder->set_geometry_type_to_build(default_geom_type);

	// Activate our AddPointGeometryOperation - it will add points to the
	// specified GeometryBuilder and add RenderedGeometry objects
	// to the specified main render layer.
	add_point_geometry_operation->activate(geometry_builder, main_layer_type);

}

void
GPlatesCanvasTools::CommonDigitiseGeometry::handle_left_click(
	GPlatesViewOperations::AddPointGeometryOperation *add_point_geometry_operation,
	const GPlatesMaths::PointOnSphere &point_on_sphere,
	const double &closeness_inclusion_threshold)
{
	// Plain and simple append point.
	add_point_geometry_operation->add_point(point_on_sphere,closeness_inclusion_threshold);
}
