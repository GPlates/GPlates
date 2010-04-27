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

#include "ReconstructionGraph.h"
#include "ReconstructionTree.h"

#include "utils/ScopeGuard.h"


namespace
{
	bool
	edge_is_already_in_graph(
			const GPlatesModel::ReconstructionGraph &graph,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			const GPlatesMaths::FiniteRotation &pole)
	{
		using namespace GPlatesModel;

		// First, ensure that an edge of this same (fixed plate ID, moving plate ID) does
		// not already exist inside this graph.
		//
		// Note that it's fine if *either one* of the fixed plate ID or moving plate ID are
		// the same as an edge in the graph (same fixed plate ID is natural for a tree;
		// same moving plate ID is a cross-over point; plus, there's the fact that we're
		// also inserting edges for reversed poles), as long as *both* aren't equal.  (If
		// you think about it, how can it be a cross-over point if it's not "crossing over"
		// from one fixed plate ID to a different one?)
		ReconstructionGraph::edge_refs_by_plate_id_map_const_range_type fixed_plate_id_match_range =
				graph.find_edges_whose_fixed_plate_id_match(fixed_plate_id);
		for (ReconstructionGraph::edge_refs_by_plate_id_map_const_iterator iter =
				fixed_plate_id_match_range.first;
				iter != fixed_plate_id_match_range.second;
				++iter) {
			if (iter->second->moving_plate() == moving_plate_id) {
#if 0  // Silence these warnings for the release.
				std::cerr << "Warning: Duplicate edges detected:\n";
				ReconstructionGraph::edge_ref_type edge =
						ReconstructionTreeEdge::create(fixed_plate_id,
						moving_plate_id,
						pole,
						ReconstructionTreeEdge::PoleTypes::ORIGINAL);

				output_for_debugging(std::cerr, *edge);
				std::cerr << "is for the same pole as the existing edge:\n";
				output_for_debugging(std::cerr, *(iter->second));
				std::cerr << "Won't add the new edge!\n" << std::endl;
#endif

				return true;
			}
		}

		// Next, ensure that an edge of *reversed* (fixed plate ID, moving plate ID) does
		// not already exist inside this graph.
		ReconstructionGraph::edge_refs_by_plate_id_map_const_range_type moving_plate_id_match_range =
				graph.find_edges_whose_fixed_plate_id_match(moving_plate_id);
		for (ReconstructionGraph::edge_refs_by_plate_id_map_const_iterator iter =
				moving_plate_id_match_range.first;
				iter != moving_plate_id_match_range.second;
				++iter) {
			if (iter->second->moving_plate() == fixed_plate_id) {
#if 0  // Silence these warnings for the release.
				std::cerr << "Warning: Duplicate edges detected:\n";
				ReconstructionGraph::edge_ref_type edge =
						ReconstructionTreeEdge::create(fixed_plate_id,
						moving_plate_id,
						pole,
						ReconstructionTreeEdge::PoleTypes::ORIGINAL);

				output_for_debugging(std::cerr, *edge);
				std::cerr << "is for the same pole (reversed) as the existing edge:\n";
				output_for_debugging(std::cerr, *(iter->second));
				std::cerr << "Won't add the new edge!\n" << std::endl;
#endif

				return true;
			}
		}

		return false;
	}
}


