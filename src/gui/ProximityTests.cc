/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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
 
#include "ProximityTests.h"


void
GPlatesGui::ProximityTests::find_close_rfgs(
		std::priority_queue<ProximityHit> &sorted_hits,
		GPlatesModel::Reconstruction &recon,
		const GPlatesMaths::PointOnSphere &test_point,
		const GPlatesMaths::real_t &proximity_inclusion_threshold)
{
	typedef GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere>
			reconstructed_point;
	typedef GPlatesModel::ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere>
			reconstructed_polyline;

	// First, test for proximity to reconstructed points.
	std::vector<reconstructed_point>::const_iterator point_iter = recon.point_geometries().begin();
	std::vector<reconstructed_point>::const_iterator point_end = recon.point_geometries().end();
	for ( ; point_iter != point_end; ++point_iter) {
		const GPlatesMaths::PointOnSphere &curr_point = *(point_iter->geometry());
		GPlatesModel::FeatureHandle::weak_ref feature_ref = point_iter->feature_ref();

		// Don't bother initialising this.
		GPlatesMaths::real_t proximity;

		if (curr_point.is_close_to(test_point, proximity_inclusion_threshold, proximity)) {
			sorted_hits.push(ProximityHit(feature_ref, proximity.dval()));
		}
	}

	// Next, test for proximity to reconstructed polylines.

	const GPlatesMaths::real_t pit_sqrd = proximity_inclusion_threshold * proximity_inclusion_threshold;
	const GPlatesMaths::real_t latitude_exclusion_threshold = sqrt(1.0 - pit_sqrd);

	std::vector<reconstructed_polyline>::const_iterator polyline_iter = recon.polyline_geometries().begin();
	std::vector<reconstructed_polyline>::const_iterator polyline_end = recon.polyline_geometries().end();
	for ( ; polyline_iter != polyline_end; ++polyline_iter) {
		const GPlatesMaths::PolylineOnSphere &curr_polyline = *(polyline_iter->geometry());
		GPlatesModel::FeatureHandle::weak_ref feature_ref = polyline_iter->feature_ref();

		// Don't bother initialising this.
		GPlatesMaths::real_t proximity;

		if (curr_polyline.is_close_to(test_point, proximity_inclusion_threshold,
				latitude_exclusion_threshold, proximity)) {
			sorted_hits.push(ProximityHit(feature_ref, proximity.dval()));
		}
	}
}
