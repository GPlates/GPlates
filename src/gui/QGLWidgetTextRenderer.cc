/* $Id$ */

/**
 * @file 
 * Renders text on an OpenGL canvas using a QGLWidget
 * 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "QGLWidgetTextRenderer.h"
#include "OpenGL.h"

GPlatesGui::QGLWidgetTextRenderer::QGLWidgetTextRenderer(
		QGLWidget *gl_widget_ptr) :
	d_gl_widget_ptr(gl_widget_ptr)
{
}

void
GPlatesGui::QGLWidgetTextRenderer::render_text(
		int x,
		int y,
		const QString &string,
		const GPlatesGui::Colour &colour,
		const QFont &font) const
{
	glColor3fv(colour);
	d_gl_widget_ptr->renderText(x, y, string, font);
}

void
GPlatesGui::QGLWidgetTextRenderer::render_text(
		double x,
		double y,
		double z,
		const QString &string,
		const GPlatesGui::Colour &colour,
		int x_offset,
		int y_offset,
		const QFont &font) const
{
	if (x_offset == 0 && y_offset == 0)
	{
		d_gl_widget_ptr->renderText(x, y, z, string, font);
	}
	else
	{
		// compute screen coordinates
		glColor3fv(colour);
		GLdouble model[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, model);
		GLdouble proj[16];
		glGetDoublev(GL_PROJECTION_MATRIX, proj);
		GLint view[4];
		glGetIntegerv(GL_VIEWPORT, view);
		GLdouble winX, winY, winZ;
		gluProject(x, y, z, model, proj, view, &winX, &winY, &winZ);
		int height = view[3];

		// render, with offset (note that OpenGL and Qt y-axes appear to be the reverse of each other)
		render_text(static_cast<int>(winX) + x_offset, height - (static_cast<int>(winY) + y_offset), string, colour, font);
	}
}

