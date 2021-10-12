/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "GLStateGraphNode.h"


GPlatesOpenGL::GLStateGraphNode::GLStateGraphNode() :
	// Root state set starts the global render group...
	d_render_group(GLStateSet::GLOBAL),
	d_state_set(GLStateSet::create(d_render_group)),
	d_parent_node(NULL),
	d_depth(0)
{
}


GPlatesOpenGL::GLStateGraphNode::GLStateGraphNode(
		const GLStateSet::non_null_ptr_to_const_type &state_set,
		unsigned int depth,
		GLStateGraphNode &parent_node) :
	d_render_group(
			// Use the state set's render group otherwise inherit from parent node...
			state_set->get_render_group()
					? state_set->get_render_group().get()
					: parent_node.get_render_group()),
	d_state_set(state_set),
	d_parent_node(&parent_node),
	d_depth(depth)
{
}


GPlatesOpenGL::GLStateGraphNode &
GPlatesOpenGL::GLStateGraphNode::get_or_create_child_node(
		const GLStateSet::non_null_ptr_to_const_type &child_state_set)
{
	if (child_state_set->get_enable_render_sub_group())
	{
		// Don't try to share a GLStateGraphNode - create a new GLStateGraphNode
		// each time this same state set comes along - this will prevent changes
		// to the draw order during rendering - because render operations that
		// reference the same 'GLStateGraphNode' will get drawn together to minimise
		// state changes - and this can change the order in which they are drawn.
		const non_null_ptr_type child_node = create(child_state_set, d_depth + 1, *this);

		// Keep our child nodes alive.
		d_child_nodes.push_back(child_node);

		return *child_node;
	}


	// To avoid looking up the map twice (ie, call 'find' and then call 'insert'
	// if not found) we'll try to insert a dummy index into the map:
	// - if it fails then it means an entry already exists and we'll return that,
	// - if it succeeds then we'll create a new node index and replace the
	//   dummy node index with it.

	const std::size_t dummy_node_index = 0;
	std::pair<state_set_map_type::iterator, bool> state_set_insert =
			d_shared_state_sets.insert(
					state_set_map_type::value_type(child_state_set, dummy_node_index));

	// The state set was successfully inserted using the key 'child_state_set'.
	if (state_set_insert.second)
	{
		// Create a child node since we didn't find one matching 'child_state_set'.
		const non_null_ptr_type child_node = create(child_state_set, d_depth + 1, *this);

		// Add to our sequence of child nodes.
		const std::size_t child_node_index = d_child_nodes.size();
		d_child_nodes.push_back(child_node);

		// Replace the dummy node index with the new child node index.
		state_set_insert.first->second = child_node_index;
	}

	// Return either the newly created child node or the existing child node.
	return *d_child_nodes[state_set_insert.first->second];
}
