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

#include "gui/ProximityTests.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/FeaturePropertiesDialog.h"
#include "model/FeatureHandle.h"
#include "global/InternalInconsistencyException.h"


	
void
GPlatesCanvasTools::ClickGeometry::handle_activation()
{
	// FIXME: Could be pithier.
	// FIXME: May have to adjust message if we are using Map view.
	d_view_state_ptr->status_message(tr(
			"Click geometry to interact with features. Ctrl-Drag to reorient globe."));
}


void
GPlatesCanvasTools::ClickGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	// What did the user click on just now?
	std::priority_queue<GPlatesGui::ProximityTests::ProximityHit> sorted_hits;
	GPlatesGui::ProximityTests::find_close_rfgs(sorted_hits, view_state().reconstruction(),
			oriented_click_pos_on_globe, proximity_inclusion_threshold);
	
	// Give the user some useful feedback in the status bar.
	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked %1 features.").arg(sorted_hits.size()));
	} else if (sorted_hits.size() == 1) {
		d_view_state_ptr->status_message(tr("Clicked %1 feature.").arg(sorted_hits.size()));
	} else {
		d_view_state_ptr->status_message(tr("Clicked %1 features.").arg(sorted_hits.size()));
	}

	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	d_clicked_table_model_ptr->clear();

	// Obtain a FeatureHandle::weak_ref for the feature the user has clicked.
	if (sorted_hits.size() == 0) {
		// User clicked on empty space! Do not change the currently focused feature.
		emit no_hits_found();
		return;
	}
	GPlatesModel::FeatureHandle::weak_ref feature_ref(sorted_hits.top().d_feature->reference());
	if ( ! feature_ref.is_valid()) {
		throw GPlatesGlobal::InternalInconsistencyException(__FILE__, __LINE__,
				"Invalid FeatureHandle::weak_ref returned from proximity tests.");
	}
	
	// Populate the 'Clicked' FeatureTableModel.
	d_clicked_table_model_ptr->begin_insert_features(0, static_cast<int>(sorted_hits.size()) - 1);
	while ( ! sorted_hits.empty())
	{
		d_clicked_table_model_ptr->feature_sequence()->push_back(
				sorted_hits.top().d_feature->reference());
		sorted_hits.pop();
	}
	d_clicked_table_model_ptr->end_insert_features();

	// Update the focused feature.
	// Note: we do this as late as possible, so that (for instance), the 'Clicked'
	// FeatureTableModel actually has rows in it, so that it can highlight the one we just
	// clicked on. If we set the new focused feature first, the clicked table model
	// wouldn't have the right feature ref loaded and wouldn't be able to highlight
	// anything.
	d_feature_focus.set_focused_feature(feature_ref);

	emit sorted_hits_updated();
}

