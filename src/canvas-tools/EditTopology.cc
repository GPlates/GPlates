/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2009-02-06 15:36:27 -0800 (Fri, 06 Feb 2009) $
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

#include <queue>
#include <QLocale>
#include <QDebug>
#include <QString>

#include "EditTopology.h"

#include "gui/ProximityTests.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "maths/LatLonPointConversions.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "global/InternalInconsistencyException.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"
#include "property-values/XsString.h"
#include "feature-visitors/PropertyValueFinder.h"

GPlatesCanvasTools::EditTopology::EditTopology(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::TopologySectionsContainer &topology_sections_container,
				GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
				GPlatesGui::FeatureFocus &feature_focus):
	CanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_view_state_ptr(&view_state_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_topology_sections_container_ptr(&topology_sections_container),
	d_topology_tools_widget_ptr(&topology_tools_widget),
	d_feature_focus_ptr(&feature_focus)
{

}


	

void
GPlatesCanvasTools::EditTopology::handle_activation()
{
// FIXME: add status message
#if 0
		d_view_state_ptr->status_message(QObject::tr(
				"Click on features to choose segments for the boundary."
				" Ctrl+drag to reorient the globe."));
#endif

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
std::cout << "GPlatesCanvasTools::EditTopology::handle_left_click" << std::endl;

	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
		oriented_click_pos_on_globe);

	// send the click point to the widget
	d_topology_tools_widget_ptr->set_click_point( llp.latitude(), llp.longitude() );

	// Show the 'Clicked' Feature Table
	d_view_state_ptr->choose_clicked_geometry_table();

	//
	// From ClickGeometry
	//
	double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	// What did the user click on just now?
	std::priority_queue<GPlatesGui::ProximityTests::ProximityHit> sorted_hits;

	GPlatesGui::ProximityTests::find_close_rfgs(sorted_hits, view_state().reconstruction(),
			oriented_click_pos_on_globe, proximity_inclusion_threshold);
	
	// Give the user some useful feedback in the status bar.
	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	} else if (sorted_hits.size() == 1) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometry.").arg(sorted_hits.size()));
	} else {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	}

	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	d_clicked_table_model_ptr->clear();

	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked 0 geometries."));
		// User clicked on empty space! Clear the currently focused feature.
		d_feature_focus_ptr->unset_focus();
		emit no_hits_found();
		return;
	}

	// Populate the 'Clicked' FeatureTableModel.
	d_clicked_table_model_ptr->begin_insert_features(0, static_cast<int>(sorted_hits.size()) - 1);
	while ( ! sorted_hits.empty())
	{
		d_clicked_table_model_ptr->geometry_sequence().push_back(
				sorted_hits.top().d_recon_geometry);
		sorted_hits.pop();
	}
	d_clicked_table_model_ptr->end_insert_features();
	d_view_state_ptr->highlight_first_clicked_feature_table_row();
	emit sorted_hits_updated();
}
