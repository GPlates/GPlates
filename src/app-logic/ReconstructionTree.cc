/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <deque>
#include <new>

#include "ReconstructionTree.h"

#include "utils/Profile.h"


void
GPlatesAppLogic::ReconstructionTree::Edge::cache_relative_rotation() const
{
	// Get the pole samples from the graph edge.
	const ReconstructionGraph::pole_sample_list_type &pole = d_graph_edge.get_pole();
	ReconstructionGraph::pole_sample_list_type::const_iterator pole_iter = pole.begin();
	ReconstructionGraph::pole_sample_list_type::const_iterator pole_end = pole.end();

	// Iterate over the pole samples to determine where our reconstruction time lies.
	// Note that we have been guaranteed to have at least two time samples.
	ReconstructionGraph::pole_sample_list_type::const_iterator prev_pole_iter = pole_iter;
	for (++pole_iter; pole_iter != pole_end; ++pole_iter, ++prev_pole_iter)
	{
		const ReconstructionGraph::PoleSample &pole_sample = *pole_iter;

		// See if the reconstruction time is later than (ie, less far in the past than)
		// the time of the current time sample, which must mean that it lies between the previous
		// and current time samples (or is coincident with previous time sample).
		if (d_reconstruction_time_instant.is_strictly_later_than(pole_sample.get_time_instant()))
		{
			const ReconstructionGraph::PoleSample &prev_pole_sample = *prev_pole_iter;
			if (d_reconstruction_time_instant.is_coincident_with(prev_pole_sample.get_time_instant()))
			{
				// An exact match!  Hence, we can use the FiniteRotation of the previous time
				// sample directly, without need for interpolation.
				d_relative_rotation = prev_pole_sample.get_finite_rotation();

				return;
			}
			else if (pole_sample.get_time_instant().is_distant_past())
			{
				// We now allow the oldest time sample to be distant-past (+Infinity).
				//
				// Since the pole is infinitely far in the past it essentially would get ignored if we
				// interpolated between it and the previous pole (at the reconstruction time).
				// In other words the interpolation ratio would be '(t - t_prev) / (Inf - t_prev)'
				// which is zero, and so the distant-past (current) pole would get zero weighting.
				//
				// So we just use the previous pole.
				//
				// This path should only happen when ReconstructionGraph creates extra graph edges
				// that extend to the distant past, and it keeps the pole constant during this
				// extended time range, so both previous and current poles should be the same anyway.
				d_relative_rotation = prev_pole_sample.get_finite_rotation();

				return;
			}
			else if (prev_pole_sample.get_time_instant().is_distant_future())
			{
				// We now allow the youngest time sample to be distant-future (-Infinity).
				//
				// Since the previous pole is infinitely far in the future it essentially would get ignored
				// if we interpolated between it and the current pole (at the reconstruction time).
				// In other words the interpolation ratio would be '(t - -Inf) / (t_curr - -Inf)'
				// which is one, and so the distant-future (prev) pole would get zero (1.0 - 1.0 = 0.0) weighting.
				//
				// So we just use the current pole.
				//
				// It is assumed that the user is only creating a pole sample at the distant-future
				// to extend, for example, a present-day pole sample into the future.
				// In other words, the total rotation is constant from present day to the distant future.
				// If this is not the case then essentially the present-day pole sample will be extended
				// as if it was constant in the distant future.
				d_relative_rotation = pole_sample.get_finite_rotation();

				return;
			}

			const GPlatesMaths::FiniteRotation &prev_finite_rotation = prev_pole_sample.get_finite_rotation();
			const GPlatesMaths::FiniteRotation &finite_rotation = pole_sample.get_finite_rotation();

			// If either of the finite rotations has an axis hint, use it.
			boost::optional<GPlatesMaths::UnitVector3D> axis_hint;
			if (prev_finite_rotation.axis_hint())
			{
				axis_hint = prev_finite_rotation.axis_hint();
			}
			else if (finite_rotation.axis_hint())
			{
				axis_hint = finite_rotation.axis_hint();
			}

			// Interpolate between the previous and current finite rotations.
			d_relative_rotation = GPlatesMaths::interpolate(
					prev_finite_rotation,
					finite_rotation,
					prev_pole_sample.get_time_instant().value(),
					pole_sample.get_time_instant().value(),
					d_reconstruction_time_instant.value(),
					axis_hint);

			return;
		}
	}

	// The reconstruction time must coincide with the time of the last pole sample because
	// we know that the reconstruction time is contained in the inclusive time bounds of the pole.
	d_relative_rotation = pole.back().get_finite_rotation();
}


