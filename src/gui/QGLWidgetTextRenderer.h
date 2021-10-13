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

#ifndef GPLATES_GUI_QGLWIDGETTEXTRENDERER_H
#define GPLATES_GUI_QGLWIDGETTEXTRENDERER_H

#include <opengl/OpenGL.h>
#include <QGLWidget>

#include "TextRenderer.h"


namespace GPlatesGui
{
	/**
	 * Renders text (using OpenGL) to a QGLWidget.
	 */
	class QGLWidgetTextRenderer :
			public TextRenderer
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<QGLWidgetTextRenderer> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const QGLWidgetTextRenderer> non_null_ptr_to_const_type;


		/**
		 * Constructs an instance of QGLWidgetTextRenderer on the heap.
		 */
		static
		non_null_ptr_type
		create(
				QGLWidget *gl_widget_ptr)
		{
			return new QGLWidgetTextRenderer(gl_widget_ptr);
		}


		//! Specifies the renderer to use for subsequent text rendering.
		virtual
		void
		begin_render(
				GPlatesOpenGL::GLRenderer *renderer);


		//! Ends text rendering.
		virtual
		void
		end_render();


		/**
		 * Renders @a string at position (@a x , @a y ) in window coordinates
		 * using a particular @a colour and @a font.
		 */
		virtual
		void
		render_text(
				int x,
				int y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				const QFont &font = QFont(),
				float scale = 1.0f) const;

	protected:

		explicit
		QGLWidgetTextRenderer(
				QGLWidget *gl_widget_ptr);

	private:

		QGLWidget *d_gl_widget_ptr;
		GPlatesOpenGL::GLRenderer *d_renderer;
	};
}

#endif  // GPLATES_GUI_QGLWIDGETTEXTRENDERER_H
