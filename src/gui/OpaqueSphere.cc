/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#include "OpaqueSphere.h"

#include "opengl/GLDrawable.h"
#include "opengl/GLRenderGraphDrawableNode.h"


namespace
{
	static const GLdouble RADIUS = 1.0;

	static const GLint NUM_SLICES = 72;
	static const GLint NUM_STACKS = 36;
}


GPlatesGui::OpaqueSphere::OpaqueSphere(const Colour &colour) :
	d_quad(GPlatesOpenGL::GLUQuadric::create()),
	d_colour(colour)
{

	d_quad->set_normals(GLU_NONE);
	d_quad->set_orientation(GLU_OUTSIDE);
	d_quad->set_generate_texture(GL_FALSE);
	d_quad->set_draw_style(GLU_FILL);

}


void
GPlatesGui::OpaqueSphere::paint(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node)
{
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type drawable =
			d_quad->draw_sphere(RADIUS, NUM_SLICES, NUM_STACKS, d_colour);

	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(drawable);

	render_graph_parent_node->add_child_node(drawable_node);
}
