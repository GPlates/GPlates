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

#include <queue>
#include <QLocale>

#include "CommonClickGeometry.h"
#include "GlobeClickGeometry.h"

#include "gui/ProximityTests.h"
#include "global/InternalInconsistencyException.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/FeaturePropertiesDialog.h"
#include "view-operations/RenderedGeometryCollection.h"


const GPlatesCanvasTools::GlobeClickGeometry::non_null_ptr_type
GPlatesCanvasTools::GlobeClickGeometry::create(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::Globe &globe,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		const GPlatesQtWidgets::ViewportWindow &view_state,
		GPlatesGui::FeatureTableModel &clicked_table_model,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight)
{
	return GlobeClickGeometry::non_null_ptr_type(
			new GlobeClickGeometry(
					rendered_geom_collection,
					globe,
					globe_canvas,
					view_state,
					clicked_table_model,
					fp_dialog,
					feature_focus,
					geometry_focus_highlight),
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesCanvasTools::GlobeClickGeometry::GlobeClickGeometry(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::FeatureTableModel &clicked_table_model_,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
		GPlatesGui::FeatureFocus &feature_focus_,
		GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight_):
	GlobeCanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_view_state_ptr(&view_state_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_fp_dialog_ptr(&fp_dialog_),
	d_feature_focus_ptr(&feature_focus_),
	d_geometry_focus_highlight(&geometry_focus_highlight_)
{
}

void
GPlatesCanvasTools::GlobeClickGeometry::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		d_view_state_ptr->status_message(QObject::tr(
			"Click a geometry to choose a feature."
			" Shift+click to query immediately."
			" Ctrl+drag to re-orient the globe."));

		// Activate the geometry focus hightlight layer.
		d_rendered_geom_collection->set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);

	}
}


void
GPlatesCanvasTools::GlobeClickGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	double closeness_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	CommonClickGeometry::handle_left_click(oriented_click_pos_on_globe,
									closeness_inclusion_threshold,
									*d_view_state_ptr,
									*d_clicked_table_model_ptr,
									*d_feature_focus_ptr,
									*d_rendered_geom_collection);
	
}


void
GPlatesCanvasTools::GlobeClickGeometry::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	handle_left_click(click_pos_on_globe, oriented_click_pos_on_globe, is_on_globe);
	// If there is a feature focused, we'll assume that the user wants to look at it in detail.
	if (d_feature_focus_ptr->is_valid()) {
		fp_dialog().choose_query_widget_and_open();
	}
}
