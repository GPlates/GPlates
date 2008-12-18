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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPROXIMITY_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPROXIMITY_H

#include <utility>
#include <vector>

#include "RenderedGeometryLayer.h"
#include "maths/ProximityHitDetail.h"


namespace GPlatesMaths
{
	class ProximityCriteria;
}

namespace GPlatesViewOperations
{
	/**
	 * Results of a single proximity hit.
	 */
	struct RenderedGeometryProximityHit
	{
		RenderedGeometryProximityHit(
				RenderedGeometryLayer::RenderedGeometryIndex,
				GPlatesMaths::ProximityHitDetail::non_null_ptr_type);

		RenderedGeometryLayer::RenderedGeometryIndex d_rendered_geom_index;
		GPlatesMaths::ProximityHitDetail::non_null_ptr_type d_proximity_hit_detail;
	};

	/**
	 * Sequence of hit detection results (one for each @a RenderedGeometry object hit).
	 */
	typedef std::vector<RenderedGeometryProximityHit> sorted_digitisation_proximity_hits_type;

	/**
	 * Performs hit detection on the @a RenderedGeometry objects in the specified
	 * @a RenderedGeometryLayer.
	 *
	 * Returns true if at least one @a RenderedGeometry object was hit in which
	 * case a sorted list of hits (closest at beginning) is returned in
	 * @a sorted_proximity_hits.
	 */
	bool
	test_proximity(
			sorted_digitisation_proximity_hits_type &sorted_proximity_hits,
			RenderedGeometryLayer &,
			const GPlatesMaths::ProximityCriteria &);
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPROXIMITY_H
