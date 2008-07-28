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

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/EditGeometryWidget.h"
#include "qt-widgets/DigitisationWidget.h"
#include "maths/LatLonPointConversions.h"


GPlatesCanvasTools::DigitiseGeometry::DigitiseGeometry(
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesQtWidgets::DigitisationWidget &digitisation_widget_,
		GPlatesQtWidgets::DigitisationWidget::GeometryType geom_type_):
	CanvasTool(globe_, globe_canvas_),
	d_view_state_ptr(&view_state_),
	d_digitisation_widget_ptr(&digitisation_widget_),
	d_default_geom_type(geom_type_)
{  }


void
GPlatesCanvasTools::DigitiseGeometry::handle_activation()
{
	// FIXME: Could be pithier.
	// FIXME: May have to adjust message if we are using Map view.
	d_view_state_ptr->status_message(QObject::tr(
			"Click globe to add new geometry. Ctrl+Drag to reorient globe."));
	
	// Clicking these canvas tools has the same effect as changing the combo box
	// of the DigitisationWidget.
	d_digitisation_widget_ptr->change_geometry_type(d_default_geom_type);
}


void
GPlatesCanvasTools::DigitiseGeometry::handle_deactivation()
{  }


void
GPlatesCanvasTools::DigitiseGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
			oriented_click_pos_on_globe);
	
	// Plain and simple append point to digitisation widget, default geometry.
	d_digitisation_widget_ptr->append_point_to_geometry(llp.latitude(), llp.longitude());
}


void
GPlatesCanvasTools::DigitiseGeometry::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe)
{
	handle_left_click(initial_pos_on_globe, oriented_initial_pos_on_globe, was_on_globe);
}


