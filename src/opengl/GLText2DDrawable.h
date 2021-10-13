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

#include "GLDrawable.h"

#include "gui/Colour.h"
#include "gui/TextRenderer.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLTransformState;

	/**
	 * A drawable to render a text string in 2d window coordinates.
	 *
	 * To render a 3d text string us @a GLTextNode.
	 */
	class GLText2DDrawable :
			public GLDrawable
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLText2DDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLText2DDrawable> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLText2DDrawable.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLText2DDrawable> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLText2DDrawable object.
		 *
		 * Creates text @a string at position (@a x , @a y ) in window coordinates
		 * using a particular @a colour and @a font.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesGui::TextRenderer::non_null_ptr_to_const_type &text_renderer,
				int x,
				int y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				const QFont &font = QFont(),
				float scale = 1.0f)
		{
			return non_null_ptr_type(
					new GLText2DDrawable(text_renderer, x, y, string, colour, font, scale));
		}


		/**
		 * Creates a @a GLText2DDrawable object.
		 *
		 * Creates text @a string at position (@a x , @a y , @a z) in world coordinates
		 * using a particular @a colour and @a font.
		 *
		 * @a transform_state is used to project from 3D world position into 2D viewport coordinates.
		 *
		 * @a x_offset and @a y_offset are the pixel coordinate shifts to offset the text from
		 * where it would otherwise be.
		 */
		static
		non_null_ptr_type
		create(
				const GLTransformState &transform_state,
				const GPlatesGui::TextRenderer::non_null_ptr_to_const_type &text_renderer,
				double x,
				double y,
				double z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font = QFont(),
				float scale = 1.0f);


		virtual
		void
		bind() const
		{
			// Nothing to do - we are delegating to GPlatesGui::TextRenderer.
		}


		virtual
		void
		draw() const
		{
			d_text_renderer->render_text(d_x, d_y, d_string, d_colour, d_font, d_scale);
		}

	private:
		//! For rendering text
		GPlatesGui::TextRenderer::non_null_ptr_to_const_type d_text_renderer;

		int d_x;
		int d_y;
		QString d_string;
		GPlatesGui::Colour d_colour;
		QFont d_font;
		float d_scale;


		//! Constructor for 2D text.
		GLText2DDrawable(
				const GPlatesGui::TextRenderer::non_null_ptr_to_const_type &text_renderer,
				int x,
				int y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				const QFont &font,
				float scale) :
			d_text_renderer(text_renderer),
			d_x(x),
			d_y(y),
			d_string(string),
			d_colour(colour),
			d_font(font),
			d_scale(scale)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLTEXT2DDRAWABLE_H
