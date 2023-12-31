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

#include "ClickGeometry.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyInternalUtils.h"

#include "gui/AddClickedGeometriesToFeatureTable.h"

#include "maths/PointOnSphere.h"

#include "qt-widgets/FeaturePropertiesDialog.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryUtils.h"


GPlatesCanvasTools::ClickGeometry::ClickGeometry(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::GeometryBuilder &focused_feature_geometry_builder,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::FeatureTableModel &clicked_table_model_,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
		GPlatesGui::FeatureFocus &feature_focus_,
		GPlatesAppLogic::ApplicationState &application_state_) :
	CanvasTool(status_bar_callback),
	d_focused_feature_geometry_builder(focused_feature_geometry_builder),
	d_rendered_geom_collection(rendered_geom_collection),
	d_main_rendered_layer_type(main_rendered_layer_type),
	d_view_state_ptr(view_state_),
	d_clicked_table_model_ptr(clicked_table_model_),
	d_fp_dialog_ptr(fp_dialog_),
	d_feature_focus_ptr(feature_focus_),
	d_reconstruct_graph(application_state_.get_reconstruct_graph()),
	d_filter_reconstruction_geometry_predicate(
			&GPlatesGui::default_filter_reconstruction_geometry_predicate)
{  }


void
GPlatesCanvasTools::ClickGeometry::handle_activation()
{
	set_status_bar_message(QT_TR_NOOP("Click a geometry to choose a feature. Shift+click to query immediately."));

	// Only display focused feature when this tool is active.
	d_rendered_geom_collection.get_main_rendered_layer(d_main_rendered_layer_type)->set_active();
}


void
GPlatesCanvasTools::ClickGeometry::handle_deactivation()
{
	// Only display focused feature when this tool is active.
	d_rendered_geom_collection.get_main_rendered_layer(d_main_rendered_layer_type)->set_active(false);
}


void
GPlatesCanvasTools::ClickGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_clicked_geom_seq.clear();

	GPlatesGui::get_clicked_geometries(
			d_clicked_geom_seq,
			point_on_sphere,
			proximity_inclusion_threshold,
			d_rendered_geom_collection,
			d_filter_reconstruction_geometry_predicate);

	GPlatesGui::add_clicked_geometries_to_feature_table(
			d_clicked_geom_seq,
			d_view_state_ptr,
			d_clicked_table_model_ptr,
			d_feature_focus_ptr,
			d_reconstruct_graph);
}


void
GPlatesCanvasTools::ClickGeometry::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	handle_left_click(
			point_on_sphere,
			is_on_earth,
			proximity_inclusion_threshold);

	// If there is a feature focused, we'll assume that the user wants to look at it in detail.
	if (d_feature_focus_ptr.is_valid())
	{
		fp_dialog().choose_query_widget_and_open();
	}
}

