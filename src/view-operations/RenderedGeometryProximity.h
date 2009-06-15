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

#include "RenderedGeometryCollection.h"
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
				RenderedGeometryLayer::rendered_geometry_index_type,
				const RenderedGeometryLayer *rendered_geom_layer,
				GPlatesMaths::ProximityHitDetail::non_null_ptr_type);

		RenderedGeometryLayer::rendered_geometry_index_type d_rendered_geom_index;
		const RenderedGeometryLayer *d_rendered_geom_layer;
		GPlatesMaths::ProximityHitDetail::non_null_ptr_type d_proximity_hit_detail;
	};

	/**
	 * Sequence of hit detection results (one for each @a RenderedGeometry object hit).
	 */
	typedef std::vector<RenderedGeometryProximityHit> sorted_rendered_geometry_proximity_hits_type;

	/**
	 * Performs hit detection on the @a RenderedGeometry objects in the specified
	 * @a RenderedGeometryLayer.
	 *
	 * Returns true if at least one @a RenderedGeometry object was hit in which
	 * case a sorted list of hits (closest at beginning) is returned in
	 * @a sorted_proximity_hits.
	 *
	 * Note: only tests proximity if specified @a RenderedGeometryLayer is active.
	 */
	bool
	test_proximity(
			sorted_rendered_geometry_proximity_hits_type &sorted_proximity_hits,
			const RenderedGeometryLayer &,
			const GPlatesMaths::ProximityCriteria &);

	/**
	 * Performs hit detection on the @a RenderedGeometry objects in the specified
	 * @a RenderedGeometryLayer.
	 *
	 * Returns true if at least one @a RenderedGeometry object was hit in which
	 * case a sorted list of hits (closest at beginning) is returned in
	 * @a sorted_proximity_hits.
	 *
	 * @param main_layers_to_test the list of main layers to visit.
	 * @param only_if_main_layer_active only tests proximity on
	 *        @a RenderedGeometryLayer objects that belong to active main layers.
	 *
	 * Note: only tests proximity on active @a RenderedGeometryLayer objects.
	 */
	bool
	test_proximity(
			sorted_rendered_geometry_proximity_hits_type &sorted_proximity_hits,
			const RenderedGeometryCollection &rendered_geom_collection,
			const RenderedGeometryCollection::main_layers_update_type main_layers_to_test,
			const GPlatesMaths::ProximityCriteria &,
			bool only_if_main_layer_active = true);

	/**
	 * Performs hit detection on the @a RenderedGeometry objects in the specified
	 * @a RenderedGeometryLayer.
	 *
	 * Returns true if at least one @a RenderedGeometry object was hit in which
	 * case a sorted list of hits (closest at beginning) is returned in
	 * @a sorted_proximity_hits.
	 *
	 * @param main_layer_to_test the sole main layer to visit.
	 * @param only_if_main_layer_active only tests proximity on
	 *        @a RenderedGeometryLayer objects that belong to active main layers.
	 *
	 * Note: only tests proximity on active @a RenderedGeometryLayer objects.
	 */
	bool
	test_proximity(
			sorted_rendered_geometry_proximity_hits_type &sorted_proximity_hits,
			const RenderedGeometryCollection &rendered_geom_collection,
			const RenderedGeometryCollection::MainLayerType main_layer_to_test,
			const GPlatesMaths::ProximityCriteria &proximity_criteria,
			bool only_if_main_layer_active = true);
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPROXIMITY_H