void
GPlatesAppLogic::ReconstructionTree::Edge::cache_composed_absolute_rotation() const
{
	// Compose our relative rotation with the absolute rotation of the parent edge (if there is one).
	if (d_parent_edge)
	{
		d_composed_absolute_rotation = compose(
				d_parent_edge->get_composed_absolute_rotation(),
				get_relative_rotation());
	}
	else
	{
		d_composed_absolute_rotation = get_relative_rotation();
	}
}


const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
GPlatesAppLogic::ReconstructionTree::create(
		ReconstructionGraph::non_null_ptr_to_const_type reconstruction_graph,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	PROFILE_FUNC();

	ReconstructionTree::non_null_ptr_type reconstruction_tree(
			new ReconstructionTree(
					reconstruction_graph,
					GPlatesPropertyValues::GeoTimeInstant(reconstruction_time),
					anchor_plate_id));

	// Start building the tree at the anchor plate.
	//
	// If the anchor plate does not exist then we'll get an empty tree
	// (that will always return identity rotations).
	boost::optional<const ReconstructionGraph::Plate &> anchor_plate =
			reconstruction_graph->get_plate(anchor_plate_id);
	if (anchor_plate)
	{
		reconstruction_tree->create_sub_trees_from_graph_plate(
				anchor_plate.get(),  // graph_plate
				NULL,  // parent_tree_edge
				reconstruction_tree->d_anchor_plate_edges);  // tree_edges
	}

	return reconstruction_tree;
}


