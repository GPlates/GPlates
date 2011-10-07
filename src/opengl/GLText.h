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
 
#ifndef GPLATES_OPENGL_GLTEXT2DDRAWABLE_H
#define GPLATES_OPENGL_GLTEXT2DDRAWABLE_H

#include <QFont>
#include <QString>
#include <opengl/OpenGL.h>

#include "gui/Colour.h"
#include "gui/TextRenderer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	namespace GLText
	{
		/**
		 * Renders text at a 3D position.
		 *
		 * Creates text @a string at position (@a x , @a y , @a z) in world coordinates
		 * using a particular @a colour and @a font.
		 *
		 * The current model-view and projection matrices of @a renderer, along with the current
		 * viewport, are used to project from 3D world position into 2D viewport coordinates.
		 *
		 * @a x_offset and @a y_offset are the pixel coordinate shifts to offset the text from
		 * where it would otherwise be.
		 *
		 * Note that @a renderer only does the projection whereas @a text_renderer does the
		 * actual rendering of text.
		 *
		 * Also note that, because of this delegation to Qt for text rendering, the text rendering
		 * draw call cannot be queued (see the @a GLRenderer interface).
		 *
		 * TODO: Incorporate more direct rendering of text into our OpenGL framework instead of
		 * fully delegating to Qt (as @a text_renderer does). This way we have more control over
		 * the OpenGL state and better able to have depth-tested text for example (although
		 * that does conflict with anti-aliased, ie alpha-blended, text renderering).
		 */
		void
		render_text(
				GLRenderer &renderer,
				const GPlatesGui::TextRenderer &text_renderer,
				double x,
				double y,
				double z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font = QFont(),
				float scale = 1.0f);
	};
}

#endif // GPLATES_OPENGL_GLTEXT2DDRAWABLE_H
