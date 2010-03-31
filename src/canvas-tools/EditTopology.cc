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

#include "EditTopology.h"

#include "app-logic/Reconstruct.h"
#include "app-logic/TopologyInternalUtils.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "global/InternalInconsistencyException.h"
#include "gui/AddClickedGeometriesToFeatureTable.h"
#include "maths/LatLonPoint.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "property-values/XsString.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/TopologyToolsWidget.h"
#include "qt-widgets/ViewportWindow.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"
#include "presentation/ViewState.h"


GPlatesCanvasTools::EditTopology::EditTopology(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				GPlatesPresentation::ViewState &view_state_,
				const GPlatesQtWidgets::ViewportWindow &viewport_window_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget):
	GlobeCanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&view_state_.get_rendered_geometry_collection()),
	d_reconstruct_ptr(&view_state_.get_reconstruct()),
	d_viewport_window_ptr(&viewport_window_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_topology_sections_container_ptr(&topology_sections_container),
	d_topology_tools_widget_ptr(&topology_tools_widget),
	d_feature_focus_ptr(&view_state_.get_feature_focus())
{
}


	

void
GPlatesCanvasTools::EditTopology::handle_activation()
{
	// This tool must have a focused feature to activate 
	if ( ! d_feature_focus_ptr->is_valid() )
	{
		// switch to the choose feature tool
		// FIXME:  Since ViewportWindow is passed as a const ref cannot call this :
		// d_viewport_window_ptr->choose_click_geometry_tool();
		return;
	}

	// else check type 

	// Check feature type via qstrings 
	//
	// FIXME: Do this check based on feature properties rather than feature type.
	// So if something looks like a TCPB (because it has a topology polygon property)
	// then treat it like one. For this to happen we first need TopologicalNetwork to
	// use a property type different than TopologicalPolygon.
	//
	static const QString topology_boundary_type_name ("TopologicalClosedPlateBoundary");
	static const QString topology_network_type_name ("TopologicalNetwork");
	QString feature_type_name = GPlatesUtils::make_qstring_from_icu_string(
		d_feature_focus_ptr->focused_feature()->handle_data().feature_type().get_name() );

	// Only activate for topologies
	if ( ( feature_type_name != topology_boundary_type_name ) &&
		( feature_type_name != topology_network_type_name ) )
	{
		// unset the focus
 		d_feature_focus_ptr->unset_focus();
		
		return;
	}

	// else, all checks passed , continue to activate the low level tools 

	// Activate rendered layer.
	d_rendered_geom_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_topology_tools_widget_ptr->activate( GPlatesGui::TopologyTools::EDIT );
}

void
GPlatesCanvasTools::EditTopology::handle_deactivation()
{
	d_topology_tools_widget_ptr->deactivate();
}


void
GPlatesCanvasTools::EditTopology::handle_left_click(
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
			&GPlatesAppLogic::TopologyInternalUtils::include_only_reconstructed_feature_geometries);

	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
		oriented_click_pos_on_globe);

	// Send the click point to the widget.
	//
	// NOTE: This is done after adding the clicked geometries to the feature table
	// to ensure that the focused feature is set, or unset, first since our handling
	// of 'set_click_point()' relies on this - because it tracks the feature that was
	// clicked on.
	d_topology_tools_widget_ptr->set_click_point( llp.latitude(), llp.longitude() );
}


void
GPlatesCanvasTools::EditTopology::handle_shift_left_click(
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