void
GPlatesAppLogic::ReconstructionTree::create_sub_trees_from_graph_plate(
		const ReconstructionGraph::Plate &graph_plate,
		Edge *parent_tree_edge,
		edge_list_type &tree_edges)
{
	/*
	 * The reconstruction *graph* can contain cycles due to crossovers (when a moving plate switches
	 * fixed plates at a particular time) because, in a reconstruction graph, each edge represents
	 * a total reconstruction *sequence* (which contains a pole over a range of times).
	 *
	 * An example reconstruction graph is:
	 *
	 *                            ------0------
	 *                           /     / \     \
	 *                          1     2   3     4
	 *                         / \    |   |
	 *                        5   6   7   8
	 *                                 \ /
	 *                                  9
	 *                                /   \
	 *                              10     11
	 *                             /  \   /  \
	 *                            12  13 14  15
	 *
	 * Conversely a reconstruction *tree* represents an *acyclic* graph rooted at a chosen anchor plate.
	 *
	 * Using the above reconstruction graph, and choosing 0 for the anchor plate, and choosing the
	 * time to match the crossover time for plate 9, might result in the following reconstruction tree:
	 *
	 *                            ------0------
	 *                           /     / \     \
	 *                          1     2   3     4
	 *                         / \    |   |
	 *                        5   6   7   8
	 *                                 \
	 *                                  9
	 *                                /   \
	 *                              10     11
	 *                             /  \   /  \
	 *                            12  13 14  15
	 *
	 * ...where we've only followed one path down through to the crossover to moving plate 9
	 * (note there is no link between 8 and 9). If the crossover has been synchronised then taking
	 * either path should give the same result (ie, instead we might have had a link from 8 to 9, it just
	 * depends on which graph edge happens to come first, which depends on the order in the rotation file).
	 * Also note that in this case all tree edges are not reversed (compared to the reconstruction graph edges),
	 * in other words we've always traversed downwards from 0 in the reconstruction *graph* diagram.
	 *
	 * If we choose a *non-zero* anchor plate then we'll need to traverse upwards for some tree branches
	 * and hence will get reversed tree edges. The rule for reversed tree edges (traversing upwards)
	 * is, for each plate, we can only traverse upwards through *one* reconstruction graph edge.
	 * This avoids taking a longer path than expected to reach any plate from plate 0 (see reasons below).
	 *
	 * For example, choosing anchor plate 10 results in the following reconstruction tree:
	 *
	 *                                  --10
	 *                                 /  / \
	 *                              --9  12 13
	 *                             /   \
	 *                            7     11
	 *                            |    /  \
	 *                            2   14  15
	 *                            |
	 *                         ---0---
	 *                        /   |   \
	 *                       1    3    4
	 *                      / \   |
	 *                     5   6  8
	 *
	 * ...where edges 10->9, 9->7, 7->2 and 2->0 are reversed edges (traversed upwards in the reconstruction graph).
	 * Note that we could have traversed either edge 9->7 or edge 9->8 (if the reconstruction time is
	 * at the crossover time for plate 9) - this just depends on which graph edge happens to come first
	 * (which depends on the order in the rotation file). Note that at other reconstruction times there will only
	 * be one path (9->7 or 9->8) since only one path will have a time range (associated with total reconstruction
	 * sequence 7->9 or 8->9) that contains the reconstruction time. But at the crossover time there are two options.
	 *
	 * As for why we can only traverse one graph edge upwards per plate, note that, in the above tree,
	 * the paths from plate 0 to plates 7 and 8 are 0->2->7 and 0->3->8. If we had taken both paths upward
	 * from plate 9 (9->7 and 9->8) then it's possible (depending on how the tree is traversed, eg,
	 * depth-first versus breadth-first) that the paths from plate 0 to plates 7 and 8 could be
	 * 0->2->7 and 0->2->7->9->8. Note that the second path (from 0 to 8) is much longer than before,
	 * and this is probably not what the user (who created the rotation file) expected - they're expecting
	 * everything to take the shortest path relative to plate 0, even when the anchor plate is non-zero.
	 * Here is the relevant part of that *incorrect* tree (compared to the relevant part of a correct version):
	 *
	 *   INCORRECT        CORRECT
	 *
	 *       9               9
	 *      / \              |
	 *     7   8             7
	 *     |   |             |
	 *     2   3             2
	 *      \                |
	 *       0               0
	 *                       |
	 *                       3
	 *                       |
	 *                       8
	 * 
	 * Now if all the crossovers are synchronised, then relative rotations (relative to plate 0 or
	 * relative to the anchor plate) should not change. However some rather long paths can be
	 * taken is real scenarios and this just amplifies the possibility of an un-synchronised
	 * crossover causing problems (not to mention the long paths are unexpected when looking at
	 * the reconstruction tree plate circuit paths in the GUI dialog).
	 *
	 * One final thing, it is possible to have more than one *graph* edge between the same fixed and moving plates.
	 * This happens when a fixed/moving rotation sequence is split into two (or more) sequences
	 * (such as splitting across two rotation files, one for 0-250Ma and the other for 250-410Ma).
	 * In the following, the graph has two edges 1->5 (0-250Ma and 250-410Ma) and two edges 1->6
	 * (0-250Ma and 250-410Ma). The tree at 200Ma uses the first edge in each plate pair, whereas the
	 * tree at 400Ma uses the second edge in each plate pair:
	 *
	 *       GRAPH                      TREE (200Ma)     TREE (400Ma)
	 *                                                              
	 *        ---1---                 ---1                 1--- 
	 *       /  / \  \               /    \               /    \
	 *        5     6                 5     6           5     6 
	 *                               (0-250Ma)        (250-410Ma)
	 *
	 * ...and at the common time 250Ma, either edge in each plate pair could be used - this just depends
	 * on which graph edge happens to come first (which depends on the order in the rotation file).
	 * But only one edge per plate pair will get traversed to create a tree edge because once a tree
	 * edge has been created (to the moving plate) another tree edge cannot be created to that same
	 * (tree edge) moving plate. In other words only one *tree* edge is created for 1->5 and likewise
	 * only one for 1->6.
	 */

	//
	// For the reasons above...
	//
	// We can only traverse an incoming graph edge (upwards, in the reverse direction) if:
	//   (1) We're at the anchor plate ('parent_tree_edge == NULL'), or
	//   (2) the parent edge is in the reverse direction (ie, we're moving up the reconstruction *graph*).
	//
	// And we can only traverse up *one* incoming edge (we can't take both branches up through a crossover).
	//
	if (parent_tree_edge == NULL ||
		parent_tree_edge->is_reversed())
	{
		// Iterate over the edges going *into* the plate.
		const ReconstructionGraph::plate_incoming_edge_list_type &incoming_graph_edges = graph_plate.get_incoming_edges();
		ReconstructionGraph::plate_incoming_edge_list_type::const_iterator incoming_graph_edges_iter = incoming_graph_edges.begin();
		ReconstructionGraph::plate_incoming_edge_list_type::const_iterator incoming_graph_edges_end = incoming_graph_edges.end();
		for ( ; incoming_graph_edges_iter != incoming_graph_edges_end; ++incoming_graph_edges_iter)
		{
			const ReconstructionGraph::Edge &incoming_graph_edge = *incoming_graph_edges_iter;

			// Create a sub-tree by following the current incoming graph edge
			// in the reverse direction from its moving plate (which is 'graph_plate') to its fixed plate.
			//
			// But only if it contains the reconstruction time.
			if (create_sub_tree_from_graph_edge(
					incoming_graph_edge,
					parent_tree_edge,
					tree_edges,
					true/*reverse_tree_edge*/))
			{
				// We only create one reversed tree edge (and associated sub-tree).
				break;
			}
		}
	}

	// Iterate over the edges going *out* the plate.
	//
	// Note that we can traverse all outgoing edges.
	const ReconstructionGraph::plate_outgoing_edge_list_type &outgoing_graph_edges = graph_plate.get_outgoing_edges();
	ReconstructionGraph::plate_outgoing_edge_list_type::const_iterator outgoing_graph_edges_iter = outgoing_graph_edges.begin();
	ReconstructionGraph::plate_outgoing_edge_list_type::const_iterator outgoing_graph_edges_end = outgoing_graph_edges.end();
	for ( ; outgoing_graph_edges_iter != outgoing_graph_edges_end; ++outgoing_graph_edges_iter)
	{
		const ReconstructionGraph::Edge &outgoing_graph_edge = *outgoing_graph_edges_iter;

		// Create a sub-tree by following the current outgoing graph edge
		// in the forward direction from its fixed plate (which is 'graph_plate') to its moving plate.
		//
		// But only if it contains the reconstruction time and doesn't create a cycle in the tree.
		create_sub_tree_from_graph_edge(
				outgoing_graph_edge,
				parent_tree_edge,
				tree_edges,
				false/*reverse_tree_edge*/);
	}
}


