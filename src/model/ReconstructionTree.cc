/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <iostream>
#include <iterator>  /* std::distance */
#include <deque>
#include "ReconstructionTree.h"
#include "ReconstructionTreeEdge.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace
{
	template<typename I>
	void
	output_edges(
			std::ostream &os,
			I begin,
			I end)
	{
		for ( ; begin != end; ++begin) {
			GPlatesModel::output_for_debugging(os, *(begin->second));
		}
	}
}


const GPlatesModel::ReconstructionTree::non_null_ptr_type
GPlatesModel::ReconstructionTree::create(
		ReconstructionGraph &graph,
		integer_plate_id_type root_plate_id_)
{

	//std::cerr << std::endl << "Starting new tree... " << std::endl;
	// FIXME:  This function is very, very exception-unsafe.

	ReconstructionTree::non_null_ptr_type tree(
			new ReconstructionTree(root_plate_id_, graph.reconstruction_time()),
			GPlatesUtils::NullIntrusivePointerHandler());

	// We *could* do this recursively, but to minimise the chance that pathological input data
	// (eg, trees which are actually linear, like lists) could kill the program, let's use a
	// FIFO instead.
	std::deque<edge_ref_type> edges_to_be_processed;

	edge_refs_by_plate_id_map_range_type root_edge_range =
			graph.find_edges_whose_fixed_plate_id_match(root_plate_id_);

	// Note that this if-statement is not strictly necessary (ie, the code would still function
	// correctly without it, iterating over empty ranges), but we might want to notify the user
	// that there were no matches, since that is presumably *not* what the user was intending.
	if (root_edge_range.first == root_edge_range.second) {
		// No edges found whose fixed plate ID matches the root plate ID.  Nothing to do.
		// FIXME:  Should we invoke an alert box to the user or something?
		return tree;
	}
	
	bool  reversed_edge_in_rootmost_collection = false;

	//std::cerr << "Looping over potential rootmost edges" << std::endl;
	for (edge_refs_by_plate_id_map_iterator root_iter = root_edge_range.first;
			root_iter != root_edge_range.second;
			++root_iter) {
		edge_ref_type curr_edge = root_iter->second;
		
		//output_for_debugging(std::cerr, *curr_edge);
		if (curr_edge->pole_type() == ReconstructionTreeEdge::PoleTypes::ORIGINAL)
		{
			// If the edge is original, we add it to the collection, regardless of 
			// whether or not there are reversed edges in the collection. 
			edges_to_be_processed.push_back(curr_edge);
			tree->d_rootmost_edges.push_back(curr_edge);
			curr_edge->set_parent_edge(NULL);
		} 
		else
		if (!reversed_edge_in_rootmost_collection) 
		{
			// The current edge is reversed, and we don't yet have any reversed edges,
			// so we can add the current edge. 
			edges_to_be_processed.push_back(curr_edge);
			tree->d_rootmost_edges.push_back(curr_edge);
			curr_edge->set_parent_edge(NULL);
			reversed_edge_in_rootmost_collection = true;
		}
		// Otherwise the current edge is reversed, but we already have a reversed edge,
		// so we do nothing. 
	}

	while ( ! edges_to_be_processed.empty()) {
		edge_ref_type edge_being_processed = edges_to_be_processed.front();
		edges_to_be_processed.pop_front();

#if 0
		std::cerr << std::endl;
		std::cerr << "Processing edge: ";
		output_for_debugging(std::cerr, *edge_being_processed);
#endif

		// We want to find all the edges which hang relative to this edge (ie, all the
		// edges whose fixed plate ID is equal to the moving plate ID of this edge).
		integer_plate_id_type moving_plate_id_of_edge_being_processed =
				edge_being_processed->moving_plate();

		// Find any and all other edges with the same moving plate ID.
		edge_refs_by_plate_id_map_const_range_type moving_plate_id_edge_range =
				tree->find_edges_whose_moving_plate_id_match(moving_plate_id_of_edge_being_processed);

		// Have we already processed edges whose moving plate ID is the same as the moving
		// plate ID of the edge being processed?
		if ((moving_plate_id_edge_range.first != moving_plate_id_edge_range.second) ||
			(moving_plate_id_of_edge_being_processed == root_plate_id_)){
			// There is already an edge leading to the moving plate ID.  (Actually, at
			// least one, but we'll assume that it's only one, since this block of code
			// should ensure that it's never more than one.)  We don't need another
			// edge which leads to the moving plate ID, and we don't *want* another. 
			// (It could result in an infinite loop.)
			//
			// FIXME:  Should we check that the composed absolute rots are the same?
// Silence these warnings for the release.
#if 0
			std::cerr << "Warning: Potential cycle detected (may be a cross-over point):\n";
			output_for_debugging(std::cerr, *edge_being_processed);
			std::cerr << "forms a cycle with the existing edge(s):\n";
			output_edges(std::cerr, moving_plate_id_edge_range.first, moving_plate_id_edge_range.second);
			std::cerr << "Won't process the new edge!\n" << std::endl;
#endif


// Remove the edge_being_processed from its parent's children, using the parent member
// of the ReconstructionTreeEdge.
		
			if (edge_being_processed->parent_edge())
			{
				
				edge_ref_type parent_edge = 
						GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeEdge,
								GPlatesUtils::NullIntrusivePointerHandler>(
								edge_being_processed->parent_edge(),
								GPlatesUtils::NullIntrusivePointerHandler());
				
				edge_collection_type::iterator kids;
				edge_collection_type::iterator kids_begin =
						parent_edge->children_in_built_tree().begin();
				edge_collection_type::iterator kids_end = 
						parent_edge->children_in_built_tree().end();

				kids = std::find(kids_begin,kids_end,edge_being_processed);

				if (kids == kids_end){
					// We shouldn't be here, because any edges added to the queue
					// should also have been allocated a parent, and added to the parent's
					// children-collection. 
					std::cerr << "Couldn't find edge_being_processed in the parent's children" << std::endl;
				}
				else
				{
					parent_edge->children_in_built_tree().erase(kids);
				}
			}


//////////////////////////////////////////////////////////////////////////////////////////////////
				
			continue;
		}
		// Otherwise, let's insert this edge into the edge-by-moving-plate-ID map.
		tree->d_edges_by_moving_plate_id.insert(
				std::make_pair(moving_plate_id_of_edge_being_processed, edge_being_processed));

		edge_refs_by_plate_id_map_const_range_type potential_children_range =
				graph.find_edges_whose_fixed_plate_id_match(moving_plate_id_of_edge_being_processed);

		integer_plate_id_type fixed_plate_id_of_edge_being_processed =
				edge_being_processed->fixed_plate();
		// Each of the targets of iter is an edge which has a fixed plate ID which is equal
		// to the moving plate ID of the edge being processed.
		//
		// Before we do anything with each of the targets of iter, however, we will ensure
		// that it is not the reverse of the edge being processed.

		bool reversed_edge_in_children = false;

		for (edge_refs_by_plate_id_map_const_iterator iter = potential_children_range.first;
				iter != potential_children_range.second;
				++iter) {

			//std::cerr << "Potential child: " << std::endl;
	
			edge_ref_type potential_child = iter->second;

			//output_for_debugging(std::cerr, *potential_child);

			// First, check whether the potential child is the reverse of the edge
			// being processed.
			if (potential_child->moving_plate() == fixed_plate_id_of_edge_being_processed) {
				// The potential child is the reverse of the edge being processed. 
				// Do nothing with it.
				continue;
			}

			// If the current edge is original, then we should only have original children. 
			if ((edge_being_processed->pole_type() == ReconstructionTreeEdge::PoleTypes::ORIGINAL)
				&& (potential_child->pole_type() == ReconstructionTreeEdge::PoleTypes::REVERSED)){
				continue;
			}

			// If the current edge is reversed, then we should have at most one reversed edge in the
			// children. So if the potential_child is reversed, make sure we don't already have a 
			// reversed edge in the children-collection. 
			if ((edge_being_processed->pole_type() == ReconstructionTreeEdge::PoleTypes::REVERSED)
				&& (potential_child->pole_type() == ReconstructionTreeEdge::PoleTypes::REVERSED))
			{
				if (reversed_edge_in_children){
					// We already have a reversed edge in the children collection, 
					// so do nothing with the current potential_child.
					continue;
				}
				else
				{
					reversed_edge_in_children = true;
				}
			}


			edges_to_be_processed.push_back(potential_child);
			edge_being_processed->children_in_built_tree().push_back(potential_child);
			potential_child->set_parent_edge(edge_being_processed.get());
		
			
	//		std::cerr << "Added child: ";
	//		output_for_debugging(std::cerr,*potential_child);

			// Finally, set the "composed absolute rotation" of the child edge.
			const GPlatesMaths::FiniteRotation &absolute_rot_of_parent =
					edge_being_processed->composed_absolute_rotation();
			const GPlatesMaths::FiniteRotation &relative_rot_of_child =
					potential_child->relative_rotation();

			potential_child->set_composed_absolute_rotation(
					::GPlatesMaths::compose(absolute_rot_of_parent, relative_rot_of_child));
		}
	}

	tree->d_graph.swap(graph);

	return tree;
}


