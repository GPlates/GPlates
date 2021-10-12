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
 
#ifndef GPLATES_OPENGL_GLSTATEGRAPHBUILDER_H
#define GPLATES_OPENGL_GLSTATEGRAPHBUILDER_H

#include <boost/noncopyable.hpp>

#include "GLStateGraph.h"
#include "GLStateSet.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Builds a graph of OpenGL state snapshots in the form of locations in a state set graph.
	 *
	 * Each node in this graph represents the full OpenGL state when a drawable is rendered.
	 */
	class GLStateGraphBuilder :
			public GPlatesUtils::ReferenceCount<GLStateGraphBuilder>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLStateGraphBuilder.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStateGraphBuilder> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLStateGraphBuilder.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStateGraphBuilder> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLStateGraphBuilder object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLStateGraphBuilder());
		}


		/**
		 * Pushes @a state_set onto the state graph and returns the new current state graph node.
		 *
		 * This effectively moves from the current state graph node to one of its children
		 * (the child associated with @a state_set).
		 *
		 * If the current state graph node has a child state graph node that is
		 * associated with @a state_set then this existing branch of the state graph is
		 * traversed, otherwise a new child state graph node is created.
		 *
		 * Returns the new current state graph node (the child node).
		 */
		GLStateGraphNode::non_null_ptr_to_const_type
		push_state_set(
				const GLStateSet::non_null_ptr_to_const_type &state_set);


		/**
		 * Pops the most recently pushed @a GLStateSet off the stack and
		 * returns the new current state graph node.
		 *
		 * This effectively moves from the current state graph node to its parent.
		 * The current state graph node is not destroyed though - it is retained so
		 * that the full state graph can be returned in @a get_state_graph.
		 *
		 * Returns the new current state graph node (the parent node).
		 *
		 * @throws PreconditionViolationError if popped too many times.
		 */
		GLStateGraphNode::non_null_ptr_to_const_type
		pop_state_set();


		/**
		 * Returns the state graph created by calls to @a push_state_set and @a pop_state_set.
		 *
		 * The returned state graph is used to effect actual OpenGL state changes.
		 */
		GLStateGraph::non_null_ptr_type
		get_state_graph();

	private:
		/**
		 * This will be the root node of the state graph being built.
		 */
		GLStateGraphNode::non_null_ptr_type d_root_node;

		/**
		 * The state graph node that represents the current OpenGL state.
		 */
		GLStateGraphNode *d_current_node;


		//! Constructor.
		GLStateGraphBuilder();
	};
}

#endif // GPLATES_OPENGL_GLSTATEGRAPHBUILDER_H
