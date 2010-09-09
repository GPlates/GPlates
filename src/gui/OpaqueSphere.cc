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
#include "opengl/GLTransform.h"


namespace
{
	static const GLdouble RADIUS = 1.0;
	static const GLdouble INNER_RADIUS = 0.0;

	static const GLint NUM_SLICES = 72;
//	static const GLint NUM_STACKS = 36;
	static const GLint NUM_LOOPS = 1;
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
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
		const GPlatesMaths::UnitVector3D &axis,
		double angle_in_deg)
{
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type drawable =
			d_quad->draw_disk(INNER_RADIUS, RADIUS, NUM_SLICES, NUM_LOOPS, d_colour);
	// Note: This used to be
	//     d_quad->draw_sphere(RADIUS, NUM_SLICES, NUM_STACKS, d_colour)
	// But note that the globe appears as a perfect circle on the screen with a
	// flat colour, so we might as well just draw a much simpler disk with that
	// colour. Drawing a disk has one other advantage over actually drawing a
	// sphere in our case: if we write into the depth buffer while drawing the
	// disk, we can use a depth test to occlude the geometries on the far side of
	// the globe, but since the disk cuts through the centre of the globe, we
	// avoid any artifacts due to geometries dipping in and out of the surface of
	// the globe.

	GPlatesOpenGL::GLRenderGraphDrawableNode::non_null_ptr_type drawable_node =
			GPlatesOpenGL::GLRenderGraphDrawableNode::create(drawable);

	GPlatesOpenGL::GLTransform::non_null_ptr_type t =
			GPlatesOpenGL::GLTransform::create(GL_MODELVIEW);
	// Undo the rotation done in Globe so that the disk always faces the camera.
	t->get_matrix().gl_rotate(-angle_in_deg, axis.x().dval(), axis.y().dval(), axis.z().dval());
	// Rotate the axes so that the z-axis is perpendicular to the screen.
	// This is because gluDisk draws the disk on the z = 0 plane.
	t->get_matrix().gl_rotate(90, 0.0, 1.0, 0.0);
	drawable_node->set_transform(t);

	render_graph_parent_node->add_child_node(drawable_node);
}

