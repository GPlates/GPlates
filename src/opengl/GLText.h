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


namespace GPlatesOpenGL
{
	class GLRenderer;

	namespace GLText
	{
		/**
		 * Renders text at a 2D position specified in OpenGL viewport coordinates (origin is bottom-left).
		 *
		 * This is typically used to render text in the 2D map views.
		 *
		 * Creates text @a string at position (@a x , @a y , 0.0) in world coordinates
		 * using a particular @a colour and @a font.
		 *
		 * The current model-view and projection matrices of @a renderer, along with the current
		 * viewport, are used to project from 2D world position into 2D viewport coordinates.
		 *
		 * @a x_offset and @a y_offset are the pixel coordinate shifts to offset the text from
		 * where it would otherwise be.
		 * NOTE: These are specified in OpenGL viewport coordinates (origin is bottom-left).
		 * So if @a y_offset is '+1' then it offsets one pixel towards the top of the window.
		 *
		 * Also note that, because of this delegation to Qt for text rendering, the text rendering
		 * draw call cannot be queued (see the @a GLRenderer interface).
		 *
		 * Since OpenGL viewport and Qt use different coordinate systems this method
		 * inverts the 'y' coordinate before passing to Qt to render the text.
		 */
		void
		render_text_2D(
				GLRenderer &renderer,
				const double &world_x,
				const double &world_y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font,
				float scale = 1.0f);


		/**
		 * Renders text at a 3D position.
		 *
		 * This is typically used to render text in the 3D globe view.
		 *
		 * Creates text @a string at position (@a x , @a y , @a z) in world coordinates
		 * using a particular @a colour and @a font.
		 *
		 * The current model-view and projection matrices of @a renderer, along with the current
		 * viewport, are used to project from 3D world position into 2D viewport coordinates.
		 *
		 * @a x_offset and @a y_offset are the pixel coordinate shifts to offset the text from
		 * where it would otherwise be.
		 * NOTE: These are specified in OpenGL viewport coordinates (origin is bottom-left).
		 * So if @a y_offset is '+1' then it offsets one pixel towards the top of the window.
		 *
		 * Also note that, because of this delegation to Qt for text rendering, the text rendering
		 * draw call cannot be queued (see the @a GLRenderer interface).
		 */
		void
		render_text_3D(
				GLRenderer &renderer,
				double world_x,
				double world_y,
				double world_z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font = QFont(),
				float scale = 1.0f);
	};
}

#endif // GPLATES_OPENGL_GLTEXT2DDRAWABLE_H
