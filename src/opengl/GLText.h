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
#include <opengl/OpenGL1.h>

#include "gui/Colour.h"


namespace GPlatesOpenGL
{
	class GL;

	namespace GLText
	{
		/**
		 * Renders text at a 3D position.
		 *
		 * This is typically used to render text in the 3D globe view.
		 *
		 * Creates text @a string at position (@a x , @a y , @a z) in world coordinates
		 * using a particular @a colour and @a font.
		 * Note that @a y is the baseline of the text (ie, text is drawn just above the y-coordinate).
		 *
		 * The current model-view and projection matrices of @a renderer, along with the current
		 * viewport, are used to project from 3D world position into 2D viewport coordinates.
		 *
		 * @a x_offset and @a y_offset are the pixel coordinate shifts to offset the text from where it would otherwise be.
		 *
		 * NOTE: These are specified in OpenGL coordinate frame (where the origin is bottom-left, unlike Qt's top-left origin).
		 *       So if @a y_offset is '+1' then it offsets one pixel towards the top of the window.
		 * NOTE: Event though OpenGL uses device pixels the offsets should be specified in device-independent pixels
		 *       (eg, widget coordinates). The device pixel ratio is taken care of internally.
		 *
		 * @a scale scales the font size.
		 * NOTE: As with the pixel offsets, the scale factor should *not* take into account the device pixel ratio.
		 *
		 * Since OpenGL viewport and Qt use different coordinate systems this method
		 * inverts the 'y' coordinate before passing to Qt to render the text.
		 */
		void
		render_text_3D(
				GL &gl,
				double world_x,
				double world_y,
				double world_z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font = QFont(),
				float scale = 1.0f);


		/**
		 * Renders text at a 2D position specified in the OpenGL coordinate frame (origin is bottom-left).
		 *
		 * This is the same as @a render_text_3D except 'world_z' is zero.
		 *
		 * This is typically used to render text in the 2D map views.
		 *
		 * Note: If the input world coordinates are in device-independent pixels (eg, 2D text rendering) then the
		 *       projection transform should have been specified using the device-independent paint device dimensions.
		 */
		inline
		void
		render_text_2D(
				GL &gl,
				const double &world_x,
				const double &world_y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font,
				float scale = 1.0f)
		{
			render_text_3D(gl, world_x, world_y, 0.0, string, colour, x_offset, y_offset, font, scale);
		}
	};
}

#endif // GPLATES_OPENGL_GLTEXT2DDRAWABLE_H
