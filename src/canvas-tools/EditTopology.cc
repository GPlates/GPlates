/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2009-02-06 15:36:27 -0800 (Fri, 06 Feb 2009) $
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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


GPlatesCanvasTools::EditTopology::EditTopology(
		const status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesGui::FeatureTableModel &clicked_table_model_,	
		GPlatesGui::TopologySectionsContainer &topology_sections_container,
		GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
		GPlatesAppLogic::ApplicationState &application_state) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection(&view_state_.get_rendered_geometry_collection()),
	d_viewport_window_ptr(&viewport_window_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_topology_sections_container_ptr(&topology_sections_container),
	d_topology_tools_widget_ptr(&topology_tools_widget),
	d_feature_focus_ptr(&view_state_.get_feature_focus()),
	d_reconstruct_graph(application_state.get_reconstruct_graph())
{  }


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

	//
	// FIXME: Do this check based on feature properties rather than feature type.
	// So if something looks like a TCPB (because it has a topology polygon property)
	// then treat it like one. For this to happen we first need TopologicalNetwork to
	// use a property type different than TopologicalPolygon.
	//
	static const GPlatesModel::FeatureType plate_type =
		GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary");
	static const GPlatesModel::FeatureType network_type =
		GPlatesModel::FeatureType::create_gpml("TopologicalNetwork");
	static const GPlatesModel::FeatureType slab_type =
		GPlatesModel::FeatureType::create_gpml("TopologicalSlabBoundary");

	const GPlatesModel::FeatureType &feature_type = d_feature_focus_ptr->focused_feature()->feature_type();

	// Only activate for topologies
	if (
		feature_type != plate_type && 
		feature_type != network_type &&
		feature_type != slab_type)
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

	set_status_bar_message(QT_TR_NOOP("Click a feature to add it to a topology."));
}

void
GPlatesCanvasTools::EditTopology::handle_deactivation()
{
	d_topology_tools_widget_ptr->deactivate();
}


void
GPlatesCanvasTools::EditTopology::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	// Show the 'Clicked' Feature Table
	d_viewport_window_ptr->choose_clicked_geometry_table();

	GPlatesGui::add_clicked_geometries_to_feature_table(
			point_on_sphere,
			proximity_inclusion_threshold,
			*d_viewport_window_ptr,
			*d_clicked_table_model_ptr,
			*d_feature_focus_ptr,
			*d_rendered_geom_collection,
			d_reconstruct_graph,
			&GPlatesAppLogic::TopologyInternalUtils::include_only_reconstructed_feature_geometries);
}