const std::pair<GPlatesMaths::FiniteRotation,
		GPlatesModel::ReconstructionTree::ReconstructionCircumstance>
GPlatesModel::ReconstructionTree::get_composed_absolute_rotation(
		integer_plate_id_type moving_plate_id) const
{
	using namespace GPlatesMaths;

	// If the moving plate ID is the root of the reconstruction tree, return the identity
	// rotation.
	if (moving_plate_id == d_root_plate_id) {
		return std::make_pair(
				FiniteRotation::create(UnitQuaternion3D::create_identity_rotation()),
				ExactlyOnePlateIdMatchFound);
	}

	edge_refs_by_plate_id_map_const_range_type range =
			find_edges_whose_moving_plate_id_match(moving_plate_id);

	if (range.first == range.second) {
		// No matches.  Let's return the identity rotation and inform the client code.
		return std::make_pair(
				FiniteRotation::create(UnitQuaternion3D::create_identity_rotation()),
				NoPlateIdMatchesFound);
	}
	if (std::distance(range.first, range.second) > 1) {
		// More than one match.  Ambiguity!
		// For now, let's just use the first match anyway.
		// FIXME:  Should we verify that all alternatives are equivalent?
		// FIXME:  Should we complain to the user about this?
		const GPlatesMaths::FiniteRotation &finite_rotation =
				range.first->second->composed_absolute_rotation();

		return std::make_pair(finite_rotation, MultiplePlateIdMatchesFound);
	} else {
		// Exactly one match.  Ideal!
		const GPlatesMaths::FiniteRotation &finite_rotation =
				range.first->second->composed_absolute_rotation();

		return std::make_pair(finite_rotation, ExactlyOnePlateIdMatchFound);
	}
}


GPlatesModel::ReconstructionTree::edge_refs_by_plate_id_map_const_range_type
GPlatesModel::ReconstructionTree::find_edges_whose_moving_plate_id_match(
		integer_plate_id_type plate_id) const
{
	return d_edges_by_moving_plate_id.equal_range(plate_id);
}


GPlatesModel::ReconstructionTree::edge_refs_by_plate_id_map_range_type
GPlatesModel::ReconstructionTree::find_edges_whose_moving_plate_id_match(
		integer_plate_id_type plate_id)
{
	return d_edges_by_moving_plate_id.equal_range(plate_id);
}
