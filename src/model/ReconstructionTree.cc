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

#include <iterator>  /* std::distance */
#include <deque>
#include "ReconstructionTree.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace
{
	void
	print_edge(
			const GPlatesModel::ReconstructionTree::ReconstructionTreeEdge &edge)
	{
		std::cerr << "Edge: moving "
				<< edge.moving_plate()->value()
				<< ", fixed "
				<< edge.fixed_plate()->value();
		if (edge.pole_type() == GPlatesModel::ReconstructionTree::ReconstructionTreeEdge::PoleTypes::ORIGINAL) {
			std::cerr << ", original, edge id ";
		} else {
			std::cerr << ", reversed, edge id ";
		}
		std::cerr << (&edge)
				<< ",\n finite rotation "
				<< edge.composed_absolute_rotation()
				<< std::endl;
	}


	template<typename I>
	void
	print_edges(
			I begin,
			I end)
	{
		for ( ; begin != end; ++begin) {
			::print_edge(*(begin->second));
		}
	}
}


void
GPlatesModel::ReconstructionTree::insert_total_reconstruction_pole(
		GpmlPlateId::non_null_ptr_type fixed_plate_,
		GpmlPlateId::non_null_ptr_type moving_plate_,
		const GPlatesMaths::FiniteRotation &pole)
{
	// FIXME:  Do we need to sanity-check that 'fixed_plate_' and 'moving_plate_' are not equal?

	GpmlPlateId::integer_plate_id_type fixed_plate_id = fixed_plate_->value();
	GpmlPlateId::integer_plate_id_type moving_plate_id = moving_plate_->value();

	// FIXME:  First, ensure an edge of this (fixed plate, moving plate) does not already exist
	// inside this reconstruction tree.  Do we also need to check it for the reverse?

	edge_ref_map_range_type fixed_plate_id_match_range =
			find_edges_whose_fixed_plate_id_match(fixed_plate_id);
	for (edge_ref_map_iterator iter = fixed_plate_id_match_range.first;
			iter != fixed_plate_id_match_range.second; ++iter) {
		if (iter->second->moving_plate()->value() == moving_plate_id) {
			std::cerr << "Warning: Duplicate edges detected:\n";
			ReconstructionTreeEdge new_edge(fixed_plate_, moving_plate_, pole,
					ReconstructionTreeEdge::PoleTypes::ORIGINAL);
			::print_edge(new_edge);
			std::cerr << "is for the same pole as the existing edge:\n";
			::print_edge(*(iter->second));
			std::cerr << "Won't add the new edge!\n" << std::endl;

			return;
		}
	}

	edge_ref_map_range_type moving_plate_id_match_range =
			find_edges_whose_fixed_plate_id_match(moving_plate_id);
	for (edge_ref_map_iterator iter = moving_plate_id_match_range.first;
			iter != moving_plate_id_match_range.second; ++iter) {
		if (iter->second->moving_plate()->value() == fixed_plate_id) {
			std::cerr << "Warning: Duplicate edges detected:\n";
			ReconstructionTreeEdge new_edge(fixed_plate_, moving_plate_, pole,
					ReconstructionTreeEdge::PoleTypes::ORIGINAL);
			::print_edge(new_edge);
			std::cerr << "is for the same pole as the existing edge\n";
			::print_edge(*(iter->second));
			std::cerr << "Won't add the new edge!\n" << std::endl;

			return;
		}
	}

	// Now, let's insert the "original" pole.
	d_unused_edges.push_front(ReconstructionTreeEdge(fixed_plate_, moving_plate_,
				pole, ReconstructionTreeEdge::PoleTypes::ORIGINAL));
	edge_list_iterator original_edge_ref = d_unused_edges.begin();
	d_edge_refs_by_fixed_plate_id.insert(std::make_pair(fixed_plate_id, original_edge_ref));

	// Now, let's insert the "reversed" pole.
	d_unused_edges.push_front(ReconstructionTreeEdge(moving_plate_, fixed_plate_,
				::GPlatesMaths::get_reverse(pole),
				ReconstructionTreeEdge::PoleTypes::REVERSED));
	edge_list_iterator reversed_edge_ref = d_unused_edges.begin();
	d_edge_refs_by_fixed_plate_id.insert(std::make_pair(moving_plate_id, reversed_edge_ref));
}


