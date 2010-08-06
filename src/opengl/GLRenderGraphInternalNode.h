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
 
#ifndef GPLATES_OPENGL_GLRENDERGRAPHINTERNALNODE_H
#define GPLATES_OPENGL_GLRENDERGRAPHINTERNALNODE_H

#include <vector>

#include "GLRenderGraphNode.h"


namespace GPlatesOpenGL
{
	/**
	 * A @a GLRenderGraphNode derivation that contains child nodes.
	 */
	class GLRenderGraphInternalNode :
			public GLRenderGraphNode
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderGraphInternalNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderGraphInternalNode> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderGraphInternalNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderGraphInternalNode> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderGraphInternalNode object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLRenderGraphInternalNode());
		}


		void
		add_child_node(
				const GLRenderGraphNode::non_null_ptr_type &child_node)
		{
			d_child_nodes.push_back(child_node);
		}


		/**
		 * Accept a ConstGLRenderGraphVisitor instance.
		 *
		 * NOTE: This does not traverse the child nodes - traversal is the responsibility
		 * of the visitor (it can traverse child nodes using @a visit_child_nodes).
		 * This is done so the visitor can pre process this node, then visit its child nodes,
		 * and then post process this node.
		 */
		virtual
		void
		accept_visitor(
				ConstGLRenderGraphVisitor &visitor) const;


		/**
		 * Accept a GLRenderGraphVisitor instance.
		 *
		 * NOTE: This does not traverse the child nodes - traversal is the responsibility
		 * of the visitor (it can traverse child nodes using @a visit_child_nodes).
		 * This is done so the visitor can pre process this node, then visit its child nodes,
		 * and then post process this node.
		 */
		virtual
		void
		accept_visitor(
				GLRenderGraphVisitor &visitor);


		/**
		 * Utility method (for visitors) that calls 'accept_visitor()' on all child nodes.
		 */
		void
		visit_child_nodes(
				ConstGLRenderGraphVisitor &visitor) const;


		/**
		 * Utility method (for visitors) that calls 'accept_visitor()' on all child nodes.
		 */
		void
		visit_child_nodes(
				GLRenderGraphVisitor &visitor);

	protected:
		GLRenderGraphInternalNode()
		{  }

	private:
		//! Typedef for a sequence of child nodes.
		typedef std::vector<GLRenderGraphNode::non_null_ptr_type> child_node_seq_type;

		child_node_seq_type d_child_nodes;
	};
}

#endif // GPLATES_OPENGL_GLRENDERGRAPHINTERNALNODE_H
