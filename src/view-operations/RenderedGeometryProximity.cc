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
#include "RenderedGeometryUtils.h"
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
			return lhs.d_proximity_hit_detail->closeness() >
				rhs.d_proximity_hit_detail->closeness();
		}

		/**
		 * Sorts proximity hits by closeness.
		 */
		void
		sort_proximity_by_closeness(
				sorted_rendered_geometry_proximity_hits_type &sorted_proximity_seq)
		{
				// Sort results based on proximity closeness.
				std::sort(
						sorted_proximity_seq.begin(),
						sorted_proximity_seq.end(),
						proximity_hit_closeness_compare);
		}

		/**
		 * Tests proximity to @a RenderedGeometry objects in an active @a RenderedGeometryLayer.
		 */
		struct RenderedGeometryLayerProximity
		{
			RenderedGeometryLayerProximity(
					sorted_rendered_geometry_proximity_hits_type &sorted_proximity_seq,
					const GPlatesMaths::ProximityCriteria &proximity_criteria,
					bool test_vertices_only = false) :
			d_sorted_proximity_seq(sorted_proximity_seq),
			d_proximity_criteria(proximity_criteria),
			d_test_vertices_only(test_vertices_only)
			{  }

			void
			operator()(
					const RenderedGeometryLayer &rendered_geom_layer)
			{
				// Only visit layer if it's active.
				if (rendered_geom_layer.is_active())
				{
					// Iterate over the rendered geometries and test proximity to each.
					RenderedGeometryLayer::rendered_geometry_index_type rendered_geom_index;
					for (rendered_geom_index = 0;
						rendered_geom_index < rendered_geom_layer.get_num_rendered_geometries();
						++rendered_geom_index)
					{
						RenderedGeometry rendered_geom = rendered_geom_layer.get_rendered_geometry(
							rendered_geom_index);

						GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type hit = NULL;
						if (d_test_vertices_only)
						{
							hit =
								rendered_geom.test_vertex_proximity(d_proximity_criteria);					
						}
						else
						{
							hit =
								rendered_geom.test_proximity(d_proximity_criteria);
						}

						if (hit)
						{
							// Convert maybe_null pointer to non_null pointer.
							GPlatesMaths::ProximityHitDetail::non_null_ptr_type hit_detail(
								hit.get(),
								GPlatesUtils::NullIntrusivePointerHandler());

							// Add to list of hits.
							d_sorted_proximity_seq.push_back(
								RenderedGeometryProximityHit(
								rendered_geom_index, &rendered_geom_layer, hit_detail));
						}
					}
				}
			}

			sorted_rendered_geometry_proximity_hits_type &d_sorted_proximity_seq;
			const GPlatesMaths::ProximityCriteria &d_proximity_criteria;
			bool d_test_vertices_only;
		};
	}
}

bool
GPlatesViewOperations::test_proximity(
		sorted_rendered_geometry_proximity_hits_type &sorted_proximity_seq,
		const GPlatesMaths::ProximityCriteria &proximity_criteria,
		const RenderedGeometryLayer &rendered_geom_layer)
{
	// Setup up to do proximity tests.
	RenderedGeometryLayerProximity proximity_accumulator(
			sorted_proximity_seq, proximity_criteria);

	// Do the actual proximity tests.
	proximity_accumulator(rendered_geom_layer);

	// Sort the hit results by closeness.
	sort_proximity_by_closeness(sorted_proximity_seq);

	return !sorted_proximity_seq.empty();
}

