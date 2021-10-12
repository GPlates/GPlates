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

#include "GlobeDigitiseGeometry.h"

#include "canvas-tools/CommonDigitiseGeometry.h"
#include "qt-widgets/ViewportWindow.h"
#include "view-operations/AddPointGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryCollection.h"

const GPlatesCanvasTools::GlobeDigitiseGeometry::non_null_ptr_type
GPlatesCanvasTools::GlobeDigitiseGeometry::create(
		GPlatesViewOperations::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryOperationTarget &geom_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		// Ultimately would like to remove the following arguments...
		GPlatesGui::Globe &globe,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		const GPlatesQtWidgets::ViewportWindow &view_state)
{
	return GlobeDigitiseGeometry::non_null_ptr_type(
			new GlobeDigitiseGeometry(
					geom_type,
					geom_operation_target,
					active_geometry_operation,
					rendered_geometry_collection,
					choose_canvas_tool,
					canvas_tool_type,
					query_proximity_threshold,
					// Ultimately would like to remove the following arguments...
					globe,
					globe_canvas,
					view_state),
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesCanvasTools::GlobeDigitiseGeometry::GlobeDigitiseGeometry(
	GPlatesViewOperations::GeometryType::Value geom_type,
	GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
	GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
	GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
	GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
	GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
	const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
	// Ultimately would like to remove the following arguments...
	GPlatesGui::Globe &globe_,
	GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
	const GPlatesQtWidgets::ViewportWindow &view_state_):
GlobeCanvasTool(globe_, globe_canvas_),
d_view_state_ptr(&view_state_),
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
{
}


GPlatesCanvasTools::GlobeDigitiseGeometry::~GlobeDigitiseGeometry()
{
	// boost::scoped_ptr destructor needs complete type.
}

void
GPlatesCanvasTools::GlobeDigitiseGeometry::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		CommonDigitiseGeometry::handle_activation(
			d_geometry_operation_target,
			d_default_geom_type,
			d_add_point_geometry_operation.get(),
			d_canvas_tool_type);

		if (d_default_geom_type == GPlatesViewOperations::GeometryType::MULTIPOINT) {
			d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new point."
				" Ctrl+drag to re-orient the globe."));
		} else {
			d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new vertex."
				" Ctrl+drag to re-orient the globe."));
		}
	}
}


void
GPlatesCanvasTools::GlobeDigitiseGeometry::handle_deactivation()
{
	// Deactivate our AddPointGeometryOperation.
	d_add_point_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::GlobeDigitiseGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	double closeness_inclusion_threshold =
		globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);

	CommonDigitiseGeometry::handle_left_click(d_add_point_geometry_operation.get(),
										oriented_click_pos_on_globe,
										closeness_inclusion_threshold);

}