bool
GPlatesAppLogic::ReconstructionTree::create_sub_tree_from_graph_edge(
		const ReconstructionGraph::Edge &graph_edge,
		Edge *parent_tree_edge,
		edge_list_type &tree_edges,
		bool reverse_tree_edge)
{
	// If the reconstruction time is outside the graph edge [begin,end] time range then
	// discontinue the current tree edge branch.
	if (d_reconstruction_time_instant.is_strictly_earlier_than(graph_edge.get_begin_time()) ||
		d_reconstruction_time_instant.is_strictly_later_than(graph_edge.get_end_time()))
	{
		return false;
	}

	// If the tree edge is the reverse of the graph edge (ie, if we're following the graph edge backwards)
	// then swap the fixed and moving plate associated with the tree edge.
	const ReconstructionGraph::Plate &graph_plate_of_tree_edge_fixed_plate =
			reverse_tree_edge ? graph_edge.get_moving_plate() : graph_edge.get_fixed_plate();
	const ReconstructionGraph::Plate &graph_plate_of_tree_edge_moving_plate =
			reverse_tree_edge ? graph_edge.get_fixed_plate() : graph_edge.get_moving_plate();

	const GPlatesModel::integer_plate_id_type tree_edge_fixed_plate_id =
			graph_plate_of_tree_edge_fixed_plate.get_plate_id();
	const GPlatesModel::integer_plate_id_type tree_edge_moving_plate_id =
			graph_plate_of_tree_edge_moving_plate.get_plate_id();

	// If we've looped back to the anchor (via a crossover cycle) then discontinue the current tree edge branch.
	// We avoid cycles in the tree (it's a directed *acyclic* graph).
	//
	// This also prevents reversing back along the tree edge branch (from tree edge child back to parent).
	if (tree_edge_moving_plate_id == d_anchor_plate_id)
	{
		return false;
	}

	// If there's already a tree edge with the same (moving) plate ID then we've looped back to the
	// same (moving) plate (via a crossover cycle), so discontinue the current tree edge branch.
	// We avoid cycles in the tree (it's a directed *acyclic* graph).
	//
	// This also prevents reversing back along the tree edge branch (from tree edge child back to parent).
	//
	// Attempt to insert 'tree_edge_moving_plate_id' into our map of tree edges.
	// It will only succeed if we don't already have 'tree_edge_moving_plate_id' in our map.
	std::pair<edge_map_type::iterator, bool> tree_edge_insert_result = d_all_edges.insert(
			edge_map_type::value_type(tree_edge_moving_plate_id, NULL));
	if (!tree_edge_insert_result.second)
	{
		return false;
	}

	// Successfully inserted 'tree_edge_moving_plate_id', which means it did not already exist.
	// So create a tree edge to moving plate from fixed plate.
	//
	// Note: Seems boost::object_pool<>::construct() is limited to 3 constructor arguments in its default compiled mode.
	// So, since we have more arguments, we'll allocate and then construct using placement new.
	Edge *tree_edge = d_edge_pool.malloc();
	if (tree_edge == NULL)
	{
		throw std::bad_alloc();
	}
	new (tree_edge) Edge(tree_edge_fixed_plate_id, tree_edge_moving_plate_id, d_reconstruction_time_instant, graph_edge);

	// Initialise the map item to point to the new tree edge.
	tree_edge_insert_result.first->second = tree_edge;

	// Add to the sub-tree edges.
	tree_edges.push_front(*tree_edge);

	// The parent of the current tree edge.
	tree_edge->d_parent_edge = parent_tree_edge;

	// Create a sub-tree rooted at the graph plate associated with the current tree edge's moving plate
	// since the tree is branching from its fixed plate to moving plates.
	create_sub_trees_from_graph_plate(
			graph_plate_of_tree_edge_moving_plate,  // graph_plate
			tree_edge,  // parent_tree_edge
			tree_edge->d_child_edges);  // tree_edges

	return true;
}