void
GPlatesModel::ReconstructionTree::build_tree(
		GpmlPlateId::integer_plate_id_type root_plate_id)
{
	if (tree_is_built()) {
		demolish_tree();
	}

	// We *could* do this recursively, but to minimise the chance that pathological input data
	// (eg, trees which are actually linear, like lists) could kill the program, let's use a
	// FIFO instead.
	std::deque<edge_list_iterator> edges_to_be_processed;

	edge_ref_map_range_type root_edge_range =
			find_edges_whose_fixed_plate_id_match(root_plate_id);

	// Note that this if-statement is not strictly necessary (ie, the code would still function
	// correctly without it, iterating over empty ranges), but we might want to notify the user
	// that there were no matches, since that is presumably *not* what the user was intending.
	if (root_edge_range.first == root_edge_range.second) {
		// No edges found whose fixed plate ID matches the root plate ID.  Nothing to do.
		// FIXME:  Should we invoke an alert box to the user or something?
		return;
	}
	for (edge_ref_map_iterator root_iter = root_edge_range.first; root_iter != root_edge_range.second; ++root_iter) {
		edges_to_be_processed.push_back(root_iter->second);
		d_rootmost_edges.splice(d_rootmost_edges.end(), d_unused_edges, root_iter->second);
	}

	while ( ! edges_to_be_processed.empty()) {
		edge_list_iterator edge_being_processed = edges_to_be_processed.front();
		edges_to_be_processed.pop_front();

		// We want to find all the edges which hang relative to this edge (ie, all the
		// edges whose fixed plate IDs are equal to the moving plate ID of this edge).
		GpmlPlateId::integer_plate_id_type moving_plate_id_of_edge =
				edge_being_processed->moving_plate()->value();

		// Find any and all other edges with the same moving plate ID.
		edge_ref_map_range_type moving_plate_id_edge_range =
				find_edges_whose_moving_plate_id_match(moving_plate_id_of_edge, root_plate_id);

		// Have we already processed edges which describe the moving plate ID of this edge?
		if (moving_plate_id_edge_range.first != moving_plate_id_edge_range.second) {
			// There is already an edge leading to the moving plate ID.  We don't need
			// another, and we don't *want* another.  (It could result in an infinite
			// loop.)
			// FIXME:  Should we check that the composed absolute rots are the same?
			std::cerr << "Warning: Potential cycle detected (may be a cross-over point):\n";
			::print_edge(*edge_being_processed);
			std::cerr << "forms a cycle with the existing edge(s):\n";
			::print_edges(moving_plate_id_edge_range.first, moving_plate_id_edge_range.second);
			std::cerr << "Won't process the new edge!\n" << std::endl;

			continue;
		}
		// Otherwise, let's insert this edge into the edge-by-moving-plate-ID map.
		d_edge_refs_by_moving_plate_id.insert(
				std::make_pair(moving_plate_id_of_edge, edge_being_processed));

		edge_ref_map_range_type potential_children_range =
				find_edges_whose_fixed_plate_id_match(moving_plate_id_of_edge);

		GpmlPlateId::integer_plate_id_type fixed_plate_id_of_edge =
				edge_being_processed->fixed_plate()->value();
		// Each of the targets of iter is an edge which has a fixed plate ID which is equal
		// to the moving plate ID of the current edge.
		//
		// Before we do anything with each of the targets of iter, however, we will ensure
		// that it is not the reverse of the current edge.
		for (edge_ref_map_iterator iter = potential_children_range.first;
				iter != potential_children_range.second; ++iter) {

			edge_list_iterator potential_child = iter->second;

			// First, check whether the potential child is the reverse of the current
			// edge.
			if (potential_child->moving_plate()->value() == fixed_plate_id_of_edge) {
				// The potential child is the reverse of the current edge.  Do
				// nothing with it.
				continue;
			}

			edges_to_be_processed.push_back(potential_child);
			edge_being_processed->tree_children().splice(
					edge_being_processed->tree_children().end(),
					d_unused_edges, potential_child);

			// Finally, set the "composed absolute rotation" of the child edge.
			const GPlatesMaths::FiniteRotation &absolute_rot_of_parent =
					edge_being_processed->composed_absolute_rotation();
			const GPlatesMaths::FiniteRotation &relative_rot_of_child =
					potential_child->relative_rotation();

			potential_child->set_composed_absolute_rotation(
					::GPlatesMaths::compose(absolute_rot_of_parent, relative_rot_of_child));
		}
	}
}


const boost::intrusive_ptr<GPlatesMaths::PointOnSphere>
GPlatesModel::ReconstructionTree::reconstruct_point(
		GPlatesContrib::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> p,
		GpmlPlateId::integer_plate_id_type plate_id_of_feature,
		GpmlPlateId::integer_plate_id_type root_plate_id)
{
	// If the requested plate ID is the root of the reconstruction tree, return a copy of the
	// point (as if the point were reconstructed using the identity rotation!).
	if (plate_id_of_feature == root_plate_id) {
		return get_intrusive_ptr(p->clone_on_heap());
	}

	edge_ref_map_range_type range =
			find_edges_whose_moving_plate_id_match(plate_id_of_feature, root_plate_id);

	if (range.first == range.second) {
		// No matches.  Return a NULL pointer?
		// FIXME:  Should we return a NULL pointer, or a copy of the original geometry?
		return NULL;
	}
	if (std::distance(range.first, range.second) > 1) {
		// More than one match.  Ambiguity!
		// For now, let's just use the first match anyway.
		// FIXME:  Should we verify that all alternatives are equivalent?
		// FIXME:  Should we complain to the user about this?
		const GPlatesMaths::FiniteRotation &finite_rotation =
				range.first->second->composed_absolute_rotation();

		return get_intrusive_ptr(finite_rotation * p);
	} else {
		// Exactly one match.  Ideal!
		const GPlatesMaths::FiniteRotation &finite_rotation =
				range.first->second->composed_absolute_rotation();

		return get_intrusive_ptr(finite_rotation * p);
	}
}


const boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere>
GPlatesModel::ReconstructionTree::reconstruct_polyline(
		GPlatesContrib::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> p,
		GpmlPlateId::integer_plate_id_type plate_id_of_feature,
		GpmlPlateId::integer_plate_id_type root_plate_id)
{
	// If the requested plate ID is the root of the reconstruction tree, return a copy of the
	// polyline (as if the polyline were reconstructed using the identity rotation!).
	if (plate_id_of_feature == root_plate_id) {
		return get_intrusive_ptr(p->clone_on_heap());
	}

	edge_ref_map_range_type range =
			find_edges_whose_moving_plate_id_match(plate_id_of_feature, root_plate_id);

	if (range.first == range.second) {
		// No matches.  Return a NULL pointer?
		// FIXME:  Should we return a NULL pointer, or a copy of the original geometry?
		return NULL;
	}
	if (std::distance(range.first, range.second) > 1) {
		// More than one match.  Ambiguity!
		// For now, let's just use the first match anyway.
		// FIXME:  Should we verify that all alternatives are equivalent?
		// FIXME:  Should we complain to the user about this?
		const GPlatesMaths::FiniteRotation &finite_rotation =
				range.first->second->composed_absolute_rotation();

		return get_intrusive_ptr(finite_rotation * p);
	} else {
		// Exactly one match.  Ideal!
		const GPlatesMaths::FiniteRotation &finite_rotation =
				range.first->second->composed_absolute_rotation();

		return get_intrusive_ptr(finite_rotation * p);
	}
}


GPlatesModel::ReconstructionTree::edge_ref_map_range_type
GPlatesModel::ReconstructionTree::find_edges_whose_fixed_plate_id_match(
		GpmlPlateId::integer_plate_id_type fixed_plate_id)
{
	return d_edge_refs_by_fixed_plate_id.equal_range(fixed_plate_id);
}


GPlatesModel::ReconstructionTree::edge_ref_map_range_type
GPlatesModel::ReconstructionTree::find_edges_whose_moving_plate_id_match(
		GpmlPlateId::integer_plate_id_type moving_plate_id,
		GpmlPlateId::integer_plate_id_type root_plate_id)
{
	// Ensure the tree is built, otherwise the edge-by-moving-plate-ID map will be empty!
	if ( ! tree_is_built()) {
		build_tree(root_plate_id);
	}
	return d_edge_refs_by_moving_plate_id.equal_range(moving_plate_id);
}


void
GPlatesModel::ReconstructionTree::demolish_tree()
{
	// We *could* do this recursively, but to minimise the chance that pathological input data
	// (eg, trees which are actually linear, like lists) could kill the program, let's use a
	// FIFO instead.
	std::deque<std::pair<edge_list_type *, edge_list_iterator> > edges_to_be_processed;

	// Schedule all the root-most edges for processing.
	edge_list_iterator root_iter = d_rootmost_edges.begin();
	edge_list_iterator root_end = d_rootmost_edges.end();
	for ( ; root_iter != root_end; ++root_iter) {
		edges_to_be_processed.push_back(std::make_pair(&d_rootmost_edges, root_iter));
	}

	while ( ! edges_to_be_processed.empty()) {
		std::pair<edge_list_type *, edge_list_iterator> edge_being_processed =
				edges_to_be_processed.front();
		edges_to_be_processed.pop_front();

		// Schedule all the children of 'edge_being_processed.second' for processing too.
		edge_list_iterator iter = edge_being_processed.second->tree_children().begin();
		edge_list_iterator end = edge_being_processed.second->tree_children().end();
		for ( ; iter != end; ++iter) {
			edges_to_be_processed.push_back(
					std::make_pair(&(edge_being_processed.second->tree_children()), iter));
		}

		// Now process 'edge_being_processed'.
		d_unused_edges.splice(d_unused_edges.begin(),
				*(edge_being_processed.first), edge_being_processed.second);
	}

	// Finally, clear the edge-by-moving-plate-ID map, since this is only valid when the tree
	// is built.
	d_edge_refs_by_moving_plate_id.clear();
}
