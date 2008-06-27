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
#include "maths/ProximityCriteria.h"


void
GPlatesGui::ProximityTests::find_close_rfgs(
		std::priority_queue<ProximityHit> &sorted_hits,
		GPlatesModel::Reconstruction &recon,
		const GPlatesMaths::PointOnSphere &test_point,
		const double &proximity_inclusion_threshold)
{
	using GPlatesModel::ReconstructedFeatureGeometry;

	GPlatesMaths::ProximityCriteria criteria(test_point, proximity_inclusion_threshold);

	std::vector<ReconstructedFeatureGeometry>::const_iterator iter = recon.geometries().begin();
	std::vector<ReconstructedFeatureGeometry>::const_iterator end = recon.geometries().end();
	for ( ; iter != end; ++iter) {
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type hit =
				iter->geometry()->test_proximity(criteria);
		if (hit) {
			sorted_hits.push(ProximityHit(iter->feature_ref(),
					GPlatesMaths::ProximityHitDetail::non_null_ptr_type(*hit)));
		}
	}
}
