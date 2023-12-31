/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <algorithm>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>

#include "AddClickedGeometriesToFeatureTable.h"

#include "FeatureFocus.h"
#include "FeatureTableModel.h"

#include "app-logic/ReconstructGraph.h"

#include "presentation/ViewState.h"

#include "qt-widgets/SearchResultsDockWidget.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryProximity.h"
#include "view-operations/RenderedGeometryUtils.h"


void
GPlatesGui::get_clicked_geometries(
		GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type &clicked_geom_seq,
		const GPlatesMaths::PointOnSphere &click_point_on_sphere,
		double proximity_inclusion_threshold,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		filter_reconstruction_geometry_predicate_type filter_recon_geom_predicate)
{
	//
	// See if any interesting reconstruction geometries were clicked by the user.
	//

	// What did the user click on just now?
	GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type sorted_hits;

	// Test for proximity to the RenderedGeometry objects in the reconstruction layer.
	// These RenderedGeometry objects each contain a ReconstructionGeometry.
	// If the reconstruction main layer is inactive or parts of it are inactive (ie, child
	// layers) then they don't get tested.
	// Only what's visible gets tested which is what we want.
	GPlatesMaths::ProximityCriteria criteria(
			click_point_on_sphere, proximity_inclusion_threshold);
	GPlatesViewOperations::test_proximity(
			sorted_hits,
			rendered_geometry_collection,
			criteria);

	// Get any ReconstructionGeometry objects that are referenced by the clicked
	// RenderedGeometry objects.
	GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
			clicked_geom_seq, sorted_hits);

	// Remove those reconstruction geometries that the caller is not interested in.
	// Remove reconstruction geometry if it does not satisfy the caller's predicate.
	clicked_geom_seq.erase(
			std::remove_if(clicked_geom_seq.begin(), clicked_geom_seq.end(),
					!boost::lambda::bind(filter_recon_geom_predicate, boost::lambda::_1)),
			clicked_geom_seq.end());
}


void
GPlatesGui::add_clicked_geometries_to_feature_table(
		const GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type &clicked_geom_seq,
		GPlatesQtWidgets::ViewportWindow &view_state,
		GPlatesGui::FeatureTableModel &clicked_table_model,
		GPlatesGui::FeatureFocus &feature_focus,
		const GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		bool highlight_first_clicked_feature_in_table)
{
	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	clicked_table_model.clear();

	if (clicked_geom_seq.empty())
	{
		// None of the hits were interesting to us so clear the currently focused feature.
		feature_focus.unset_focus();
		return;
	}

	//
	// Add the interesting geometries to the feature table.
	//

	clicked_table_model.begin_insert_features(0, static_cast<int>(clicked_geom_seq.size()) - 1);

	// The sequence of ReconstructionGeometry we are going to add to.
	GPlatesGui::FeatureTableModel::geometry_sequence_type &clicked_table_recon_geom_seq =
		clicked_table_model.geometry_sequence();

	// Add the reconstruction geometries to the clicked table model.
	std::transform(
			clicked_geom_seq.begin(),
			clicked_geom_seq.end(),
			std::inserter(
					clicked_table_recon_geom_seq,
					// Add to the beginning of the current geometry sequence.
					clicked_table_recon_geom_seq.begin()),
			// Calls the FeatureTableModel::ReconstructionGeometryRow constructor with a
			// 'ReconstructionGeometry::non_null_ptr_to_const_type' and
			// 'const ReconstructGraph &' as the arguments...
			boost::lambda::bind(
					boost::lambda::constructor<FeatureTableModel::ReconstructionGeometryRow>(),
					boost::lambda::_1,
					boost::cref(reconstruct_graph)));

	clicked_table_model.end_insert_features();

	// Give the user some useful feedback in the status bar.
	if (clicked_geom_seq.size() == 1)
	{
		view_state.status_message(QObject::tr("Clicked 1 geometry."));
	}
	else
	{
		view_state.status_message(
				QObject::tr("Clicked %1 geometries.").arg(clicked_geom_seq.size()));
	}

	if (highlight_first_clicked_feature_in_table)
	{
		view_state.search_results_dock_widget().highlight_first_clicked_feature_table_row();
	}
	else
	{
		// We want to highlight the currently focused feature.
		// However it's possible the clicked geometries (just added above) are old ReconstructionGeometry
		// objects that need to be updated to the current ReconstructionGeometry's for the current
		// reconstruction time.
		// We need to update them otherwise the focused feature geometry might not be found in
		// the updated clicked table.
		clicked_table_model.handle_rendered_geometry_collection_update();

		view_state.search_results_dock_widget().highlight_focused_feature_in_table(feature_focus);
	}
}


void
GPlatesGui::add_geometry_to_top_of_feature_table(
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry_ptr,
		GPlatesGui::FeatureTableModel &clicked_table_model,
		const GPlatesAppLogic::ReconstructGraph &reconstruct_graph)
{
	// Construct the new row.
	FeatureTableModel::ReconstructionGeometryRow rg_row(reconstruction_geometry_ptr, reconstruct_graph);

	// Add it, calling Qt Model/View methods before and after to ensure everyone gets notified.
	clicked_table_model.begin_insert_features(0, 0);
	GPlatesGui::FeatureTableModel::geometry_sequence_type &geom_seq =
			clicked_table_model.geometry_sequence();
	geom_seq.insert(geom_seq.begin(), rg_row);
	clicked_table_model.end_insert_features();
}

