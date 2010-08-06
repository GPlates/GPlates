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
 
#ifndef GPLATES_OPENGL_GLSTATEGRAPHNODE_H
#define GPLATES_OPENGL_GLSTATEGRAPHNODE_H

#include <cstddef> // For std::size_t
#include <map>
#include <vector>

#include "GLStateSet.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A node in @a GLStateGraph.
	 *
	 * Each @a GLStateGraph object represents the hierarchical composition of a sequence
	 * of @a GLStateSet objects in the render graph and hence represents the full OpenGL
	 * state rather than a partial state like @a GLStateSet does.
	 */
	class GLStateGraphNode :
			public GPlatesUtils::ReferenceCount<GLStateGraphNode>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLStateGraphNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStateGraphNode> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLStateGraphNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStateGraphNode> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLStateGraphNode object that is the root of a state graph.
		 *
		 * It has no state set but it has the GLStateSet::GLOBAL render group so that
		 * any child nodes (that don't set a render group) will inherit the GLOBAL render group.
		 */
		static
		non_null_ptr_type
		create_root_node()
		{
			return non_null_ptr_type(new GLStateGraphNode());
		}


		/**
		 * Creates a @a GLStateGraphNode object.
		 */
		static
		non_null_ptr_type
		create(
				const GLStateSet::non_null_ptr_to_const_type &state_set,
				unsigned int depth,
				GLStateGraphNode &parent_node)
		{
			return non_null_ptr_type(new GLStateGraphNode(state_set, depth, parent_node));
		}


		/**
		 * Returns the state set associated with this node.
		 */
		GLStateSet::non_null_ptr_to_const_type
		get_state_set() const
		{
			return d_state_set;
		}

		/**
		 * Returns the render group assigned to the state set that 'this' node
		 * was created with or, if it doesn't have one, the render group inherited
		 * from our parent node.
		 */
		GLStateSet::render_group_type
		get_render_group() const
		{
			return d_render_group;
		}


		/**
		 * Returns the parent node.
		 *
		 * NOTE: Returned pointer is NULL if node has no parent (eg, root of state graph).
		 */
		const GLStateGraphNode *
		get_parent() const
		{
			return d_parent_node;
		}

		/**
		 * Returns the parent node.
		 *
		 * NOTE: Returned pointer is NULL if node has no parent (eg, root of state graph).
		 */
		GLStateGraphNode *
		get_parent()
		{
			return d_parent_node;
		}


		/**
		 * Returns the depth of this node in the tree (depth of zero is the root node).
		 */
		unsigned int
		get_depth() const
		{
			return d_depth;
		}


		/**
		 * Finds a child node of 'this' that matches @a child_state_set or
		 * creates a new child node if a matching one is not found.
		 *
		 * Returns child @a GLStateGraphNode.
		 */
		GLStateGraphNode &
		get_or_create_child_node(
				const GLStateSet::non_null_ptr_to_const_type &child_state_set);


	private:
		//! Typedef for a sequence of state graph nodes.
		typedef std::vector<GLStateGraphNode::non_null_ptr_type> child_node_seq_type;

		/**
		 * Typedef for associated state sets with child state graph node indices
		 * into @a child_node_seq_type.
		 */
		typedef std::map<
				GLStateSet::non_null_ptr_to_const_type,
				std::size_t> state_set_map_type;


		/**
		 * The render group assigned to @a d_state_set or, if it doesn't have one,
		 * the render group of @a d_parent_node (or if this is the root node then
		 * render group is explicitly specified in constructor).
		 */
		GLStateSet::render_group_type d_render_group;

		/**
		 * The @a GLStateSet associated with this node.
		 *
		 * The complete OpenGL state represented by this node is the composition of
		 * all the state sets of the ancestors of this node (and the state set of this node).
		 */
		GLStateSet::non_null_ptr_to_const_type d_state_set;

		GLStateGraphNode *d_parent_node;

		/**
		 * The sequence of all child @a GLStateGraphNode objects.
		 */
		child_node_seq_type d_child_nodes;

		/**
		 * A lookup of child nodes keyed by state sets.
		 *
		 * NOTE: Only the state sets that are shared are in this lookup table
		 * (ie, those state sets that don't have render sub group enabled).
		 */
		state_set_map_type d_shared_state_sets;

		/**
		 * Depth in the tree (zero is the root node).
		 */
		unsigned int d_depth;


		//! Constructor used to create a root state graph node.
		GLStateGraphNode();

		//! Constructor.
		GLStateGraphNode(
				const GLStateSet::non_null_ptr_to_const_type &state_set,
				unsigned int depth,
				GLStateGraphNode &parent_node);
	};
}

#endif // GPLATES_OPENGL_GLSTATEGRAPHNODE_H
