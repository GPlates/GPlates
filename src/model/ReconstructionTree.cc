/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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


void
GPlatesModel::ReconstructionTree::insert_total_reconstruction_pole(
		GpmlPlateId::non_null_ptr_type fixed_plate_,
		GpmlPlateId::non_null_ptr_type moving_plate_,
		const GPlatesMaths::FiniteRotation &pole)
{
	// FIXME:  Do we need to sanity-check that 'fixed_plate_' and 'moving_plate_' are not equal?

	// FIXME:  First, ensure a node of this (fixed plate, moving plate) does not already exist
	// inside this reconstruction tree.  Do we also need to check it for the reverse?
	GpmlPlateId::integer_plate_id_type fixed_plate_id = fixed_plate_->value();
	GpmlPlateId::integer_plate_id_type moving_plate_id = moving_plate_->value();

	// Now, let's insert the "original" pole.
	d_unused_nodes.push_front(ReconstructionTreeNode(fixed_plate_, moving_plate_,
				pole, ReconstructionTreeNode::PoleTypes::ORIGINAL));
	list_node_reference_type original_node_ref = d_unused_nodes.begin();
	d_list_nodes_by_fixed_plate_id.insert(std::make_pair(fixed_plate_id, original_node_ref));

	// Now, let's insert the "reversed" pole.
	d_unused_nodes.push_front(ReconstructionTreeNode(moving_plate_, fixed_plate_,
				::GPlatesMaths::get_reverse(pole),
				ReconstructionTreeNode::PoleTypes::REVERSED));
	list_node_reference_type reversed_node_ref = d_unused_nodes.begin();
	d_list_nodes_by_fixed_plate_id.insert(std::make_pair(moving_plate_id, reversed_node_ref));
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
	std::deque<list_node_reference_type> poles_to_be_processed;

	std::pair<map_iterator, map_iterator> root_range = find_nodes_whose_fixed_plate_id_match(root_plate_id);

	// Note that this if-statement is not strictly necessary (ie, the code would still function
	// correctly without it, iterating over empty ranges), but we might want to notify the user
	// that there were no matches, since that is presumably *not* what the user was intending.
	if (root_range.first == root_range.second) {
		// No poles found whose fixed plate ID matches the root plate ID.  Nothing to do.
		// FIXME:  Should we invoke an alert box to the user or something?
		return;
	}
	for (map_iterator root_iter = root_range.first; root_iter != root_range.second; ++root_iter) {
		poles_to_be_processed.push_back(root_iter->second);
		d_rootmost_nodes.splice(d_rootmost_nodes.end(), d_unused_nodes, root_iter->second);
	}

	while ( ! poles_to_be_processed.empty()) {
		list_node_reference_type pole_to_be_processed = poles_to_be_processed.front();
		poles_to_be_processed.pop_front();

		// We want to find all the poles which move relative to this pole (ie, all the
		// poles whose fixed plate IDs are equal to the moving plate ID of this pole).
		GpmlPlateId::integer_plate_id_type moving_plate_id_of_pole =
				pole_to_be_processed->moving_plate()->value();

		// (Also, before we worry about the tree-children of this pole, let's insert this
		// pole into the pole-by-moving-plate-ID map.)
		d_list_nodes_by_moving_plate_id.insert(
				std::make_pair(moving_plate_id_of_pole, pole_to_be_processed));

		std::pair<map_iterator, map_iterator> range =
				find_nodes_whose_fixed_plate_id_match(moving_plate_id_of_pole);

		GpmlPlateId::integer_plate_id_type fixed_plate_id_of_pole =
				pole_to_be_processed->fixed_plate()->value();
		// Each of the targets of iter is a pole which has a fixed plate ID which is equal
		// to the moving plate ID of the current pole.
		//
		// Before we do anything with each of the targets of iter, however, we will ensure
		// that it is not the reverse of the current pole.
		for (map_iterator iter = range.first; iter != range.second; ++iter) {
			if (iter->second->moving_plate()->value() == fixed_plate_id_of_pole) {
				// The target of iter is the reverse of the current pole.  Do
				// nothing with it.
				continue;
			}

			poles_to_be_processed.push_back(iter->second);
			pole_to_be_processed->tree_children().splice(
					pole_to_be_processed->tree_children().end(),
					d_unused_nodes, iter->second);

			// Finally, set the "composed absolute rotation" of the child pole.
			const GPlatesMaths::FiniteRotation &absolute_rot_of_parent =
					pole_to_be_processed->composed_absolute_rotation();
			const GPlatesMaths::FiniteRotation &relative_rot_of_child =
					iter->second->relative_rotation();

			iter->second->set_composed_absolute_rotation(
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

	std::pair<map_iterator, map_iterator> range =
			find_nodes_whose_moving_plate_id_match(plate_id_of_feature, root_plate_id);

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

	std::pair<map_iterator, map_iterator> range =
			find_nodes_whose_moving_plate_id_match(plate_id_of_feature, root_plate_id);

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


std::pair<GPlatesModel::ReconstructionTree::map_iterator, GPlatesModel::ReconstructionTree::map_iterator>
GPlatesModel::ReconstructionTree::find_nodes_whose_fixed_plate_id_match(
		GpmlPlateId::integer_plate_id_type fixed_plate_id)
{
	return d_list_nodes_by_fixed_plate_id.equal_range(fixed_plate_id);
}


std::pair<GPlatesModel::ReconstructionTree::map_iterator, GPlatesModel::ReconstructionTree::map_iterator>
GPlatesModel::ReconstructionTree::find_nodes_whose_moving_plate_id_match(
		GpmlPlateId::integer_plate_id_type moving_plate_id,
		GpmlPlateId::integer_plate_id_type root_plate_id)
{
	// Ensure the tree is built, otherwise the pole-by-moving-plate-ID map will be empty!
	if ( ! tree_is_built()) {
		build_tree(root_plate_id);
	}
	return d_list_nodes_by_moving_plate_id.equal_range(moving_plate_id);
}


void
GPlatesModel::ReconstructionTree::demolish_tree()
{
	// We *could* do this recursively, but to minimise the chance that pathological input data
	// (eg, trees which are actually linear, like lists) could kill the program, let's use a
	// FIFO instead.
	std::deque<std::pair<node_list_type *, list_node_reference_type> > poles_to_be_processed;

	// Schedule all the root-most nodes for processing.
	list_node_reference_type root_iter = d_rootmost_nodes.begin();
	list_node_reference_type root_end = d_rootmost_nodes.end();
	for ( ; root_iter != root_end; ++root_iter) {
		poles_to_be_processed.push_back(std::make_pair(&d_rootmost_nodes, root_iter));
	}

	while ( ! poles_to_be_processed.empty()) {
		std::pair<node_list_type *, list_node_reference_type> pole_to_be_processed =
				poles_to_be_processed.front();
		poles_to_be_processed.pop_front();

		// Schedule all the children of 'pole_to_be_processed.second' for processing too.
		list_node_reference_type iter = pole_to_be_processed.second->tree_children().begin();
		list_node_reference_type end = pole_to_be_processed.second->tree_children().end();
		for ( ; iter != end; ++iter) {
			poles_to_be_processed.push_back(
					std::make_pair(&(pole_to_be_processed.second->tree_children()), iter));
		}

		// Now process 'pole_to_be_processed'.
		d_unused_nodes.splice(d_unused_nodes.begin(),
				*(pole_to_be_processed.first), pole_to_be_processed.second);
	}

	// Finally, clear the pole-by-moving-plate-ID map, since this is only valid when the tree
	// is built.
	d_list_nodes_by_moving_plate_id.clear();
}
