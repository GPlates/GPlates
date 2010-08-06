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

#include "GLText3DNode.h"

#include "GLText2DDrawable.h"
#include "GLTransformState.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::GLText3DNode::GLText3DNode(
		const GPlatesGui::TextRenderer::ptr_to_const_type &text_renderer,
		double x,
		double y,
		double z,
		const QString &string,
		const GPlatesGui::Colour &colour,
		int x_offset,
		int y_offset,
		const QFont &font,
		float scale) :
	d_text_renderer(text_renderer),
	d_x(x),
	d_y(y),
	d_z(z),
	d_string(string),
	d_colour(colour),
	d_x_offset(x_offset),
	d_y_offset(y_offset),
	d_font(font),
	d_scale(scale)
{
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLText3DNode::get_drawable(
		const GLTransformState &transform_state) const
{
	GLdouble winX, winY, winZ;
	transform_state.glu_project(d_x, d_y, d_z, &winX, &winY, &winZ);

	// render, with offset (note that OpenGL and Qt y-axes appear to be the reverse of each other)
	const boost::optional<GLViewport> viewport = transform_state.get_current_viewport();

	// The viewport should have been set otherwise something is definitely wrong.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			viewport,
			GPLATES_ASSERTION_SOURCE);

	// Get the window coordinates at which to render text.
	const int x = static_cast<int>(winX) + d_x_offset;
	const int y = viewport->height() - (static_cast<int>(winY) + d_y_offset);

	return GLText2DDrawable::create(d_text_renderer, x, y, d_string, d_colour, d_font, d_scale);
}
