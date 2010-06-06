/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2009-02-06 15:36:27 -0800 (Fri, 06 Feb 2009) $
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2008, 2009 California Institute of Technology 
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
#include <QDebug>
#include <QString>

#include "BuildTopology.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyInternalUtils.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "global/InternalInconsistencyException.h"
#include "gui/AddClickedGeometriesToFeatureTable.h"
#include "maths/LatLonPoint.h"
#include "model/FeatureHandle.h"
#include "presentation/ViewState.h"
#include "property-values/XsString.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/TopologyToolsWidget.h"
#include "qt-widgets/ViewportWindow.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"
#include "view-operations/RenderedGeometryProximity.h"
#include "view-operations/RenderedGeometryUtils.h"


GPlatesCanvasTools::BuildTopology::BuildTopology(
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesPresentation::ViewState &view_state_,
		const GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesGui::FeatureTableModel &clicked_table_model_,	
		GPlatesGui::TopologySectionsContainer &topology_sections_container,
		GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
		GPlatesAppLogic::ApplicationState &application_state):
	GlobeCanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&view_state_.get_rendered_geometry_collection()),
	d_viewport_window_ptr(&viewport_window_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_topology_sections_container_ptr(&topology_sections_container),
	d_topology_tools_widget_ptr(&topology_tools_widget),
	d_feature_focus_ptr(&view_state_.get_feature_focus()),
	d_reconstruct_graph(application_state.get_reconstruct_graph())
{
}


void
GPlatesCanvasTools::BuildTopology::handle_activation()
{
	// Activate rendered layer.
	d_rendered_geom_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// ONLY allow this tool to active with no foucs
	if ( d_feature_focus_ptr->is_valid() )
	{
		// unset the focus
		d_feature_focus_ptr->unset_focus();
	}

	d_topology_tools_widget_ptr->activate( GPlatesGui::TopologyTools::BUILD );
}


void
GPlatesCanvasTools::BuildTopology::handle_deactivation()
{
	d_topology_tools_widget_ptr->deactivate();
}


void
GPlatesCanvasTools::BuildTopology::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	// Show the 'Clicked' Feature Table
	d_viewport_window_ptr->choose_clicked_geometry_table();

	const double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	GPlatesGui::add_clicked_geometries_to_feature_table(
			oriented_click_pos_on_globe,
			proximity_inclusion_threshold,
			*d_viewport_window_ptr,
			*d_clicked_table_model_ptr,
			*d_feature_focus_ptr,
			*d_rendered_geom_collection,
			d_reconstruct_graph,
			&GPlatesAppLogic::TopologyInternalUtils::include_only_reconstructed_feature_geometries);

	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
		oriented_click_pos_on_globe);
}


void
GPlatesCanvasTools::BuildTopology::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	handle_left_click(click_pos_on_globe, oriented_click_pos_on_globe, is_on_globe);

	// Pass the click info to the widget.
	//
	// NOTE: This is done after adding the clicked geometries to the feature table
	// to ensure that the focused feature is set, or unset, first since our handling
	// of shift-left-click relies on this. For example, changing the stored click point
	// in a topological section which only changes it if there's a focused feature
	// at the clicked point.
	//
	d_topology_tools_widget_ptr->handle_shift_left_click(
		click_pos_on_globe, oriented_click_pos_on_globe, is_on_globe);
}
