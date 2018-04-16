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
	// Save the focused feature (if one is currently focused) so we can restore it when this tool is deactivated.
	// The focused feature is restored once topology building has finished because, firstly, it leaves
	// things almost the way they were (doesn't restore full clicked feature sequence though) and,
	// secondly, it allows the build topology tools to be active even if a feature is currently focused
	// (previously there had to be no feature in focus to use the build tools and this could be confusing
	// if the Choose Feature tool is not currently selected and hence the focused feature wasn't visible
	// and hence it wasn't obvious to the user what was blocking the build tools from being active).
	if (d_feature_focus_ptr->is_valid())
	{
		d_save_restore_focused_feature = d_feature_focus_ptr->focused_feature();
		d_save_restore_focused_feature_geometry_property = d_feature_focus_ptr->associated_geometry_property();
	}
	else
	{
		d_save_restore_focused_feature = GPlatesModel::FeatureHandle::weak_ref();
		d_save_restore_focused_feature_geometry_property = GPlatesModel::FeatureHandle::iterator();
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

	// Restore the focused feature, if any (saved when this tool was activated).
	if (d_save_restore_focused_feature.is_valid())
	{
		if (d_save_restore_focused_feature_geometry_property.is_still_valid())
		{
			d_feature_focus_ptr->set_focus(
					d_save_restore_focused_feature,
					d_save_restore_focused_feature_geometry_property);
		}
		else // Focused feature but geometry property no longer valid...
		{
			// Set focus to first geometry found within the feature.
			d_feature_focus_ptr->set_focus(d_save_restore_focused_feature);
		}
	}
	else // No focused feature...
	{
		d_feature_focus_ptr->unset_focus();
	}

	// Populate the feature table so that the Clicked (Geometries) GUI table shows the focused feature.
	// NOTE: We do this *after* focusing the feature so that it can be found in the updated clicked feature table.
	if (d_feature_focus_ptr->associated_reconstruction_geometry())
	{
		GPlatesGui::add_clicked_geometries_to_feature_table(
				std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>(
						1,
						d_feature_focus_ptr->associated_reconstruction_geometry().get()),
				*d_viewport_window_ptr,
				*d_clicked_table_model_ptr,
				*d_feature_focus_ptr,
				d_reconstruct_graph,
				false/*highlight_first_clicked_feature_in_table*/);
	}
	else
	{
		d_clicked_table_model_ptr->clear();
	}
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