bool
GPlatesViewOperations::test_proximity(
		sorted_rendered_geometry_proximity_hits_type &sorted_proximity_hits,
		const RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesMaths::ProximityCriteria &proximity_criteria,
		const RenderedGeometryCollection::main_layers_update_type main_layers_to_test,
		bool only_if_main_layer_active)
{
	// Setup up to do proximity tests.
	// This object will test proximity within a single RenderedGeometryLayer.
	RenderedGeometryLayerProximity proximity_accumulator(
			sorted_proximity_hits, proximity_criteria);

	// Setup to do proximity
	// This object will traverse all active RenderedGeoemtryLayers in the specified main layer of
	// RenderedGeometryCollection and call the above object on each RenderedGeometryLayer.
	RenderedGeometryUtils::ConstVisitFunctionOnRenderedGeometryLayers proximity_tester(
			boost::ref(proximity_accumulator),
			main_layers_to_test,
			only_if_main_layer_active);

	// Do the actual proximity tests.
	// This will traverse the RenderedGeometryCollection and accumulate proximity hit results
	// into 'sorted_proximity_hits'.
	proximity_tester.call_function(rendered_geom_collection);

	// Sort the hit results by closeness.
	sort_proximity_by_closeness(sorted_proximity_hits);

	return !sorted_proximity_hits.empty();
}

bool
GPlatesViewOperations::test_vertex_proximity(
	sorted_rendered_geometry_proximity_hits_type &sorted_proximity_hits,
	const RenderedGeometryCollection &rendered_geom_collection,
	const RenderedGeometryCollection::main_layers_update_type main_layers_to_test,
	const GPlatesMaths::ProximityCriteria &proximity_criteria,
	bool only_if_main_layer_active)
{
	// Setup up to do proximity tests.
	// This object will test proximity within a single RenderedGeometryLayer.
	RenderedGeometryLayerProximity proximity_accumulator(
		sorted_proximity_hits, proximity_criteria, true /* test vertices only */);

	// Setup to do proximity
	// This object will traverse all active RenderedGeoemtryLayers in the specified main layer of
	// RenderedGeometryCollection and call the above object on each RenderedGeometryLayer.
	RenderedGeometryUtils::ConstVisitFunctionOnRenderedGeometryLayers proximity_tester(
		boost::ref(proximity_accumulator),
		main_layers_to_test,
		only_if_main_layer_active);

	// Do the actual proximity tests.
	// This will traverse the RenderedGeometryCollection and accumulate proximity hit results
	// into 'sorted_proximity_hits'.
	proximity_tester.call_function(rendered_geom_collection);

	// Sort the hit results by closeness.
	sort_proximity_by_closeness(sorted_proximity_hits);

	return !sorted_proximity_hits.empty();
}

bool
GPlatesViewOperations::test_proximity(
		sorted_rendered_geometry_proximity_hits_type &sorted_proximity_hits,
		const RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesMaths::ProximityCriteria &proximity_criteria,
		RenderedGeometryCollection::MainLayerType main_layer_to_test,
		bool only_if_main_layer_active)
{
	// Only test proximity on the specified main layer.
	RenderedGeometryCollection::main_layers_update_type main_layers_to_test;
	main_layers_to_test.set(main_layer_to_test);

	return test_proximity(sorted_proximity_hits, rendered_geom_collection,
			proximity_criteria, main_layers_to_test, only_if_main_layer_active);
}

bool
GPlatesViewOperations::test_vertex_proximity(
	sorted_rendered_geometry_proximity_hits_type &sorted_proximity_hits,
	const RenderedGeometryCollection &rendered_geom_collection,
	RenderedGeometryCollection::MainLayerType main_layer_to_test,
	const GPlatesMaths::ProximityCriteria &proximity_criteria,
	bool only_if_main_layer_active)
{
	// Only test proximity on the specified main layer.
	RenderedGeometryCollection::main_layers_update_type main_layers_to_test;
	main_layers_to_test.set(main_layer_to_test);

	return test_vertex_proximity(sorted_proximity_hits, rendered_geom_collection, main_layers_to_test,
		proximity_criteria, only_if_main_layer_active);
}

GPlatesViewOperations::RenderedGeometryProximityHit::RenderedGeometryProximityHit(
		RenderedGeometryLayer::rendered_geometry_index_type rendered_geom_index,
		const RenderedGeometryLayer *rendered_geom_layer,
		GPlatesMaths::ProximityHitDetail::non_null_ptr_type proximity_hit_detail) :
d_rendered_geom_index(rendered_geom_index),
d_rendered_geom_layer(rendered_geom_layer),
d_proximity_hit_detail(proximity_hit_detail)
{
}
