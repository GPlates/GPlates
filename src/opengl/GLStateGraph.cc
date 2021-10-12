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

#include <vector>

#include "GLStateGraph.h"

#include "GLState.h"


GPlatesOpenGL::GLStateGraph::GLStateGraph(
		const GLStateGraphNode::non_null_ptr_to_const_type &state_graph_root_node) :
	d_root_node(state_graph_root_node),
	d_current_node(d_root_node.get())
{
}


void
GPlatesOpenGL::GLStateGraph::change_state(
		GLState &state,
		const GLStateGraphNode &destination_node_ref)
{
	const GLStateGraphNode *destination_node = &destination_node_ref;

	// If the state hasn't changed then nothing to do.
	if (destination_node == d_current_node)
	{
		return;
	}

	// NOTE: If we get here then we know that both the current and destination nodes
	// cannot be the root node.

	// If the current node and the destination node have the same parent.
	// We are testing this early because this is the most common case.
	if (destination_node->get_parent() == d_current_node->get_parent())
	{
		// Pop the state of the current node.
		state.pop_state_set();
		d_current_node = d_current_node->get_parent(); // In case an exception is thrown

		// Push the state of the destination node.
		state.push_state_set(destination_node->get_state_set());
		d_current_node = destination_node;

		return;
	}

	// If the current node is deeper in the tree then pop state sets until
	// it's at the same depth as the destination node.
	while (d_current_node->get_depth() > destination_node->get_depth())
	{
		// Pop the state of the current node.
		state.pop_state_set();
		d_current_node = d_current_node->get_parent();
	}

	// Keep track of the ancestor nodes of 'destination_node' going back until
	// a node that is the common root sub-tree of both 'source_node' and 'destination_node'.
	typedef std::vector<const GLStateGraphNode *> node_seq_type;
	node_seq_type destination_ancestor_nodes;
	// Reserve enough space to avoid reallocations (could be too much but that's ok).
	destination_ancestor_nodes.reserve(destination_node->get_depth());

	// If the destination node is deeper in the tree then push state sets until
	// it's at the same depth as the current node.
	while (destination_node->get_depth() > d_current_node->get_depth())
	{
		destination_ancestor_nodes.push_back(destination_node);
		destination_node = destination_node->get_parent();
	}

	// Both nodes are now at the same tree depth so step both nodes up the tree
	// until they meet at a sub-tree root. We have now traced a state path from the
	// current node to the destination node. This is the state change that we are
	// applying throughout this whole method.
	while (destination_node != d_current_node)
	{
		state.pop_state_set();
		d_current_node = d_current_node->get_parent();

		destination_ancestor_nodes.push_back(destination_node);
		destination_node = destination_node->get_parent();
	}

	// Iterate over the nodes from the sub-tree root down to the destination node
	// and push their state sets onto 'state'.
	for (node_seq_type::reverse_iterator ancestor_node_iter = destination_ancestor_nodes.rbegin();
		ancestor_node_iter != destination_ancestor_nodes.rend();
		++ancestor_node_iter)
	{
		const GLStateGraphNode *const ancestor_node = *ancestor_node_iter;

		state.push_state_set(ancestor_node->get_state_set());

		// Keep our current node in sync with 'state' in case exceptions are thrown.
		d_current_node = ancestor_node;
	}
}


void
GPlatesOpenGL::GLStateGraph::change_state_to_root_node(
		GLState &state)
{
	const GLStateGraphNode * const root_node = d_root_node.get();

	while (d_current_node != root_node)
	{
		state.pop_state_set();
		d_current_node = d_current_node->get_parent();
	}
}
