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

#include "QGLWidgetTextRenderer.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "opengl/GLRenderer.h"


GPlatesGui::QGLWidgetTextRenderer::QGLWidgetTextRenderer(
		QGLWidget *gl_widget_ptr) :
	d_gl_widget_ptr(gl_widget_ptr),
	d_renderer(NULL)
{  }



void
GPlatesGui::QGLWidgetTextRenderer::begin_render(
		GPlatesOpenGL::GLRenderer *renderer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_renderer,
			GPLATES_ASSERTION_SOURCE);

	d_renderer = renderer;
}


void
GPlatesGui::QGLWidgetTextRenderer::end_render()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_renderer,
			GPLATES_ASSERTION_SOURCE);

	d_renderer = NULL;
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
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_renderer,
			GPLATES_ASSERTION_SOURCE);

	// 'QGLWidget::renderText' is expecting the OpenGL state to be the default state so set
	// the default state and restore on scope exit.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state(*d_renderer, true/*reset_to_default_state*/);
	// This is one of the rare cases where we need to apply the OpenGL state encapsulated in
	// GLRenderer directly to OpenGL so that Qt can see it. When we're rendering exclusively using
	// GLRenderer we don't need this because the next draw call will flush the state to OpenGL for us.
	d_renderer->apply_current_state_to_opengl();

	// NOTE: We don't normally make direct calls to OpenGL (instead using 'GLRenderer') but this
	// is an exception since GLRenderer doesn't wrap per-vertex state (GLRenderer does not use
	// immediate-mode rendering - uses the more efficient vertex arrays instead).
	// So setting this won't affect GLRenderer and it's needed for 'QGLWidget::renderText'.
	glColor4fv(colour);

	d_gl_widget_ptr->renderText(x, y, string, scale_font(font, scale));
}
