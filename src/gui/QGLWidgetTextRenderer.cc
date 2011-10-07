/* $Id$ */

/**
 * @file 
 * Renders text on an OpenGL canvas using a QGLWidget
 * 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
#include <QFontInfo>

#include "QGLWidgetTextRenderer.h"


namespace
{
	const qreal MIN_POINT_SIZE = 2.0;

	QFont
	scale_font(
			const QFont &font,
			float scale)
	{
		QFont ret = font;

		qreal point_size = QFontInfo(font).pointSizeF();
		ret.setPointSizeF((std::max)(
				MIN_POINT_SIZE,
				point_size * scale));

		return ret;
	}
}


GPlatesGui::QGLWidgetTextRenderer::QGLWidgetTextRenderer(
		QGLWidget *gl_widget_ptr) :
	d_gl_widget_ptr(gl_widget_ptr)
{  }


void
GPlatesGui::QGLWidgetTextRenderer::render_text(
		int x,
		int y,
		const QString &string,
		const GPlatesGui::Colour &colour,
		const QFont &font,
		float scale) const
{
	glColor4fv(colour);
	d_gl_widget_ptr->renderText(x, y, string, scale_font(font, scale));
}
