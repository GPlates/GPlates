/* $Id$ */

/**
 * \file 
 * Detects proximity to rendered geometry.
 * $Revision$
 * $Date$
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

#include <algorithm>

#include "RenderedGeometryProximity.h"
#include "maths/ProximityCriteria.h"

namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Compare based on proximity closeness.
		 */
		bool
		proximity_hit_closeness_compare(
				const RenderedGeometryProximityHit &lhs,
				const RenderedGeometryProximityHit &rhs)
		{
			return lhs.d_proximity_hit_detail->closeness() <
				rhs.d_proximity_hit_detail->closeness();
		}
	}
}

bool
GPlatesViewOperations::test_proximity(
		sorted_digitisation_proximity_hits_type &sorted_proximity_seq,
		RenderedGeometryLayer &rendered_geom_layer,
		const GPlatesMaths::ProximityCriteria &proximity_criteria)
{
	// Only visit layer if it's active.
	if (rendered_geom_layer.is_active())
	{
		// Iterate over the rendered geometries and test proximity to each.
		RenderedGeometryLayer::RenderedGeometryIndex rendered_geom_index;
		for (rendered_geom_index = 0;
			rendered_geom_index < rendered_geom_layer.get_num_rendered_geometries();
			++rendered_geom_index)
		{
			RenderedGeometry rendered_geom = rendered_geom_layer.get_rendered_geometry(
				rendered_geom_index);

			GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type hit =
				rendered_geom.test_proximity(proximity_criteria);

			if (hit)
			{
				// Convert maybe_null pointer to non_null pointer.
				GPlatesMaths::ProximityHitDetail::non_null_ptr_type hit_detail(
						hit.get(),
						GPlatesUtils::NullIntrusivePointerHandler());

				// Add to list of hits.
				sorted_proximity_seq.push_back(
						RenderedGeometryProximityHit(rendered_geom_index, hit_detail));
			}
		}

		// Sort results based on proximity closeness.
		std::sort(
				sorted_proximity_seq.begin(),
				sorted_proximity_seq.end(),
				proximity_hit_closeness_compare);
	}

	return !sorted_proximity_seq.empty();
}

GPlatesViewOperations::RenderedGeometryProximityHit::RenderedGeometryProximityHit(
		RenderedGeometryLayer::RenderedGeometryIndex rendered_geom_index,
		GPlatesMaths::ProximityHitDetail::non_null_ptr_type proximity_hit_detail) :
d_rendered_geom_index(rendered_geom_index),
d_proximity_hit_detail(proximity_hit_detail)
{
}
