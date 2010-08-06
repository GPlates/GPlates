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

#include "GLUNurbsRendererDrawable.h"


GPlatesOpenGL::GLUNurbsCurve::GLUNurbsCurve(
		GLint num_knots,
		const boost::shared_array<GLfloat> &knots,
		GLint stride,
		const boost::shared_array<GLfloat> &ctrl_pts,
		GLint order,
		GLenum curve_type) :
	d_num_knots(num_knots),
	d_knots(knots),
	d_stride(stride),
	d_ctrl_pts(ctrl_pts),
	d_order(order),
	d_curve_type(curve_type)
{
}


void
GPlatesOpenGL::GLUNurbsCurve::draw(
		GLUnurbsObj *nurbs) const
{
	gluBeginCurve(nurbs);
	gluNurbsCurve(nurbs,
			d_num_knots, d_knots.get(), d_stride, d_ctrl_pts.get(), d_order, d_curve_type);
	gluEndCurve(nurbs);
}


void
GPlatesOpenGL::GLUNurbsRendererDrawable::bind() const
{
	gluNurbsProperty(
			d_glu_nurbs_obj.get(), GLU_SAMPLING_METHOD, d_glu_nurbs_params.sampling_method);
	gluNurbsProperty(
			d_glu_nurbs_obj.get(), GLU_SAMPLING_TOLERANCE, d_glu_nurbs_params.sampling_tolerance);
}


void
GPlatesOpenGL::GLUNurbsRendererDrawable::draw() const
{
	// The colour is here instead of in a @a GLStateSet because it's really part
	// of the vertex data. On most systems the OpenGL driver will store the colour
	// with each vertex assembled by the nurbs renderer.
	glColor3fv(d_colour);

	d_glu_nurbs_geometry->draw(d_glu_nurbs_obj.get());
}
