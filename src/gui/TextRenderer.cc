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

#include <exception>
#include <QDebug>

#include "TextRenderer.h"


QFont
GPlatesGui::TextRenderer::scale_font(
		const QFont &font,
		float scale)
{
	QFont ret = font;

	const qreal min_point_size = 2.0;
	qreal point_size = QFontInfo(font).pointSizeF();
	ret.setPointSizeF((std::max)(
			min_point_size,
			point_size * scale));

	return ret;
}


GPlatesGui::TextRenderer::RenderScope::RenderScope(
		TextRenderer &text_renderer,
		GPlatesOpenGL::GLRenderer *renderer) :
	d_text_renderer(text_renderer),
	d_renderer(renderer),
	d_called_end_render(false)
{
	d_text_renderer.begin_render(d_renderer);
}


GPlatesGui::TextRenderer::RenderScope::~RenderScope()
{
	if (!d_called_end_render)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			d_text_renderer.end_render();
		}
		catch (std::exception &exc)
		{
			qWarning() << "TextRenderer::end_render: exception thrown: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "TextRenderer::end_render: exception thrown: Unknown error";
		}
	}
}


void
GPlatesGui::TextRenderer::RenderScope::end_render()
{
	if (!d_called_end_render)
	{
		d_text_renderer.end_render();
		d_called_end_render = true;
	}
}
