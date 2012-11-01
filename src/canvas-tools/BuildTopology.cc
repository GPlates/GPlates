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

#include "BuildTopology.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyInternalUtils.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/GPlatesAssert.h"
#include "global/InternalInconsistencyException.h"

#include "gui/TopologyTools.h"

#include "maths/LatLonPoint.h"

#include "model/FeatureHandle.h"

#include "presentation/ViewState.h"

#include "property-values/XsString.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/SearchResultsDockWidget.h"
#include "qt-widgets/TopologyToolsWidget.h"
#include "qt-widgets/ViewportWindow.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"

#include "view-operations/RenderedGeometryProximity.h"
#include "view-operations/RenderedGeometryUtils.h"


GPlatesCanvasTools::BuildTopology::BuildTopology(
		GPlatesAppLogic::TopologyGeometry::Type build_topology_geometry_type,
		const status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesGui::FeatureTableModel &clicked_table_model_,	
		GPlatesQtWidgets::TopologyToolsWidget &topology_tools_widget,
		GPlatesAppLogic::ApplicationState &application_state) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection(&view_state_.get_rendered_geometry_collection()),
	d_viewport_window_ptr(&viewport_window_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_topology_tools_widget_ptr(&topology_tools_widget),
	d_feature_focus_ptr(&view_state_.get_feature_focus()),
	d_reconstruct_graph(application_state.get_reconstruct_graph()),
	d_build_topology_geometry_type(build_topology_geometry_type)
{  }


void
GPlatesCanvasTools::BuildTopology::handle_activation()
{
	// ONLY allow this tool to active with no focus
	if ( d_feature_focus_ptr->is_valid() )
	{
		// unset the focus
		d_feature_focus_ptr->unset_focus();
	}

	// Set up the topology sections filter based on the topology geometry type.
	switch (d_build_topology_geometry_type)
	{
	case GPlatesAppLogic::TopologyGeometry::LINE:
		d_topology_sections_filter =
				&GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_line_topological_section;
		break;

	case GPlatesAppLogic::TopologyGeometry::BOUNDARY:
		d_topology_sections_filter =
				&GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_boundary_topological_section;
		break;

	case GPlatesAppLogic::TopologyGeometry::NETWORK:
		d_topology_sections_filter =
				&GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_network_topological_section;
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	d_topology_tools_widget_ptr->activate(
			GPlatesQtWidgets::TopologyToolsWidget::BUILD,
			d_build_topology_geometry_type);

	set_status_bar_message(QT_TR_NOOP("Click a feature to add it to a topology."));
}


void
GPlatesCanvasTools::BuildTopology::handle_deactivation()
{
	d_topology_tools_widget_ptr->deactivate();

	d_topology_sections_filter = GPlatesGui::filter_reconstruction_geometry_predicate_type();
}


void
GPlatesCanvasTools::BuildTopology::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	// Show the 'Clicked' Feature Table
	d_viewport_window_ptr->search_results_dock_widget().choose_clicked_geometry_table();
	
	GPlatesGui::get_and_add_clicked_geometries_to_feature_table(
			point_on_sphere,
			proximity_inclusion_threshold,
			*d_viewport_window_ptr,
			*d_clicked_table_model_ptr,
			*d_feature_focus_ptr,
			*d_rendered_geom_collection,
			d_reconstruct_graph,
			d_topology_sections_filter);
}

void
GPlatesCanvasTools::BuildTopology::handle_left_control_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	// TODO: Make this add the first item in clicked table (ie, under mouse click) as a topological
	// section so the user doesn't have to click the 'Add' button in the task panel.
	handle_left_click(point_on_sphere, is_on_earth, proximity_inclusion_threshold);
}
