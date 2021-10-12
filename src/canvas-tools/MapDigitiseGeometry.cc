/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway.
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

#include "MapDigitiseGeometry.h"

#include "canvas-tools/CommonDigitiseGeometry.h"
#include "maths/PointOnSphere.h"
#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ViewportWindow.h"
#include "view-operations/AddPointGeometryOperation.h"
#include "view-operations/GeometryOperationTarget.h"
#include "view-operations/RenderedGeometryCollection.h"

const GPlatesCanvasTools::MapDigitiseGeometry::non_null_ptr_type
GPlatesCanvasTools::MapDigitiseGeometry::create(
		GPlatesViewOperations::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryOperationTarget &geom_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		// Ultimately would like to remove the following arguments...
		GPlatesQtWidgets::MapCanvas &map_canvas,
		GPlatesQtWidgets::MapView &map_view,
		const GPlatesQtWidgets::ViewportWindow &view_state)
{
	return MapDigitiseGeometry::non_null_ptr_type(
			new MapDigitiseGeometry(
					geom_type,
					geom_operation_target,
					active_geometry_operation,
					rendered_geometry_collection,
					choose_canvas_tool,
					canvas_tool_type,
					query_proximity_threshold,
					// Ultimately would like to remove the following arguments...
					map_canvas,
					map_view,
					view_state),
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesCanvasTools::MapDigitiseGeometry::MapDigitiseGeometry(
		GPlatesViewOperations::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		// Ultimately would like to remove the following arguments...
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		const GPlatesQtWidgets::ViewportWindow &view_state_):
	MapCanvasTool(map_canvas_, map_view_),
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


GPlatesCanvasTools::MapDigitiseGeometry::~MapDigitiseGeometry()
{
	// boost::scoped_ptr destructor needs complete type.
}

void
GPlatesCanvasTools::MapDigitiseGeometry::handle_activation()
{
	if (map_view().isVisible())
	{
		CommonDigitiseGeometry::handle_activation(
			d_geometry_operation_target,
			d_default_geom_type,
			d_add_point_geometry_operation.get(),
			d_canvas_tool_type);


		if (d_default_geom_type == GPlatesViewOperations::GeometryType::MULTIPOINT) {
			d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new point."
				" Ctrl+drag to pan the map."));
		} else {
			d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new vertex."
				" Ctrl+drag to pan the map."));
		}
	}

}


void
GPlatesCanvasTools::MapDigitiseGeometry::handle_deactivation()
{
	// Deactivate our AddPointGeometryOperation.
	d_add_point_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::MapDigitiseGeometry::handle_left_click(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{
	if (!is_on_surface)
	{
		return;
	}

	double lon = click_point_on_scene.x();
	double lat = click_point_on_scene.y();

	boost::optional<GPlatesMaths::LatLonPoint> llp = 
		map_canvas().projection().inverse_transform(lon,lat);

	if (llp)
	{
		GPlatesMaths::PointOnSphere point_on_sphere = GPlatesMaths::make_point_on_sphere(*llp);	

		double closeness_inclusion_threshold = map_view().current_proximity_inclusion_threshold(point_on_sphere);

		CommonDigitiseGeometry::handle_left_click(
			d_add_point_geometry_operation.get(), point_on_sphere,closeness_inclusion_threshold);
	}
}
