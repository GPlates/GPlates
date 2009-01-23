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

#include "qt-widgets/ViewportWindow.h"
#include "view-operations/AddPointGeometryOperation.h"
#include "view-operations/GeometryBuilderToolTarget.h"
#include "view-operations/RenderedGeometryCollection.h"

const GPlatesCanvasTools::DigitiseGeometry::non_null_ptr_type
GPlatesCanvasTools::DigitiseGeometry::create(
		GPlatesViewOperations::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryFactory &rendered_geometry_factory,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		// Ultimately would like to remove the following arguments...
		GPlatesGui::Globe &globe,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		const GPlatesQtWidgets::ViewportWindow &view_state)
{
	return DigitiseGeometry::non_null_ptr_type(
			new DigitiseGeometry(
					geom_type,
					geom_builder_tool_target,
					rendered_geometry_collection,
					rendered_geometry_factory,
					choose_canvas_tool,
					// Ultimately would like to remove the following arguments...
					globe,
					globe_canvas,
					view_state),
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesCanvasTools::DigitiseGeometry::DigitiseGeometry(
		GPlatesViewOperations::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryBuilderToolTarget &geom_builder_tool_target,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryFactory &rendered_geometry_factory,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		// Ultimately would like to remove the following arguments...
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_):
	CanvasTool(globe_, globe_canvas_),
	d_view_state_ptr(&view_state_),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_geom_builder_tool_target(&geom_builder_tool_target),
	d_default_geom_type(geom_type),
	d_add_point_geometry_operation(
		new GPlatesViewOperations::AddPointGeometryOperation(
				geom_type,
				&rendered_geometry_collection,
				&rendered_geometry_factory,
				choose_canvas_tool))
{
}


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

	// Tell everyone we're activating digitise geometry tool.
	d_geom_builder_tool_target->activate(
			GPlatesViewOperations::GeometryBuilderToolTarget::DIGITISE_GEOMETRY);

	// Ask which GeometryBuilder we are to operate on.
	GPlatesViewOperations::GeometryBuilder *geometry_builder =
			d_geom_builder_tool_target->get_geometry_builder_for_active_tool();

	// Ask which main rendered layer we are to operate on.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_layer_type =
			d_geom_builder_tool_target->get_main_rendered_layer_for_active_tool();

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

	// FIXME:  We may have to adjust the message if we are using a Map View.
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


void
GPlatesCanvasTools::DigitiseGeometry::handle_deactivation()
{
	// Deactivate our AddPointGeometryOperation.
	d_add_point_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::DigitiseGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	// Plain and simple append point.
	d_add_point_geometry_operation->add_point(oriented_click_pos_on_globe);
}