void
GPlatesModel::ReconstructionGraph::insert_total_reconstruction_pole(
		integer_plate_id_type fixed_plate_id_,
		integer_plate_id_type moving_plate_id_,
		const GPlatesMaths::FiniteRotation &pole,
		const FeatureHandle::weak_ref &total_reconstruction_sequence_feature)
{
	// FIXME:  Confirm that 'fixed_plate_id' and 'moving_plate_id' are not equal

	if (edge_is_already_in_graph(*this, fixed_plate_id_, moving_plate_id_, pole)) {
		return;
	}

	// This is an edge for the "original" pole.
	edge_ref_type original_edge =
			ReconstructionTreeEdge::create(fixed_plate_id_, moving_plate_id_,
					pole,
					ReconstructionTreeEdge::PoleTypes::ORIGINAL);

	// This is an edge for the "reversed" pole.
	edge_ref_type reversed_edge =
			ReconstructionTreeEdge::create(moving_plate_id_, fixed_plate_id_,
					GPlatesMaths::get_reverse(pole),
					ReconstructionTreeEdge::PoleTypes::REVERSED);

	// Now, let's insert the "original" edge.
	// Remember that this function is supposed to be strongly exception-safe; ie, we should
	// provide a commit-or-rollback guarantee.  The only operations left which affect program
	// state, and can throw, are the two 'insert' invocations.  This function is strongly
	// exception-safe (that is, it either succeeds or has no effect), but since we're invoking
	// it twice, it's possible that it could fail the second time around, after the first has 
	// succeeded, leaving the program state changed.  Hence, we're going to have to be a bit
	// more careful, and clean up after ourselves if necessary.
	edge_refs_by_plate_id_map_iterator pos_of_inserted_original_edge =
			d_edges_by_fixed_plate_id.insert(std::make_pair(fixed_plate_id_, original_edge));

#ifdef WIN32
	// This is needed for WIN32 because std::map::erase returns an iterator instead of void.
	// This is what C++0x will support but it's not what C++03 expects so we have to
	// cast it.
	#if _MSC_VER <= 1400
		typedef edge_refs_by_plate_id_map_type::iterator
			(edge_refs_by_plate_id_map_type::*map_erase_func_type) (
				edge_refs_by_plate_id_map_type::iterator);
	#else
		typedef edge_refs_by_plate_id_map_type::iterator
			(edge_refs_by_plate_id_map_type::*map_erase_func_type) (
				edge_refs_by_plate_id_map_type::const_iterator);
	#endif
#else
	typedef void
		(edge_refs_by_plate_id_map_type::*map_erase_func_type) (
			edge_refs_by_plate_id_map_type::iterator);
#endif

	map_erase_func_type map_erase_func = &edge_refs_by_plate_id_map_type::erase;

	// Undo the insert if an exception is thrown.
	// Note that the 'erase' function does not throw.
	GPlatesUtils::ScopeGuard guard_insert_original = GPlatesUtils::make_guard(
			map_erase_func,
			d_edges_by_fixed_plate_id,
			pos_of_inserted_original_edge);

	// Now, let's insert the "reversed" edge.
	edge_refs_by_plate_id_map_iterator pos_of_inserted_reversed_edge =
			d_edges_by_fixed_plate_id.insert(std::make_pair(moving_plate_id_, reversed_edge));

	// Undo the insert if an exception is thrown.
	// Note that the 'erase' function does not throw.
	GPlatesUtils::ScopeGuard guard_insert_reversed = GPlatesUtils::make_guard(
			map_erase_func,
			d_edges_by_fixed_plate_id,
			pos_of_inserted_reversed_edge);

	// Keep track of the features used to generate total reconstruction poles.
	// Succeeds or has no effect if exception is thrown.
	d_reconstruction_features.push_back(total_reconstruction_sequence_feature);

	// We've made it this far so we can dismiss the undos.
	guard_insert_original.dismiss();
	guard_insert_reversed.dismiss();
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesModel::ReconstructionTree,
		GPlatesUtils::NullIntrusivePointerHandler>
GPlatesModel::ReconstructionGraph::build_tree(
		integer_plate_id_type root_plate_id)
{
	return ReconstructionTree::create(*this, root_plate_id);
}


GPlatesModel::ReconstructionGraph::edge_refs_by_plate_id_map_const_range_type
GPlatesModel::ReconstructionGraph::find_edges_whose_fixed_plate_id_match(
		integer_plate_id_type plate_id) const
{
	return d_edges_by_fixed_plate_id.equal_range(plate_id);
}


GPlatesModel::ReconstructionGraph::edge_refs_by_plate_id_map_range_type
GPlatesModel::ReconstructionGraph::find_edges_whose_fixed_plate_id_match(
		integer_plate_id_type plate_id)
{
	return d_edges_by_fixed_plate_id.equal_range(plate_id);
}
