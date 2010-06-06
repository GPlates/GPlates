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

#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryProximity.h"
#include "view-operations/RenderedGeometryUtils.h"


void
GPlatesGui::add_clicked_geometries_to_feature_table(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		double proximity_inclusion_threshold,
		const GPlatesQtWidgets::ViewportWindow &view_state,
		GPlatesGui::FeatureTableModel &clicked_table_model,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		filter_reconstruction_geometry_predicate_type filter_recon_geom_predicate)
{
	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	clicked_table_model.clear();

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
			point_on_sphere, proximity_inclusion_threshold);
	GPlatesViewOperations::test_proximity(
			sorted_hits,
			rendered_geometry_collection,
			criteria);

	// The sequence of ReconstructionGeometry's clicked by the user.
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type new_recon_geom_seq;

	// Get any ReconstructionGeometry objects that are referenced by the clicked
	// RenderedGeometry objects.
	GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
			new_recon_geom_seq, sorted_hits);

	using boost::lambda::_1;

	// Remove those reconstruction geometries that the caller is not interested in.
	// Remove reconstruction geometry if it does not satisfy the caller's predicate.
	new_recon_geom_seq.erase(
			std::remove_if(new_recon_geom_seq.begin(), new_recon_geom_seq.end(),
					!boost::lambda::bind(filter_recon_geom_predicate, _1)),
			new_recon_geom_seq.end());

	if (new_recon_geom_seq.empty())
	{
		// None of the hits were interesting to us so clear the currently focused feature.
		feature_focus.unset_focus();
		return;
	}

	//
	// Add the interesting geometries to the feature table.
	//

	clicked_table_model.begin_insert_features(
			0, static_cast<int>(new_recon_geom_seq.size()) - 1);

	// The sequence of ReconstructionGeometry's were going to add to.
	GPlatesGui::FeatureTableModel::geometry_sequence_type &clicked_table_recon_geom_seq =
		clicked_table_model.geometry_sequence();

	// Add the reconstruction geometries to the clicked table model.
	using boost::lambda::_1;
	std::transform(
			new_recon_geom_seq.begin(),
			new_recon_geom_seq.end(),
			std::inserter(
					clicked_table_recon_geom_seq,
					// Add to the beginning of the current geometry sequence.
					clicked_table_recon_geom_seq.begin()),
			// Calls the FeatureTableModel::ReconstructionGeometryRow constructor with a
			// 'ReconstructionGeometry::non_null_ptr_to_const_type' and
			// 'const ReconstructGraph &' as the arguments...
			boost::lambda::bind(
					boost::lambda::constructor<FeatureTableModel::ReconstructionGeometryRow>(),
					_1,
					boost::cref(reconstruct_graph)));

	clicked_table_model.end_insert_features();

	// Give the user some useful feedback in the status bar.
	if (new_recon_geom_seq.size() == 1)
	{
		view_state.status_message(QObject::tr("Clicked 1 geometry."));
	}
	else
	{
		view_state.status_message(
				QObject::tr("Clicked %1 geometries.").arg(new_recon_geom_seq.size()));
	}

	view_state.highlight_first_clicked_feature_table_row();
}
