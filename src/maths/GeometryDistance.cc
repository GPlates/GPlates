/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "GeometryDistance.h"

#include "PolyGreatCircleArcBoundingTree.h"


namespace GPlatesMaths
{
	namespace
	{
		void
		minimum_distance_between_point_and_polyline_bounding_tree_node(
				const PointOnSphere &point,
				const PolylineOnSphere::bounding_tree_type &polyline_bounding_tree,
				const PolylineOnSphere::bounding_tree_type::node_type &polyline_sub_tree_node,
				AngularDistance &min_distance,
				AngularExtent &min_distance_threshold,
				boost::optional<UnitVector3D &> closest_position_on_polyline)
		{
			if (polyline_sub_tree_node.is_leaf_node())
			{
				// Iterate over the great circle arcs of the leaf node.
				PolylineOnSphere::bounding_tree_type::great_circle_arc_const_iterator_type
						gca_iter = polyline_sub_tree_node.get_bounded_great_circle_arcs_begin();
				PolylineOnSphere::bounding_tree_type::great_circle_arc_const_iterator_type
						gca_end = polyline_sub_tree_node.get_bounded_great_circle_arcs_end();
				for ( ; gca_iter != gca_end; ++gca_iter)
				{
					const GreatCircleArc &gca = *gca_iter;

					// Calculate minimum distance from the point to the current great circle arc.
					const AngularDistance min_distance_point_to_gca =
							minimum_distance(
									point,
									gca,
									min_distance_threshold,
									closest_position_on_polyline);

					// If shortest distance so far (within threshold)...
					if (min_distance_point_to_gca.is_precisely_less_than(min_distance))
					{
						min_distance = min_distance_point_to_gca;
						min_distance_threshold = AngularExtent::create_from_angular_distance(min_distance);
					}
				}

				return;
			}
			// else is an internal node...

			const PolylineOnSphere::bounding_tree_type::node_type child_nodes[2] =
			{
				polyline_bounding_tree.get_child_node(polyline_sub_tree_node, 0),
				polyline_bounding_tree.get_child_node(polyline_sub_tree_node, 1)
			};

			const AngularDistance child_node_min_bounding_small_circle_distances[2] =
			{
				minimum_distance(point, child_nodes[0].get_bounding_small_circle()),
				minimum_distance(point, child_nodes[1].get_bounding_small_circle())
			};

			// Visit the closest child node first since it can avoid unnecessary calculations
			// when visiting the furthest child node (because more likely to exceed the threshold).
			unsigned int child_node_visit_indices[2];
			if (child_node_min_bounding_small_circle_distances[0].is_precisely_less_than(
				child_node_min_bounding_small_circle_distances[1]))
			{
				child_node_visit_indices[0] = 0;
				child_node_visit_indices[1] = 1;
			}
			else
			{
				child_node_visit_indices[0] = 1;
				child_node_visit_indices[1] = 0;
			}

			// Iterate over the child nodes.
			for (unsigned int n = 0; n < 2; ++n)
			{
				const unsigned int child_offset = child_node_visit_indices[n];

				// If the point is further away (from the child node's bounding small circle)
				// than the current threshold then skip the current child node.
				if (child_node_min_bounding_small_circle_distances[child_offset]
					.is_precisely_greater_than(min_distance_threshold))
				{
					continue;
				}

				minimum_distance_between_point_and_polyline_bounding_tree_node(
						point,
						polyline_bounding_tree,
						child_nodes[child_offset],
						min_distance,
						min_distance_threshold,
						closest_position_on_polyline);
			}
		}
	}
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PointOnSphere &point1,
		const PointOnSphere &point2,
		boost::optional<const AngularExtent &> minimum_distance_threshold)
{
	const AngularDistance min_distance =
			AngularDistance::create_from_cosine(
					dot(point1.position_vector(), point2.position_vector()));

	// If there's a threshold and the minimum distance is greater than the threshold then
	// return the maximum possible distance (PI) to signal this.
	if (minimum_distance_threshold &&
		min_distance.is_precisely_greater_than(minimum_distance_threshold.get()))
	{
		return AngularDistance::PI;
	}

	return min_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const PointOnSphere &point,
		const PolylineOnSphere &polyline,
		boost::optional<const AngularExtent &> minimum_distance_threshold_opt,
		boost::optional<UnitVector3D &> closest_position_on_polyline)
{
	const PolylineOnSphere::bounding_tree_type &polyline_bounding_tree = polyline.get_bounding_tree();

	const PolylineOnSphere::bounding_tree_type::node_type polyline_bounding_tree_root_node =
			polyline_bounding_tree.get_root_node();

	// The (maximum possible) distance to return if shortest distance between both geometries
	// is not within the minimum distance threshold (if any).
	AngularDistance min_distance = AngularDistance::PI;

	// Note that after each minimum distance component calculation we update the threshold
	// with the updated minimum distance.
	//
	// This avoids overwriting the closest point (so far) with a point that is further away.
	//
	// This is also an optimisation that can avoid calculating the closest point in some situations
	// where the next component minimum distance is greater than the current minimum distance.
	AngularExtent min_distance_threshold = AngularExtent::PI;
	
	// If caller specified a threshold.
	if (minimum_distance_threshold_opt)
	{
		min_distance_threshold = minimum_distance_threshold_opt.get();

		// If the point is further away (from the root node's bounding small circle) than the
		// threshold then return the maximum possible distance (PI) to signal this.
		if (minimum_distance(point, polyline_bounding_tree_root_node.get_bounding_small_circle())
			.is_precisely_greater_than(min_distance_threshold))
		{
			return AngularDistance::PI;
		}
	}

	minimum_distance_between_point_and_polyline_bounding_tree_node(
			point,
			polyline_bounding_tree,
			polyline_bounding_tree_root_node,
			min_distance,
			min_distance_threshold,
			closest_position_on_polyline);

	return min_distance;
}
