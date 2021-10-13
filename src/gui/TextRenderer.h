/* $Id$ */

/**
 * @file 
 * Base class for different implementations of rendering text in OpenGL
 * to reduce the dependency on a particular method of rendering text a la
 * the Adapter pattern in Design Patterns (Gamma, et al.)
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

#ifndef GPLATES_GUI_TEXTRENDERER_H
#define GPLATES_GUI_TEXTRENDERER_H

#include <algorithm>
#include <QString>
#include <QFont>
#include <QFontInfo>

#include "gui/Colour.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesGui
{
	class TextRenderer :
			public GPlatesUtils::ReferenceCount<TextRenderer>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TextRenderer> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TextRenderer> non_null_ptr_to_const_type;

		virtual
		~TextRenderer()
		{  }


		/**
		 * Specifies the renderer to use for subsequent text rendering.
		 *
		 * If a derived class uses a QPainter for text rendering then it is the QPainter specified
		 * to 'GLRenderer::begin_render'.
		 *
		 * The reason for @a begin_render and @a end_render is to avoid conflict in OpenGL state
		 * between our 'GLRenderer' and Qt ('QGLWidget' or an OpenGL QPainter).
		 */
		virtual
		void
		begin_render(
				GPlatesOpenGL::GLRenderer *renderer) = 0;


		//! Ends text rendering.
		virtual
		void
		end_render() = 0;

		/**
		 * RAII class to call @a begin_render and @a end_render over a scope.
		 */
		class RenderScope :
				private boost::noncopyable
		{
		public:
			RenderScope(
					TextRenderer &text_renderer,
					GPlatesOpenGL::GLRenderer *renderer);

			~RenderScope();

			//! Opportunity to end rendering before the scope exits (when destructor is called).
			void
			end_render();

		private:
			TextRenderer &d_text_renderer;
			GPlatesOpenGL::GLRenderer *d_renderer;
			bool d_called_end_render;
		};


		/**
		 * Renders @a string at position (@a x , @a y ) in *window* coordinates
		 * using a particular @a colour and @a font.
		 *
		 * The window coordinates use Qt's coordinate system where the origin is the
		 * upper-left corner of the window.
		 */
		virtual
		void
		render_text(
				int x,
				int y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				const QFont &font = QFont(),
				float scale = 1.0f) const = 0;


	protected:
		/**
		 * Utility method to scale a font.
		 */
		static
		QFont
		scale_font(
				const QFont &font,
				float scale);
	};
}

#endif  // GPLATES_GUI_TEXTRENDERER_H
