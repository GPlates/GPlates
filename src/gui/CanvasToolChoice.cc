/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "CanvasToolChoice.h"
#include "canvas-tools/ReorientGlobe.h"
#include "canvas-tools/ZoomGlobe.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/DigitiseGeometry.h"
#include "canvas-tools/MoveGeometry.h"
#include "canvas-tools/MoveVertex.h"

#include "qt-widgets/DigitisationWidget.h"


GPlatesGui::CanvasToolChoice::CanvasToolChoice(
		Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		FeatureTableModel &clicked_table_model,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesQtWidgets::DigitisationWidget &digitisation_widget_):
	d_reorient_globe_tool_ptr(GPlatesCanvasTools::ReorientGlobe::create(globe_, globe_canvas_,
			view_state_)),
	d_zoom_globe_tool_ptr(GPlatesCanvasTools::ZoomGlobe::create(globe_, globe_canvas_,
			view_state_)),
	d_click_geometry_tool_ptr(GPlatesCanvasTools::ClickGeometry::create(globe_, globe_canvas_,
			view_state_, clicked_table_model, fp_dialog_, feature_focus)),
	d_digitise_polyline_tool_ptr(GPlatesCanvasTools::DigitiseGeometry::create(globe_, globe_canvas_,
			view_state_, digitisation_widget_, GPlatesQtWidgets::DigitisationWidget::POLYLINE)),
	d_digitise_multipoint_tool_ptr(GPlatesCanvasTools::DigitiseGeometry::create(globe_, globe_canvas_,
			view_state_, digitisation_widget_, GPlatesQtWidgets::DigitisationWidget::MULTIPOINT)),
	d_digitise_polygon_tool_ptr(GPlatesCanvasTools::DigitiseGeometry::create(globe_, globe_canvas_,
			view_state_, digitisation_widget_, GPlatesQtWidgets::DigitisationWidget::POLYGON)),
	d_move_geometry_tool_ptr(GPlatesCanvasTools::MoveGeometry::create(globe_, globe_canvas_,
			view_state_)),
	d_move_vertex_tool_ptr(GPlatesCanvasTools::MoveVertex::create(globe_, globe_canvas_,
			view_state_)),
	d_tool_choice_ptr(d_reorient_globe_tool_ptr)
{
	tool_choice().handle_activation();
}


void
GPlatesGui::CanvasToolChoice::change_tool_if_necessary(
		CanvasTool::non_null_ptr_type new_tool_choice)
{
	if (new_tool_choice == d_tool_choice_ptr) {
		// The specified tool is already chosen.  Nothing to do here.
		return;
	}
	d_tool_choice_ptr->handle_deactivation();
	d_tool_choice_ptr = new_tool_choice;
	d_tool_choice_ptr->handle_activation();
}

