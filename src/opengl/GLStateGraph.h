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
 
#ifndef GPLATES_OPENGL_GLSTATEGRAPH_H
#define GPLATES_OPENGL_GLSTATEGRAPH_H

#include "GLStateGraphNode.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLState;

	/**
	 * A graph of OpenGL state snapshots in the form of locations in a state set graph.
	 *
	 * Each node in this graph represents the full OpenGL state when a drawable is rendered.
	 */
	class GLStateGraph :
			public GPlatesUtils::ReferenceCount<GLStateGraph>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLStateGraph.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStateGraph> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLStateGraph.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStateGraph> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLStateGraph object.
		 */
		static
		non_null_ptr_type
		create(
				const GLStateGraphNode::non_null_ptr_to_const_type &state_graph_root_node)
		{
			return non_null_ptr_type(new GLStateGraph(state_graph_root_node));
		}


		/**
		 * Returns the root state graph node.
		 */
		GLStateGraphNode::non_null_ptr_to_const_type
		get_root_state_graph_node() const
		{
			return d_root_node;
		}


		/**
		 * Changes from the current OpenGL state to @a destination_state.
		 */
		void
		change_state(
				GLState &state,
				const GLStateGraphNode &destination_state);


		/**
		 * Changes from the current OpenGL state to that of the root node.
		 *
		 * This effectively restores @a state to where it was before we used it.
		 */
		void
		change_state_to_root_node(
				GLState &state);

	private:
		GLStateGraphNode::non_null_ptr_to_const_type d_root_node;

		/**
		 * The state graph node that represents the current OpenGL state.
		 */
		const GLStateGraphNode *d_current_node;


		/**
		 * Constructor.
		 *
		 * Calls to @a change_state to modify the OpenGL state sent directly
		 * to OpenGL using @a state.
		 */
		explicit
		GLStateGraph(
				const GLStateGraphNode::non_null_ptr_to_const_type &state_graph_root_node);
	};
}

#endif // GPLATES_OPENGL_GLSTATEGRAPH_H
