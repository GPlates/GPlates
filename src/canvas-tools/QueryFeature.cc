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

#include "QueryFeature.h"

#include "gui/ProximityTests.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/QueryFeaturePropertiesDialog.h"
#include "model/FeatureHandle.h"


void
GPlatesCanvasTools::QueryFeature::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	std::priority_queue<GPlatesGui::ProximityTests::ProximityHit> sorted_hits;
	GPlatesGui::ProximityTests::find_close_rfgs(sorted_hits, view_state().reconstruction(),
			oriented_click_pos_on_globe, proximity_inclusion_threshold);
	if (sorted_hits.size() == 0) {
		emit no_hits_found();
		return;
	}
	GPlatesModel::FeatureHandle::weak_ref feature_ref(sorted_hits.top().d_feature->reference());
	if ( ! feature_ref.is_valid()) {
		// FIXME:  How did this happen?  Throw an exception!
		return;  // FIXME:  Should throw an exception instead.
	}
	
	qfp_dialog().display_feature(feature_ref);


	// Update the 'Clicked' FeatureTableModel
	d_clicked_table_model_ptr->clear();
	d_clicked_table_model_ptr->begin_insert_features(0, static_cast<int>(sorted_hits.size()) - 1);
	while ( ! sorted_hits.empty())
	{
		d_clicked_table_model_ptr->feature_sequence()->push_back(
				sorted_hits.top().d_feature->reference());
		sorted_hits.pop();
	}
	d_clicked_table_model_ptr->end_insert_features();

	// FIXME:  We should re-enable this.
	emit sorted_hits_updated();
}

