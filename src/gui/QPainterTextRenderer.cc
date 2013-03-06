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

#include <cmath>
#include <QPainter>
#include <opengl/OpenGL.h>

#include "QPainterTextRenderer.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "opengl/GLRenderer.h"


void
GPlatesGui::QPainterTextRenderer::begin_render(
		GPlatesOpenGL::GLRenderer *renderer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_renderer,
			GPLATES_ASSERTION_SOURCE);

	d_renderer = renderer;
}


void
GPlatesGui::QPainterTextRenderer::end_render()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_renderer,
			GPLATES_ASSERTION_SOURCE);

	d_renderer = NULL;
}


void
GPlatesGui::QPainterTextRenderer::render_text(
		int x,
		int y,
		const QString &string,
		const GPlatesGui::Colour &colour,
		const QFont &font,
		float scale) const
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_renderer,
			GPLATES_ASSERTION_SOURCE);

	// Suspend rendering with 'GLRenderer' so we can resume painting with 'QPainter'.
	// At scope exit we can resume rendering with 'GLRenderer'.
	//
	// We do this because the QPainter's paint engine might be OpenGL and we need to make sure
	// it's OpenGL state does not interfere with the OpenGL state of 'GLRenderer' and vice versa.
	// This also provides a means to retrieve the QPainter for rendering text.
	GPlatesOpenGL::GLRenderer::QPainterBlockScope qpainter_block_scope(*d_renderer);

	boost::optional<QPainter &> qpainter = qpainter_block_scope.get_qpainter();

	// We need a QPainter - one should have been specified to 'GLRenderer::begin_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			qpainter,
			GPLATES_ASSERTION_SOURCE);

	// Get the current painter transform, font and colour.
	const QTransform prev_world_transform = qpainter->worldTransform();
	const QPen prev_color = qpainter->pen();
	const QFont prev_font = qpainter->font();

	// Set the identity world transform since our input position is specified in *window* coordinates
	// and we don't want it transformed by the current world transform.
	qpainter->setWorldTransform(QTransform()/*identity*/);

	// Set the font and colour.
	qpainter->setPen(colour);
	qpainter->setFont(scale_font(font, scale));

	qpainter->drawText(x, y, string);

	// Restore the previous world transform, font and colour in the QPainter.
	qpainter->setWorldTransform(prev_world_transform);
	qpainter->setPen(prev_color);
	qpainter->setFont(prev_font);

	// At scope exit we can resume rendering with 'GLRenderer'...
}
