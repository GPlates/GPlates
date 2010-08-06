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

#include <boost/foreach.hpp>

#include "GLRenderGraphInternalNode.h"
#include "GLRenderGraphVisitor.h"


void
GPlatesOpenGL::GLRenderGraphInternalNode::accept_visitor(
		ConstGLRenderGraphVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesOpenGL::GLRenderGraphInternalNode::accept_visitor(
		GLRenderGraphVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesOpenGL::GLRenderGraphInternalNode::visit_child_nodes(
		ConstGLRenderGraphVisitor &visitor) const
{
	// Visit the child nodes.
	BOOST_FOREACH(const GLRenderGraphNode::non_null_ptr_type &child_node, d_child_nodes)
	{
		child_node->accept_visitor(visitor);
	}
}


void
GPlatesOpenGL::GLRenderGraphInternalNode::visit_child_nodes(
		GLRenderGraphVisitor &visitor)
{
	// Visit the child nodes.
	BOOST_FOREACH(const GLRenderGraphNode::non_null_ptr_type &child_node, d_child_nodes)
	{
		child_node->accept_visitor(visitor);
	}
}
