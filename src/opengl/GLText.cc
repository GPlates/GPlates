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

#include "GLText.h"

#include "GLProjectionUtils.h"
#include "GLRenderer.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


void
GPlatesOpenGL::GLText::render_text_2D(
		GLRenderer &renderer,
		const GPlatesGui::TextRenderer &text_renderer,
		const double &world_x,
		const double &world_y,
		const QString &string,
		const GPlatesGui::Colour &colour,
		int x_offset,
		int y_offset,
		const QFont &font,
		float scale)
{
	render_text_3D(renderer, text_renderer, world_x, world_y, 0.0, string, colour, x_offset, y_offset, font, scale);
}


void
GPlatesOpenGL::GLText::render_text_3D(
		GLRenderer &renderer,
		const GPlatesGui::TextRenderer &text_renderer,
		double world_x,
		double world_y,
		double world_z,
		const QString &string,
		const GPlatesGui::Colour &colour,
		int x_offset,
		int y_offset,
		const QFont &font,
		float scale)
{
	const GLViewport &viewport = renderer.gl_get_viewport();
	const GLMatrix &model_view_transform = renderer.gl_get_matrix(GL_MODELVIEW);
	const GLMatrix &projection_transform = renderer.gl_get_matrix(GL_PROJECTION);

	GLdouble win_x, win_y, win_z;
	GLProjectionUtils::glu_project(
			viewport, model_view_transform, projection_transform,
			world_x, world_y, world_z,
			&win_x, &win_y, &win_z);

	// Get the window coordinates at which to render text.
	const int qt_win_x = static_cast<int>(win_x) + x_offset;
	// Note that OpenGL and Qt y-axes are the reverse of each other.
	const int qt_win_y = viewport.height() - (static_cast<int>(win_y) + y_offset);

	// Delegate to Qt to do the actual rendering of text.
	text_renderer.render_text(qt_win_x, qt_win_y, string, colour, font, scale);
}
