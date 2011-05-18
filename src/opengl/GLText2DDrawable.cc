/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "GLText2DDrawable.h"

#include "GLTransformState.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::GLText2DDrawable::non_null_ptr_type
GPlatesOpenGL::GLText2DDrawable::create(
		const GLTransformState &transform_state,
		const GPlatesGui::TextRenderer::non_null_ptr_to_const_type &text_renderer,
		double x,
		double y,
		double z,
		const QString &string,
		const GPlatesGui::Colour &colour,
		int x_offset,
		int y_offset,
		const QFont &font,
		float scale)
{
	GLdouble winX, winY, winZ;
	transform_state.glu_project(x, y, z, &winX, &winY, &winZ);

	// render, with offset (note that OpenGL and Qt y-axes appear to be the reverse of each other)
	const boost::optional<GLViewport> viewport = transform_state.get_current_viewport();

	// The viewport should have been set otherwise something is definitely wrong.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			viewport,
			GPLATES_ASSERTION_SOURCE);

	// Get the window coordinates at which to render text.
	const int viewport_x = static_cast<int>(winX) + x_offset;
	const int viewport_y = viewport->height() - (static_cast<int>(winY) + y_offset);

	return GLText2DDrawable::create(text_renderer, viewport_x, viewport_y, string, colour, font, scale);
}
