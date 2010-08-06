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

#include "GLUQuadricDrawable.h"


GPlatesOpenGL::GLUQuadricSphere::GLUQuadricSphere(
		GLdouble radius,
		GLint num_slices,
		GLint num_stacks) :
	d_radius(radius),
	d_num_slices(num_slices),
	d_num_stacks(num_stacks)
{
}


void
GPlatesOpenGL::GLUQuadricDrawable::bind() const
{
	gluQuadricNormals(d_glu_quadric_obj.get(), d_glu_quadric_params.normals);
	gluQuadricTexture(d_glu_quadric_obj.get(), d_glu_quadric_params.texture_coords);
	gluQuadricOrientation(d_glu_quadric_obj.get(), d_glu_quadric_params.orientation);
	gluQuadricDrawStyle(d_glu_quadric_obj.get(), d_glu_quadric_params.draw_style);
}


void
GPlatesOpenGL::GLUQuadricDrawable::draw() const
{
	// The colour is here instead of in a @a GLStateSet because it's really part
	// of the vertex data. On some systems the colour will get stored with each
	// vertex assembled by the quadric.
	glColor3fv(d_colour);

	d_glu_quadric_geometry->draw(d_glu_quadric_obj.get());
}
