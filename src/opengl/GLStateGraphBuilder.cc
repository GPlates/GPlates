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

#include "GLStateGraphBuilder.h"

#include "GLStateGraphNode.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLStateGraphBuilder::GLStateGraphBuilder() :
	d_root_node(GLStateGraphNode::create_root_node()),
	d_current_node(d_root_node.get())
{
}


GPlatesOpenGL::GLStateGraphNode::non_null_ptr_to_const_type
GPlatesOpenGL::GLStateGraphBuilder::push_state_set(
		const GLStateSet::non_null_ptr_to_const_type &state_set)
{
	d_current_node = &d_current_node->get_or_create_child_node(state_set);

	return GPlatesUtils::get_non_null_pointer<const GLStateGraphNode>(d_current_node);
}


GPlatesOpenGL::GLStateGraphNode::non_null_ptr_to_const_type
GPlatesOpenGL::GLStateGraphBuilder::pop_state_set()
{
	d_current_node = d_current_node->get_parent();

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_node != NULL,
			GPLATES_ASSERTION_SOURCE);

	return GPlatesUtils::get_non_null_pointer<const GLStateGraphNode>(d_current_node);
}


GPlatesOpenGL::GLStateGraph::non_null_ptr_type
GPlatesOpenGL::GLStateGraphBuilder::get_state_graph()
{
	return GLStateGraph::create(d_root_node);
}
