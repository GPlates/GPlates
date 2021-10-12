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

#include <cmath>
#include <opengl/OpenGL.h>

#include "QGLWidgetTextRenderer.h"


namespace
{
	const qreal MIN_POINT_SIZE = 2.0;
	const int MIN_PIXEL_SIZE = 2;

	QFont
	scale_font(
			const QFont &font,
			float scale)
	{
		QFont ret = font;
		int pixel_size = font.pixelSize();
		if (pixel_size == -1)
		{
			// Size was specified in points
			qreal point_size = font.pointSizeF();
			ret.setPointSizeF((std::max)(
						MIN_POINT_SIZE,
						point_size * scale));
		}
		else
		{
			// Size was specified in pixels
			ret.setPixelSize((std::max)(
						MIN_PIXEL_SIZE,
						static_cast<int>(pixel_size * scale + 0.5f)));
		}
		return ret;
	}
}

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
		const QFont &font,
		float scale) const
{
	glColor3fv(colour);
	// need to change to GL_MODULATE for a moment otherwise the text will be rendered as white
	// GL_MODULATE is the default OpenGL state so we'll leave it that way when we're finished.
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	d_gl_widget_ptr->renderText(x, y, string, scale_font(font, scale));
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
		const QFont &font,
		float scale) const
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

	// render, with offset (note that OpenGL and Qt y-axes appear to be the reverse of each other)
	int height = view[3];
	render_text(
			static_cast<int>(winX) + x_offset, height - (static_cast<int>(winY) + y_offset),
			string,
			colour,
			scale_font(font, scale));
}

